#include "newblue/timer.h"
#include "newblue/scheduler.h"

namespace newblue
{
    bool Timer::Comparator::operator()(const Timer::TimerPtr &lhs, const Timer::TimerPtr &rhs) const
    {
        if (lhs == nullptr || rhs == nullptr)
        {
            if (lhs == nullptr && rhs == nullptr)
            {
                return false;
            }
            return lhs == nullptr ? true : false;
        }
        if (lhs->m_expire == rhs->m_expire)
        {
            return lhs.get() < rhs.get();
        }
        return lhs->m_expire < rhs->m_expire;
    }

    Timer::Timer(uint64_t ms, std::coroutine_handle<> h, 
            std::function<void()> cb, bool recurring, TimerManager *manager)
            : m_recurring(recurring), m_ms(ms), m_cb(cb), m_handle(h),
            m_managger(manager)
    {
        m_expire = Timer::SClock::now() + std::chrono::milliseconds(ms);
    }

    Timer::Timer(TimePoint expire)
    {
        m_expire = expire;
    }

    std::shared_ptr<Timer> Timer::Create_by_next(TimePoint expire)
    {
        return std::shared_ptr<Timer>(new Timer(expire));
    }

    std::shared_ptr<Timer> Timer::Create_by_ms(uint64_t ms, std::coroutine_handle<> h,
        std::function<void()> cb, bool recurring, TimerManager *manager)
    {
        return std::shared_ptr<Timer>(new Timer(ms,h,cb,recurring,manager));
    }

    bool Timer::cancel()
    {
        {
            std::shared_lock<std::shared_mutex> lock(m_managger->m_mutex);
            if (!m_cb)
            {
                return false;
            }
        }
        std::unique_lock<std::shared_mutex> lock(m_managger->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        m_cb = nullptr;
        auto it  = m_managger->m_timers.find(shared_from_this());
        m_managger->m_timers.erase(it);
        return true;
    }


    bool Timer::reset(uint64_t ms, bool from_now)
    {
        {
            std::shared_lock<std::shared_mutex> lock(m_managger->m_mutex);
            if (!m_cb)
            {
                return false;
            }
        }
        std::unique_lock<std::shared_mutex> lock(m_managger->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_managger->m_timers.find(shared_from_this());
        if (it == m_managger->m_timers.end())
        {
            return false;
        }
        m_managger->m_timers.erase(it);
        TimePoint start;
        if (from_now)
        {
            start = SClock::now();
        }
        else
        {
            start = m_expire - std::chrono::milliseconds(m_ms);
        }
        if (ms != -1)
        {
            m_ms = ms;
        }
        m_expire = start + std::chrono::milliseconds(m_ms);
        it = m_managger->m_timers.insert(shared_from_this()).first;
        bool at_front = (it == m_managger->m_timers.begin());
        if (at_front)
        {
            m_managger->m_tickle = true;
        }
        lock.unlock();
        if (at_front)
        {
            m_managger->onTimerInsertedAtFront();
        }
        return true;
    }

    TimerManager::TimerManager()
    {
        m_previousTime = std::chrono::steady_clock::now();
    }

    Timer::TimerPtr TimerManager::addTimer(uint64_t ms, std::coroutine_handle<> h, std::function<void()> cb, bool recurring)
    {
        Timer::TimerPtr timer = Timer::Create_by_ms(ms,h,cb,recurring,this);
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin());
        if (at_front)
        {
            m_tickle = true;
        }
        lock.unlock();
        if (at_front)
        {
            onTimerInsertedAtFront();
        }
        return timer;
    }

    // 用于addConditionTimer的辅助函数
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tem = weak_cond.lock();
        if (tem && cb)
        {
            cb();
        }
    }

    Timer::TimerPtr TimerManager::addConditionTimer(uint64_t ms, std::coroutine_handle<> h, std::function<void()> cb, 
    std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms,h,std::bind(&OnTimer,weak_cond,cb),recurring);
    }
    uint64_t TimerManager::getNextTime()
    {
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_tickle = false;
        }
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (m_timers.empty())
        {
            return ~0ull;
        }
        auto now = Timer::SClock::now();
        const Timer::TimerPtr &expire = *m_timers.begin();
        if (now >= expire->m_expire)
        {
            return 0;
        }
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(expire->m_expire - now);
        return diff.count();
    }

    void TimerManager::processExpired()
    {
        auto now = Timer::SClock::now();
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
        bool rollover = detectClockRollover(now);
        if (!rollover && (*m_timers.begin())->m_expire > now)
        {
            return;
        }
        Timer::TimerPtr now_timer = Timer::Create_by_next(now);
        auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
        std::vector<Timer::TimerPtr> expired;
        expired.insert(expired.begin(),m_timers.begin(),it);
        m_timers.erase(m_timers.begin(),it);
        for (auto &it : expired)
        {
            if (it->m_cb)
            {
                auto cb = it->m_cb;
                Scheduler::GetThis()->schedule(cb);
            }
            if (it->m_handle)
            {
                Scheduler::GetThis()->schedule(it->m_handle);;
            }
            if (it->m_recurring)
            {
                it->m_expire = now + std::chrono::milliseconds(it->m_ms);
                m_timers.insert(it);
            }
            else
            {
                it->m_cb = nullptr;
                it->m_handle= nullptr;
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
        }
        m_previousTime = now_time;
        return rollover;
    }
}