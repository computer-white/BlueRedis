#pragma once
#include <condition_variable>
#include <string>
#include <deque>
#include <mutex>
#include <atomic>
#include "src/core/task.h"
#include "src/util/defer.h"
#include "src/core/await.h"

namespace blue
{
    namespace spider
    {
        class UrlQueue
        {
        public:
            enum class PopResult
            {
                URL,
                DONE
            };

            void push(std::string url, int depth);

            blue::Task<PopResult> pop(std::string &out, int &depth);

            void fetchStarted() { m_inflight.fetch_add(1); }

            void fetchFinished() { m_inflight.fetch_sub(1); }

            size_t size() const { return m_size.load(); }

            int inflight() const { return m_inflight.load(); }

            bool finished() const
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_queue.empty() && m_inflight.load() == 0;
            }

        private:
            std::atomic<size_t> m_inflight{0};
            std::atomic<size_t> m_size{0};
            mutable std::mutex m_mutex;
            std::deque<std::pair<std::string, int>> m_queue;
        };

        inline void UrlQueue::push(std::string url, int depth)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.emplace_back(std::move(url), depth);
            m_size.fetch_add(1);
        }

        inline blue::Task<UrlQueue::PopResult> UrlQueue::pop(std::string &out, int &depth)
        {
            while (true)
            {
                if (finished())
                {
                    co_return PopResult::DONE;
                }

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (!m_queue.empty())
                    {
                        auto ans = std::move(m_queue.front());
                        out = ans.first;
                        depth = ans.second;
                        m_queue.pop_front();
                        m_size.fetch_sub(1);
                        co_return PopResult::URL;
                    }
                }
                co_await blue::sleepForMs(10);
            }
        }
    }
}