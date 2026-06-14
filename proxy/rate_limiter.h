#pragma once
#include <unordered_set>
#include "blue/redismanager.h"

namespace blue
{
    namespace http
    {
        extern blue::RedisManager::RedisManagerPtr s_redismanager_ptr;
    }   
    namespace proxy
    {
        class RateLimiter
        {
        public:
            static RateLimiter &instance()
            {
                static RateLimiter limiter;
                return limiter;
            }

            void setLimit(uint64_t val) { m_limit = val; }
            void setExpire(uint64_t val) { m_expire = val; }

            bool allow(const std::string &ip)
            {
                if (m_whitelist.contains(ip))
                {
                    return true;
                }
                std::string key = "rate:" + ip;
                long long count = blue::http::s_redismanager_ptr->incr(key);
                if (count == 1)
                {
                    // 设置60秒窗口
                    blue::http::s_redismanager_ptr->expire(key, m_expire); // 窗口
                }
                return count <= m_limit;
            }

            void addWhiteList(const std::string &ip)
            {
                m_whitelist.insert(ip);
            }

        private:
            uint64_t m_limit;
            uint64_t m_expire;
            std::unordered_set<std::string> m_whitelist = {"127.0.0.1", "::1", "localhost"};
        };
    }
}