#include <filesystem>
#include "crawler.h"
#include "resolve.h"

namespace blue
{
    namespace spider
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        Crawler::Crawler(std::shared_ptr<Fetcher> fetcher, int concurrency, double rate_per_sec, int max_depth, const std::string &outDir)
            : m_fetcher(fetcher),
              m_concurrency(concurrency),
              m_bucket(rate_per_sec),
              m_max_depth(max_depth),
              m_outputDir(outDir)
        {

            std::filesystem::create_directories(m_outputDir);
        }

        blue::Task<void> Crawler::crawl(std::vector<std::string> seeds)
        {
            for (auto &s : seeds)
            {
                if (!tryMarkSeen(s))
                {
                    continue;
                }
                m_queue.push(s, 0);
            }

            m_wg.Add(m_concurrency);

            for (int i = 0; i < m_concurrency; ++i)
            {
                IOManager::GetThis()->schedule(worker(i));
            }

            co_await m_wg.Wait();
            BLUE_LOG_INFO(g_logger) << "crawl done: total=" << m_total.load(std::memory_order_acquire)
                                    << " duplicates=" << m_duplicates.load(std::memory_order_acquire)
                                    << " unique=" << m_unique.load(std::memory_order_acquire);
            co_return;
        }

        blue::Task<void> Crawler::worker(int idx)
        {
            BLUE_LOG_INFO(g_logger) << "worker" << idx << "Start";
            while (true)
            {
                std::string url;
                int depth = 0;
                auto r = co_await m_queue.pop(url, depth);

                if (r == UrlQueue::PopResult::DONE)
                {
                    break;
                }

                // 令牌限流
                co_await m_bucket.acquire();

                m_queue.fetchStarted();
                Page page = co_await m_fetcher->fetch(url, 5000);
                m_queue.fetchFinished();

                m_total.fetch_add(1);

                savePage(page);

                auto links = parseLinks(page.body, url);
                for (auto &link : links)
                {
                    if (depth + 1 > m_max_depth)
                    {
                        continue;
                    }

                    if (!tryMarkSeen(link))
                    {
                        m_duplicates.fetch_add(1);
                        continue;
                    }
                    m_unique.fetch_add(1);
                    m_queue.push(std::move(link), depth + 1);
                }
            }
            BLUE_LOG_INFO(g_logger) << "worker" << idx << "DONE";
            m_wg.Done();
            co_return;
        }

        void Crawler::savePage(const Page &page)
        {
            // 1. 文件名：url 的 sha1/md5，避免非法字符
            std::string hash = urlHash(page.url);
            std::string htmlPath = m_outputDir + "/" + hash + ".html";

            {
                std::ofstream ofs(htmlPath, std::ios::binary);
                if (!ofs)
                {
                    BLUE_LOG_WARN(g_logger) << "savePage: open " << htmlPath << " failed";
                    return;
                }
                ofs.write(page.body.data(), page.body.size());
            }

            // 2. index.tsv 追加：url \t title \t status \t elapsed_ms
            {
                std::lock_guard<std::mutex> lock(m_indexMutex);
                if (!m_index.is_open())
                {
                    m_index.open(m_outputDir + "/index.tsv", std::ios::app);
                }
                if (m_index)
                {
                    std::string title = page.title;
                    std::replace(title.begin(), title.end(), '\t', ' ');
                    std::replace(title.begin(), title.end(), '\n', ' ');
                    m_index << page.url << '\t' << title << '\t'
                            << page.status << '\t' << page.elapsed_ms << '\n';

                    if (++m_indexCount % 100 == 0)
                    {
                        m_index.flush();
                    }
                }
            }
        }

        std::string Crawler::urlHash(const std::string &url)
        {
            size_t h = std::hash<std::string>{}(url);
            char buf[17];
            std::snprintf(buf, sizeof(buf), "%016zx", h);
            return buf;
        }

        std::vector<std::string> Crawler::parseLinks(const std::string &html,
                                                     const std::string &baseUrl)
        {
            std::vector<std::string> links;

            // 1. 提取所有 <a ... href="...">
            static const std::regex aRe(
                R"(<a\s[^>]*href\s*=\s*["']([^"']+)["'])",
                std::regex::icase);

            auto begin = std::sregex_iterator(html.begin(), html.end(), aRe);
            auto end = std::sregex_iterator();

            auto base = blue::Url::CreateUrl(baseUrl);
            if (!base)
                return links;

            for (auto it = begin; it != end; ++it)
            {
                std::string href = (*it)[1].str();

                // 2. 过滤：空、锚点、javascript、mailto、data
                if (href.empty() || href[0] == '#')
                {
                    continue;
                }
                if (href.rfind("javascript:", 0) == 0)
                {
                    continue;
                }
                if (href.rfind("mailto:", 0) == 0)
                {
                    continue;
                }
                if (href.rfind("data:", 0) == 0)
                {
                    continue;
                }
                if (href.rfind("tel:", 0) == 0)
                {
                    continue;
                }

                // 3. 相对路径转绝对
                std::string abs = resolveUrl(baseUrl, href);
                if (abs.empty())
                {
                    continue;
                }

                // 4. 只保留 http/https
                if (abs.rfind("http://", 0) != 0 && abs.rfind("https://", 0) != 0)
                {
                    continue;
                }

                // 5. 去掉 fragment（#xxx），统一形态
                auto pos = abs.find('#');
                if (pos != std::string::npos)
                {
                    abs.erase(pos);
                }

                links.push_back(std::move(abs));
            }
            return links;
        }
    }
}