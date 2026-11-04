// fetcher.h
#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <regex>
#include "src/http/httpconnection.h"
#include "src/util/url.h"
#include "src/core/task.h"

namespace blue
{
    namespace spider
    {
        struct Page
        {
            std::string url;
            int status = 0; // HTTP 状态码，0 失败
            std::string body;
            std::string title;
            int64_t elapsed_ms = 0;
        };

        class Fetcher
        {
        public:
            explicit Fetcher(std::string user_agent = "BlueSpider/1.0",
                             uint64_t pool_alive_ms = 60 * 1000,
                             uint32_t pool_max_request = 100,
                             uint32_t pool_max_size = 32)
                : m_userAgent(std::move(user_agent)),
                  m_poolAliveMs(pool_alive_ms),
                  m_poolMaxRequest(pool_max_request),
                  m_poolMaxSize(pool_max_size)
            {
            }

            blue::Task<Page> fetch(const std::string &url, uint64_t timeout_ms = 5000);

        private:
            blue::http::HttpConnectionPool::HttpConnectionPoolPtr getPool(const blue::Url::UrlPtr &url);

            static std::string extractTitle(const std::string &html);

            std::string m_userAgent;
            uint64_t m_poolAliveMs;
            uint32_t m_poolMaxRequest;
            uint32_t m_poolMaxSize;
            std::mutex m_poolMutex;
            std::unordered_map<std::string, blue::http::HttpConnectionPool::HttpConnectionPoolPtr> m_pools;
        };
    }
}