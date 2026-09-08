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

            /**
             * @brief 设置访问上限
             */
            void setLimit(uint64_t val) { m_limit.store(val, std::memory_order_release); }

            /**
             * @brief 设置限速窗口时间
             */
            void setExpire(uint64_t val) { m_expire.store(val, std::memory_order_release); }

            /**
             * @brief 检查ip是否在白名单,不在检查limit
             */
            bool allow(const std::string &ip)
            {
                std::shared_lock<std::shared_mutex> lock(m_mutex);
                if (m_whitelist.contains(ip))
                {
                    return true;
                }
                lock.unlock();

                // TODO 多线程这里有问题，无法保证incr正确次数，后序再改
                std::string key = "rate:" + ip;
                long long count = blue::http::s_redismanager_ptr->incr(key);
                if (count == 1)
                {
                    // 设置60秒窗口
                    blue::http::s_redismanager_ptr->expire(key, m_expire); // 窗口
                }
                return count <= m_limit;
            }

            /**
             * @brief 添加ip到白名单
             */
            void addWhiteList(const std::string &ip)
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_whitelist.insert(ip);
            }

        private:
            std::atomic<uint64_t> m_limit{0};
            std::atomic<uint64_t> m_expire{0};
            std::shared_mutex m_mutex;
            std::unordered_set<std::string> m_whitelist = {"127.0.0.1", "::1", "localhost"};
        };
    }
}