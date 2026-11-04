#include <algorithm>
#include <cctype>
#include "fetcher.h"
#include "resolve.h"
#include "src/util/log.h"

namespace blue
{
    namespace spider
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("spider");

        Task<Page> Fetcher::fetch(const std::string &url, uint64_t timeout_ms)
        {
            Page page;
            page.url = url;
            auto start = std::chrono::steady_clock::now();

            auto urlptr = blue::Url::CreateUrl(url);
            if (!urlptr)
            {
                BLUE_LOG_WARN(g_logger) << "invalid url: " << url;
                co_return page;
            }

            auto pool = getPool(urlptr);
            if (!pool)
            {
                BLUE_LOG_WARN(g_logger) << "no pool for url: " << url;
                co_return page;
            }

            std::map<std::string, std::string> headers;
            headers["User-Agent"] = m_userAgent;
            headers["Accept"] = "text/html,application/xhtml+xml";

            std::string current = url;
            int redirects = 0;
            while (true)
            {
                auto result = co_await pool->doGet(current, timeout_ms, headers);
                if (!result || result->result != (int)blue::http::HttpResult::ResultStatus::OK || !result->response)
                {
                    BLUE_LOG_WARN(g_logger) << "fetch failed: " << current
                                            << " reason: " << (result ? result->reason : "null result");
                    co_return page;
                }

                page.status = (int)(result->response->getStatus());
                page.body = result->response->getBody();
                page.url = current;

                if (page.status >= 300 && page.status < 400 && redirects < 3)
                {
                    auto loc = result->response->getHeader("Location");
                    if (loc.empty())
                    {
                        break;
                    }
                    auto next = resolveUrl(current, loc);
                    if (next.empty())
                    {
                        break;
                    }
                    current = next;
                    ++redirects;
                    continue;
                }
                break;
            }

            if (page.status == 200 && !page.body.empty())
            {
                page.title = extractTitle(page.body);
            }

            auto end = std::chrono::steady_clock::now();
            page.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            co_return page;
        }

        blue::http::HttpConnectionPool::HttpConnectionPoolPtr
        Fetcher::getPool(const blue::Url::UrlPtr &url)
        {
            std::string key = url->getScheme() + "://" + url->getHost() + ":" + std::to_string(url->getPort());
            std::lock_guard<std::mutex> lock(m_poolMutex);
            auto it = m_pools.find(key);
            if (it != m_pools.end())
            {
                return it->second;
            }

            auto pool = std::make_shared<blue::http::HttpConnectionPool>(
                url->getHost(), // host
                url->getHost(), // vhost
                url->getPort(),
                m_poolAliveMs,
                m_poolMaxRequest,
                url->getScheme(), // scheme
                m_poolMaxSize);
            m_pools.emplace(key, pool);
            return pool;
        }

        std::string Fetcher::extractTitle(const std::string &html)
        {
            static const std::regex titleRe(R"(<title[^>]*>([^<]*)</title>)", std::regex::icase);
            std::smatch m;
            if (std::regex_search(html, m, titleRe) && m.size() > 1)
            {
                std::string t = m[1].str();
                auto notspace = [](unsigned char c)
                { return !std::isspace(c); };
                t.erase(t.begin(), std::find_if(t.begin(), t.end(), notspace));
                t.erase(std::find_if(t.rbegin(), t.rend(), notspace).base(), t.end());
                return t;
            }
            return "";
        }
    }
}