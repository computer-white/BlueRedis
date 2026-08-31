/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file rate_limiter.h
 * @brief redis限流模块,用于httpserver.cpp中对同一客户端进行限流操作
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.19
 * @copyright Copyright (c) 2026年 blue
 */
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