#include "timer.h"
#include "scheduler.h"
#include "log.h"
#include <cassert>

namespace blue
{
    static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    // ==================== Timer 实现 ====================

    bool Timer::Comparator::operator()(const Timer::TimerPtr &lhs, const Timer::TimerPtr &rhs) const
    {
        if (!lhs && !rhs)
            return false;
        if (!lhs)
            return true; // nullptr 排在最前面
        if (!rhs)
            return false;

        if (lhs->m_expire == rhs->m_expire)
        {
            return lhs.get() < rhs.get();
        }
        return lhs->m_expire < rhs->m_expire;
    }

    Timer::Timer(uint64_t ms, std::coroutine_handle<> h,
                 std::function<void()> cb, bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(std::move(cb)), m_handle(h), m_manager(manager)
    {
        m_expire = SClock::now() + std::chrono::milliseconds(ms);
    }

    Timer::Timer(TimePoint expire)
        : m_expire(expire)
    {
    }

    std::shared_ptr<Timer> Timer::Create_by_next(TimePoint expire)
    {
        return std::shared_ptr<Timer>(new Timer(expire));
    }

    std::shared_ptr<Timer> Timer::Create_by_ms(uint64_t ms, std::coroutine_handle<> h,
                                               std::function<void()> cb, bool recurring, TimerManager *manager)
    {
        return std::shared_ptr<Timer>(new Timer(ms, h, std::move(cb), recurring, manager));
    }

    bool Timer::cancel()
    {
        // 使用原子操作快速检查
        if (!m_valid.load(std::memory_order_acquire))
        {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_manager->m_mutex);

        // 双重检查
        if (!m_valid.load(std::memory_order_acquire))
        {
            return false;
        }

        // 标记为无效
        m_valid.store(false, std::memory_order_release);
        m_cb = nullptr;
        m_handle = nullptr;

        // 从管理器中移除
        m_manager->m_timers.erase(shared_from_this());

        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if (!m_valid.load(std::memory_order_acquire))
        {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_manager->m_mutex);

        if (!m_valid.load(std::memory_order_acquire))
        {
            return false;
        }

        // 更新周期（如果指定了新值）
        if (ms != static_cast<uint64_t>(-1))
        {
            m_ms = ms;
        }

        // 计算新的过期时间
        TimePoint start = from_now ? SClock::now()
                                   : (m_expire - std::chrono::milliseconds(m_ms));
        TimePoint new_expire = start + std::chrono::milliseconds(m_ms);

        // 如果过期时间没变，不需要更新
        if (new_expire == m_expire)
        {
            return true;
        }

        // 从集合中移除
        auto self = shared_from_this();
        auto it = m_manager->m_timers.find(self);
        if (it != m_manager->m_timers.end())
        {
            m_manager->m_timers.erase(it);
        }
        else
        {
            // 不在集合中（不应该发生，但容错处理）
            m_valid.store(false, std::memory_order_release);
            return false;
        }

        // 更新过期时间并重新插入
        m_expire = new_expire;
        auto [new_it, inserted] = m_manager->m_timers.insert(self);

        if (!inserted)
        {
            // 插入失败（不应该发生）
            m_valid.store(false, std::memory_order_release);
            return false;
        }

        // 检查是否插入到了队首
        bool at_front = (new_it == m_manager->m_timers.begin());
        if (at_front)
        {
            m_manager->m_tickle.store(true, std::memory_order_release);
        }

        lock.unlock();

        if (at_front)
        {
            m_manager->onTimerInsertedAtFront();
        }

        return true;
    }

    // ==================== TimerManager 实现 ====================

    TimerManager::TimerManager()
    {
        m_previousTime = std::chrono::steady_clock::now();
    }

    Timer::TimerPtr TimerManager::addTimer(uint64_t ms, std::coroutine_handle<> h,
                                           std::function<void()> cb, bool recurring)
    {
        auto timer = Timer::Create_by_ms(ms, h, std::move(cb), recurring, this);

        std::unique_lock<std::shared_mutex> lock(m_mutex);
        bool was_empty = m_timers.empty();
        auto [it, inserted] = m_timers.insert(timer);

        if (!inserted)
        {
            BLUE_LOG_ERROR(g_logger) << "addTimer: failed to insert timer";
            return nullptr;
        }

        bool at_front = (it == m_timers.begin());
        if (at_front)
        {
            m_tickle.store(true, std::memory_order_release);
        }

        lock.unlock();

        if (at_front || was_empty)
        {
            onTimerInsertedAtFront();
        }

        return timer;
    }

    // 辅助函数：条件定时器回调
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        auto cond = weak_cond.lock();
        if (cond && cb)
        {
            cb();
        }
    }

    Timer::TimerPtr TimerManager::addConditionTimer(uint64_t ms, std::coroutine_handle<> h,
                                                    std::function<void()> cb,
                                                    std::weak_ptr<void> weak_cond,
                                                    bool recurring)
    {
        // 包装回调，检查条件是否仍然有效
        auto wrapped_cb = std::bind(&OnTimer, std::move(weak_cond), std::move(cb));
        return addTimer(ms, h, std::move(wrapped_cb), recurring);
    }

    uint64_t TimerManager::getNextTime()
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (m_timers.empty())
        {
            return ~0ull;
        }

        auto now = Timer::SClock::now();
        const auto &first_timer = *m_timers.begin();

        if (!first_timer || !first_timer->isValid())
        {
            // 存在无效定时器（不应该发生），返回 0 让 processExpired 清理
            return 0;
        }

        if (now >= first_timer->m_expire)
        {
            return 0; // 已经过期，立即处理
        }

        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            first_timer->m_expire - now);

        return static_cast<uint64_t>(diff.count());
    }

    void TimerManager::processExpired()
    {
        auto now = Timer::SClock::now();
        std::vector<Timer::TimerPtr> expired_timers;

        {
            std::shared_lock<std::shared_mutex> read_lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }

        std::unique_lock<std::shared_mutex> write_lock(m_mutex);

        if (m_timers.empty())
        {
            return;
        }

        // 先收集所有过期的定时器（不修改原集合）
        std::vector<Timer::TimerPtr> to_process;

        for (auto it = m_timers.begin(); it != m_timers.end();)
        {
            auto timer = *it;

            if (!timer->isValid())
            {
                it = m_timers.erase(it);
                continue;
            }

            if (timer->m_expire <= now)
            {
                to_process.push_back(timer);

                if (timer->m_recurring)
                {
                    // 更新过期时间
                    timer->m_expire = now + std::chrono::milliseconds(timer->m_ms);
                    ++it;
                }
                else
                {
                    timer->m_valid.store(false, std::memory_order_release);
                    it = m_timers.erase(it);
                }
            }
            else
            {
                break; // 集合是有序的，后面的都不会过期
            }
        }

        write_lock.unlock();

        // 调度所有过期的定时器回调（不持有锁）
        for (auto &timer : to_process)
        {
            if (!timer->m_valid.load(std::memory_order_acquire))
            {
                continue;
            }
            std::function<void()> cb;
            std::coroutine_handle<> handle = nullptr;

            if (timer->m_cb)
            {
                cb = timer->m_cb;
            }
            if (timer->m_handle)
            {
                handle = timer->m_handle;
            }

            // 调度执行
            auto *scheduler = Scheduler::GetThisUnsafe();
            if (scheduler)
            {
                if (cb)
                {
                    scheduler->schedule(std::move(cb));
                }
                if (handle)
                {
                    scheduler->schedule(handle);
                }
            }
        }
    }

    bool TimerManager::hasTimer()
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return !m_timers.empty();
    }

    bool TimerManager::detectClockRollover(Timer::TimePoint now_time)
    {
        bool rollover = false;
        if (now_time < m_previousTime &&
            now_time < (m_previousTime - std::chrono::milliseconds(60 * 60 * 1000)))
        {
            rollover = true;
            BLUE_LOG_WARN(g_logger) << "Detected system clock rollback";
        }
        m_previousTime = now_time;
        return rollover;
    }

    void TimerManager::removeTimerLocked(const Timer::TimerPtr &timer)
    {
        auto it = m_timers.find(timer);
        if (it != m_timers.end())
        {
            m_timers.erase(it);
        }
    }
}
