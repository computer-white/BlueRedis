#pragma once
#include <unordered_set>
#include "example/spider/fetcher.h"
#include "example/spider/url_queue.h"
#include "example/spider/token_bucket.h"
#include "src/core/wait_group.h"

namespace blue
{
    namespace spider
    {
        class Crawler
        {
        public:
            /**
             * @param fetcher std::shared_ptr<fetcher>
             * @param concurrency 并发线程数
             * @param max_depth 最大深度
             * @param outDir 输出目录
             */
            Crawler(std::shared_ptr<Fetcher> fetcher,
                    int concurrency = 4,
                    double rate_per_sec = 100.0,
                    int max_depth = 3,
                    const std::string &outDir = "/tmp/spider_out");

            ~Crawler()
            {
                if (m_index.is_open())
                {
                    m_index.flush();
                    m_index.close();
                }
            }

            blue::Task<void> crawl(std::vector<std::string> seeds);

            int total() const { return m_total.load(std::memory_order_acquire); }

            int duplicates() const { return m_duplicates.load(std::memory_order_acquire); }

            int unique() const { return m_unique.load(std::memory_order_acquire); }

        private:
            blue::Task<void> worker(int idx);

            bool tryMarkSeen(const std::string &url)
            {
                std::lock_guard<std::mutex> lock(m_seedMutex);
                if (m_seen.count(url))
                {
                    return false;
                }
                return m_seen.insert(url).second;
            }

            void savePage(const Page &page);

            std::vector<std::string> parseLinks(const std::string &html,
                                                const std::string &baseUrl);

            static std::string urlHash(const std::string &url);

        private:
            std::shared_ptr<Fetcher> m_fetcher;

            blue::spider::UrlQueue m_queue;
            blue::spider::TokenBucket m_bucket;
            blue::WaitGroup m_wg;

            std::mutex m_seedMutex;
            std::unordered_set<std::string> m_seen;

            std::string m_outputDir;
            std::mutex m_indexMutex;
            std::ofstream m_index;
            size_t m_indexCount = 0;

            int m_concurrency;                // 并发线程数
            int m_max_depth;                  // 最大递归深度
            std::atomic<int> m_total{0};      // 总共访问的
            std::atomic<int> m_duplicates{0}; // 重复的
            std::atomic<int> m_unique{0};     // 去掉重复的
        };
    }
}
