#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace blue
{
    static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    thread_local Scheduler *Scheduler::t_Scheduler = nullptr;
    thread_local int Scheduler::t_threadIndex = -1;

    Scheduler::Scheduler(size_t threads, const std::string &name)
        : m_name(name), m_threadCount(threads)
    {
        BLUE_ASSERT(threads > 0);
        // 设置随机范围
        m_steal_dist = std::uniform_int_distribution<>(0, threads - 1);
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

        std::lock_guard<std::mutex> lock(m_Schemutex);
        if (!m_stop.load(std::memory_order_acquire))
        {
            return;
        }

        m_stop.store(false, std::memory_order_release);
        m_stopping.store(false, std::memory_order_release);

        for (size_t i = 0; i < m_threadCount; i++)
        {
            m_workers.emplace_back([this, i]()
                                   {
                t_threadIndex = static_cast<int>(i);
                setThis(this);
                this->run(); });
        }
    }

    void Scheduler::stop()
    {
        if (m_stopping.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }

        m_stop.store(true, std::memory_order_release);

        m_Schecv.notify_all();

        for (auto &queue : m_threadQueues)
        {
            queue->cv.notify_all();
        }
        tickle();

        for (auto &worker : m_workers)
        {
            if (worker.joinable())
                worker.join();
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
            if (h && !h.done()) h.resume(); }, thr);
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
                // queue->tasks.emplace(std::move(cb), thr);
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
                // queue->tasks.emplace(std::move(cb), current_thread);
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
            // m_queue.emplace(std::move(cb), -1);
            m_queue.emplace_back(std::move(cb),-1);
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
        int idle_count = 0;
        const int MAX_IDLE = 50;

        while (true)
        {
            FuncAndId task;
            bool has_task = false;

            {
                std::unique_lock<std::mutex> lock(myQueue->mutex, std::try_to_lock);
                if (lock.owns_lock() && !myQueue->tasks.empty())
                {
                    task = std::move(myQueue->tasks.front());
                    myQueue->tasks.pop_front();
                    myQueue->pending.fetch_sub(1, std::memory_order_acq_rel);
                    has_task = true;
                    idle_count = 0;
                }
            }

            if (!has_task)
            {
                std::unique_lock<std::mutex> lock(m_Schemutex, std::try_to_lock);
                if (lock.owns_lock() && !m_queue.empty())
                {
                    task = std::move(m_queue.front());
                    m_queue.pop_front();
                    m_pending.fetch_sub(1, std::memory_order_acq_rel);
                    has_task = true;
                    idle_count = 0;
                }
            }

            if (!has_task)
            {
                has_task = stealTask(task);
                if (has_task)
                {
                    idle_count = 0;
                }
            }

            if (has_task && task.cb)
            {
                m_running.fetch_add(1,std::memory_order_acq_rel);
                try
                {
                    task.cb();
                    // BLUE_LOG_INFO(g_logger) << "cb back";
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
                // m_Schecv.notify_all(); // 在drainLocalQueue中通知
                m_running.fetch_sub(1,std::memory_order_acq_rel);
                m_Schecv.notify_all();
                continue;
            }

            if (m_stopping.load(std::memory_order_acquire))
            {
                break;
            }

            if (++idle_count > MAX_IDLE)
            {
                std::unique_lock<std::mutex> lock(m_Schemutex);
                if (m_queue.empty() && !has_task)
                {
                    bool all_empty = true;
                    for (auto &q : m_threadQueues)
                    {
                        std::lock_guard<std::mutex> qlock(q->mutex);
                        if (!q->tasks.empty())
                        {
                            all_empty = false;
                            break;
                        }
                    }
                    if (all_empty)
                    {
                        m_Schecv.wait_for(lock, std::chrono::milliseconds(10),
                                          [this]
                                          {
                                              return m_stopping.load(std::memory_order_acquire) ||
                                                     !m_queue.empty();
                                          });
                    }
                }
                idle_count = 0;
            }
            else
            {
                // 没有任务，短暂让出CPU
                std::this_thread::yield();
            }
        }

        // 退出前处理完本线程的所有任务
        drainLocalQueue(myIndex);
    }

    void Scheduler::drainLocalQueue(size_t index)
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
            m_Schecv.notify_all();      // 通知一下，因为子任务队列可能没有任务了
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
        std::unique_lock<std::mutex> lock(m_Schemutex);
        m_Schecv.wait(lock, [this]
                      {
            size_t total = m_pending.load(std::memory_order_acquire);
            for (auto& queue : m_threadQueues)
            {
                total += queue->pending.load(std::memory_order_acquire);
            }
            return total == 0 && m_running.load(std::memory_order_acquire) == 0; });
    }

    int Scheduler::GetThreadIndex()
    {
        return t_threadIndex;
    }

    int Scheduler::GetThreadCount()
    {
        Scheduler *sched = GetThisUnsafe();
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

    void schedule_coroutine(std::coroutine_handle<> h)
    {
        Scheduler::GetThis()->schedule(h);
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
            int victim = m_steal_dist(m_rng);
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