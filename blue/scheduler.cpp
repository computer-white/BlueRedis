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
#include "blue/scheduler.h"
#include "blue/log.h"
#include "blue/macro.h"

namespace blue
{
    static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    thread_local Scheduler *Scheduler::t_Scheduler = nullptr;
    thread_local int Scheduler::t_threadIndex = -1;
    static thread_local std::mt19937 t_rng{std::random_device{}()};

    Scheduler::Scheduler(size_t threads)
        : m_threadCount(threads)
    {
        BLUE_ASSERT(threads > 0);
        // 提前预分配
        m_threadQueues.reserve(threads);
        for (size_t i = 0; i < threads; i++)
        {
            m_threadQueues.push_back(std::make_unique<PerThreadQueue>());
        }
    }

    Scheduler::~Scheduler()
    {
        if (!m_stopping.load(std::memory_order_acquire))
        {
            stop();
        }
    }

    void Scheduler::start()
    {
        if (!m_stop.load(std::memory_order_acquire))
        {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_Schemutex);
            if (!m_stop.load(std::memory_order_acquire))
            {
                return;
            }

            m_stop.store(false, std::memory_order_release);
            m_stopping.store(false, std::memory_order_release);
            m_workers.reserve(m_threadCount);
        }

        // start只有一次，放在锁外执行
        for (size_t i = 0; i < m_threadCount; i++)
        {
            auto t = std::make_unique<blue::Mthread>(
                [this, i]()
                {
                    t_threadIndex = static_cast<int>(i);
                    setThis(this);
                    this->run();
                },
                // 线程名称
                "Schedule" + std::to_string(i + 1));
            m_workers.push_back(std::move(t));
        }
    }

    void Scheduler::stop()
    {
        // 正在停止调度器过程中
        if (m_stopping.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }

        m_stop.store(true, std::memory_order_release);

        m_Schecv.notify_all();
        m_doneCv.notify_all();
        for (auto &queue : m_threadQueues)
        {
            queue->cv.notify_all();
        }
        tickle();

        for (auto &worker : m_workers)
        {
            worker->join();
        }
        m_workers.clear();

        // join 完成后，把仍然滞留在本地队列的任务搬到全局队列
        for (size_t i = 0; i < m_threadQueues.size(); ++i)
        {
            drainLocalQueue(i);
        }
    }

    void Scheduler::schedule(std::coroutine_handle<> h, int thr)
    {
        if (!h || h.done())
        {
            return;
        }

        schedule([h]() mutable
                 {
            if (h && h.address() && !h.done()) h.resume();}, thr);
    }

    void Scheduler::schedule(std::function<void()> cb, int thr)
    {
        if (!cb)
        {
            return;
        }

        // 指定了线程放到指定线程中
        if (thr >= 0 && thr < static_cast<int>(m_threadCount))
        {
            auto &queue = m_threadQueues[thr];
            bool need_notify = false;
            {
                std::lock_guard<std::mutex> lock(queue->mutex);
                if (m_stopping.load(std::memory_order_acquire))
                {
                    return;
                }
                need_notify = queue->tasks.empty();
                queue->tasks.emplace_back(std::move(cb), thr);
                queue->pending.fetch_add(1, std::memory_order_acq_rel);
            }
            if (need_notify)
            {
                queue->cv.notify_one();
                tickle();
            }
            return;
        }

        // 如果没有指定线程，优先放入当前线程的本地队列
        int current_thread = GetThreadIndex();
        if (current_thread >= 0 && current_thread < static_cast<int>(m_threadCount))
        {
            auto &queue = m_threadQueues[current_thread];
            bool need_notify = false;
            {
                std::lock_guard<std::mutex> lock(queue->mutex);
                if (m_stopping.load(std::memory_order_acquire))
                {
                    return;
                }
                need_notify = queue->tasks.empty();
                queue->tasks.emplace_back(std::move(cb), current_thread);
                queue->pending.fetch_add(1, std::memory_order_acq_rel);
            }
            if (need_notify)
            {
                queue->cv.notify_one();
                tickle();
            }
            return;
        }
        // 如果当前线程不在调度器，放入全局队列
        bool need_notify = false;
        {
            std::lock_guard<std::mutex> lock(m_Schemutex);
            if (m_stopping.load(std::memory_order_acquire))
            {
                return;
            }
            need_notify = m_queue.empty();
            m_queue.emplace_back(std::move(cb), -1);
            m_pending.fetch_add(1, std::memory_order_acq_rel);
        }
        if (need_notify)
        {
            m_Schecv.notify_one();
            tickle();
        }
    }

    void Scheduler::run()
    {
        int myIndex = t_threadIndex;
        auto &myQueue = m_threadQueues[myIndex];

        while (true)
        {
            FuncAndId task;
            bool has_task = false;

            // 本地队列
            {
                std::unique_lock<std::mutex> lock(myQueue->mutex);
                if (!myQueue->tasks.empty())
                {
                    task = std::move(myQueue->tasks.front());
                    myQueue->tasks.pop_front();
                    myQueue->pending.fetch_sub(1, std::memory_order_acq_rel);
                    has_task = true;
                }
            }

            // 全局队列
            if (!has_task)
            {
                std::unique_lock<std::mutex> lock(m_Schemutex);
                if (!m_queue.empty())
                {
                    task = std::move(m_queue.front());
                    m_queue.pop_front();
                    m_pending.fetch_sub(1, std::memory_order_acq_rel);
                    has_task = true;
                }
            }

            // 窃取
            if (!has_task)
            {
                has_task = stealTask(task);
            }

            // 执行
            if (has_task && task.cb)
            {
                m_running.fetch_add(1, std::memory_order_acq_rel);
                try
                {
                    task.cb();
                }
                catch (const std::exception &e)
                {
                    BLUE_LOG_ERROR(g_logger) << "Task error in thread " << myIndex
                                             << ": " << e.what();
                }
                catch (...)
                {
                    BLUE_LOG_ERROR(g_logger) << "Unknown task error in thread " << myIndex;
                }
                m_running.fetch_sub(1, std::memory_order_acq_rel);

                // 只有确实有人在等 wait_all 时才唤醒，避免每次任务 notify_all
                if (m_waiting.load(std::memory_order_acquire) > 0)
                {
                    m_doneCv.notify_all();
                }
                continue;
            }

            // 退出检查
            if (m_stopping.load(std::memory_order_acquire))
            {
                break;
            }

            // 无任务：阻塞等待本地队列 / 全局队列 / 停止
            {
                std::unique_lock<std::mutex> lock(myQueue->mutex);
                myQueue->cv.wait_for(lock, std::chrono::milliseconds(5), [&]
                                     { return !myQueue->tasks.empty() ||
                                              m_stopping.load(std::memory_order_acquire) ||
                                              m_pending.load(std::memory_order_acquire) > 0; });
            }

            // 若全局队列有东西，及时让出，让循环下次去抢
            if (m_pending.load(std::memory_order_acquire) > 0)
            {
                std::this_thread::yield();
            }
        }

        // 退出前把本线程剩余任务交给全局队列
        drainLocalQueue(myIndex);
    }

    void Scheduler::drainLocalQueue(int index)
    {
        auto &queue = m_threadQueues[index];
        std::vector<FuncAndId> remaining;

        {
            std::lock_guard<std::mutex> lock(queue->mutex);
            while (!queue->tasks.empty())
            {
                remaining.push_back(std::move(queue->tasks.front()));
                queue->tasks.pop_front();
                queue->pending.fetch_sub(1, std::memory_order_acq_rel);
            }
        }

        // 放回全局队列，让其他线程处理
        if (!remaining.empty())
        {
            BLUE_LOG_WARN(g_logger) << "Thread " << index
                                    << " draining " << remaining.size() << " tasks";
            std::lock_guard<std::mutex> lock(m_Schemutex);
            for (auto &task : remaining)
            {
                m_queue.emplace_back(std::move(task.cb), -1);
                m_pending.fetch_add(1, std::memory_order_acq_rel);
            }
            m_Schecv.notify_all(); // 通知一下，因为子任务队列可能没有任务了
            m_doneCv.notify_all();
        }
    }

    void Scheduler::tickle()
    {
    }

    bool Scheduler::stopping()
    {
        return m_stopping.load(std::memory_order_acquire);
    }

    void Scheduler::wait_all()
    {
        std::unique_lock<std::mutex> lock(m_doneMutex);
        m_waiting.fetch_add(1, std::memory_order_acq_rel);
        while (true)
        {
            size_t total = m_pending.load(std::memory_order_acquire);
            for (auto &queue : m_threadQueues)
            {
                total += queue->pending.load(std::memory_order_acquire);
            }
            if (total == 0 && m_running.load(std::memory_order_acquire) == 0)
            {
                break;
            }
            m_doneCv.wait_for(lock, std::chrono::milliseconds(10));
        }
        m_waiting.fetch_sub(1, std::memory_order_acq_rel);
        clearFinishedTasks();
    }

    int Scheduler::GetThreadIndex()
    {
        return t_threadIndex;
    }

    int Scheduler::GetThreadCount()
    {
        Scheduler *sched = GetThis();
        return sched ? static_cast<int>(sched->m_threadCount) : 0;
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_Scheduler;
    }

    void Scheduler::setThis(Scheduler *t)
    {
        t_Scheduler = t;
    }

    bool Scheduler::stealTask(FuncAndId &task, int max_attempts)
    {
        if (m_threadCount <= 1)
        {
            return false;
        }
        int my_idx = t_threadIndex;
        int attempt = 0;
        while (attempt < max_attempts)
        {
            int victim = static_cast<int>(t_rng() % m_threadCount);
            if (victim == my_idx)
            {
                attempt++;
                continue;
            }

            auto &victim_queue = m_threadQueues[victim];
            std::unique_lock<std::mutex> lock(victim_queue->mutex, std::try_to_lock);

            if (lock.owns_lock() && !victim_queue->tasks.empty())
            {
                task = std::move(victim_queue->tasks.back());
                victim_queue->tasks.pop_back();
                victim_queue->pending.fetch_sub(1, std::memory_order_acq_rel);
                return true;
            }
            attempt++;
        }
        return false;
    }
}