#pragma once
#include <atomic>
#include <atomic>
#include <mutex>
#include <vector>
#include <coroutine>
#include <memory>
#include "src/core/scheduler.h"
#include "src/core/resume_ticket.h"

namespace blue
{
    class WaitGroup
    {
    public:
        WaitGroup() = default;

        WaitGroup(const WaitGroup &) = delete;
        WaitGroup &operator=(const WaitGroup &) = delete;

        ~WaitGroup()
        {
            std::vector<std::shared_ptr<ResumeTicket>> waiters;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                waiters.swap(m_waiters);
            }
            for (auto &ticket : waiters)
            {
                ticket->m_armed.store(false, std::memory_order_release); // 不再 resume
            }
        }

        /**
         * @brief 增加计数
         */
        void Add(int n = 1)
        {
            if (n < 0)
            {
                throw std::invalid_argument("argument can't be negative number");
            }
            m_count.fetch_add(n, std::memory_order_acq_rel);
        }

        /**
         * @brief 减少计数，归零唤醒所有等待者
         */
        void Done()
        {
            if (m_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                std::vector<std::shared_ptr<ResumeTicket>> waiters;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    waiters.swap(m_waiters);
                }
                for (auto &ticket : waiters)
                {
                    if (auto *sched = Scheduler::GetThis())
                    {
                        sched->schedule([ticket]
                                        { try_resume(ticket); });
                    }
                    else
                    {
                        try_resume(ticket);
                    }
                }
            }
        }

        struct WaitAwaiter
        {
            WaitGroup *wg;
            std::shared_ptr<ResumeTicket> ticket; // 帧外判断

            bool await_ready() const noexcept
            {
                return wg->m_count.load(std::memory_order_acquire) == 0;
            }

            void await_suspend(std::coroutine_handle<> h)
            {
                ticket = std::make_shared<ResumeTicket>();
                ticket->m_handle = h;
                ticket->m_armed.store(true, std::memory_order_release);

                std::lock_guard<std::mutex> lock(wg->m_mutex);
                if (wg->m_count.load(std::memory_order_acquire) == 0)
                {
                    try_resume(ticket); // 计数已归零，立即恢复
                    return;
                }
                wg->m_waiters.push_back(ticket);
            }

            void await_resume() const noexcept
            {
                if (ticket)
                {
                    ticket->m_armed.store(false, std::memory_order_release);
                }
            }
        };

        WaitAwaiter Wait()
        {
            return WaitAwaiter{this};
        }

    private:
        std::atomic<int> m_count{0};
        std::mutex m_mutex;
        std::vector<std::shared_ptr<ResumeTicket>> m_waiters;
    };
}