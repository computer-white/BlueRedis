#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include "src/core/await.h"

namespace blue
{
    namespace spider
    {
        /**
         * @brief 令牌桶限流器
         *
         * rate_per_sec 每秒补充令牌数，capacity 桶容量（允许突发）。
         * acquire() 拿不到令牌时 co_await sleepFor(10) 重试，不阻塞调度器线程。
         */
        class TokenBucket
        {
        public:
            explicit TokenBucket(double rate_per_sec, double capacity = 0)
                : m_rate(rate_per_sec),
                  m_capacity(capacity > 0 ? capacity : rate_per_sec),
                  m_tokens(m_capacity),
                  m_last_refill(std::chrono::steady_clock::now())
            {
            }

            blue::Task<void> acquire()
            {
                while (true)
                {
                    {
                        m_mutex.lock();
                        defer(m_mutex.unlock());
                        refillLocked();
                        if (m_tokens >= 1.0)
                        {
                            m_tokens -= 1.0;
                            co_return;
                        }
                    }
                    co_await blue::sleepFor(10);
                }
            }

            double tokens() const
            {
                m_mutex.lock();
                defer(m_mutex.unlock());
                return m_tokens;
            }

        private:
            void refillLocked() const
            {
                auto now = std::chrono::steady_clock::now();
                double elapsed =
                    std::chrono::duration<double>(now - m_last_refill).count();
                m_tokens = std::min(m_capacity, m_tokens + elapsed * m_rate);
                m_last_refill = now;
            }

            mutable std::mutex m_mutex;
            double m_rate;              // 每秒补充令牌数量
            double m_capacity;          // 桶容量
            mutable double m_tokens;    // 令牌数量
            mutable std::chrono::steady_clock::time_point m_last_refill;
        };
    }
}