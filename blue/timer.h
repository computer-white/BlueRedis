#pragma once
#include <set>
#include <functional>
#include <memory>
#include <cstdint>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <coroutine>
#include <atomic>

namespace blue
{
    class TimerManager;
    class Scheduler;  // 前向声明
    
    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;

    public:
        using TimerPtr = std::shared_ptr<Timer>;
        using SClock = std::chrono::steady_clock;
        using TimePoint = SClock::time_point;

        /**
         * @brief 取消定时任务
         * @return 成功返回 true，失败或已取消返回 false
         */
        bool cancel();

        /**
         * @brief 重置定时器
         * @param ms 定时器循环周期，-1 表示使用上次设置的周期
         * @param from_now 是否从当前时间开始计算
         */
        bool reset(uint64_t ms = static_cast<uint64_t>(-1), bool from_now = true);

        /**
         * @brief 获取过期时间
         */
        TimePoint getExpire() const { return m_expire; }

        /**
         * @brief 检查定时器是否有效
         */
        bool isValid() const { return m_valid.load(std::memory_order_acquire); }

    private:
        explicit Timer(uint64_t ms, std::coroutine_handle<> h, 
                      std::function<void()> cb, bool recurring, TimerManager *manager);
        explicit Timer(TimePoint expire);

    public:
        Timer() = default;

        Timer(const Timer &) = delete;
        Timer &operator=(const Timer &) = delete;
        Timer(Timer &&) = delete;
        Timer &operator=(Timer &&) = delete;

        static std::shared_ptr<Timer> Create_by_next(TimePoint expire);
        static std::shared_ptr<Timer> Create_by_ms(uint64_t ms, std::coroutine_handle<> h,
                                                   std::function<void()> cb, bool recurring, 
                                                   TimerManager *manager);

    private:
        bool m_recurring = false;           
        uint64_t m_ms = 0;                  
        TimePoint m_expire;                 
        std::coroutine_handle<> m_handle;   
        std::function<void()> m_cb;         
        TimerManager *m_manager = nullptr;
        std::atomic<bool> m_valid{true};    // 原子标志，标记定时器是否有效

        struct Comparator
        {
            bool operator()(const Timer::TimerPtr &lhs, const Timer::TimerPtr &rhs) const;
        };
    };

    class TimerManager
    {
        friend class Timer;

    public:
        using TimerManagerPtr = std::shared_ptr<TimerManager>;

        TimerManager();

        TimerManager(const TimerManager &) = delete;
        TimerManager &operator=(const TimerManager &) = delete;
        TimerManager(TimerManager &&) = delete;
        TimerManager &operator=(TimerManager &&) = delete;
        virtual ~TimerManager() = default;

        Timer::TimerPtr addTimer(uint64_t ms, std::coroutine_handle<> h, 
                                 std::function<void()> cb, bool recurring = false);

        Timer::TimerPtr addConditionTimer(uint64_t ms, std::coroutine_handle<> h, 
                                          std::function<void()> cb, 
                                          std::weak_ptr<void> weak_cond, 
                                          bool recurring = false);

        /**
         * @brief 获取到下一个定时器超时的毫秒数
         * @return 超时毫秒数，如果没有定时器返回 ~0ull
         */
        uint64_t getNextTime();

        /**
         * @brief 处理所有过期的定时器
         */
        void processExpired();

        /**
         * @brief 是否有活动的定时器
         */
        bool hasTimer();

    protected:
        /**
         * @brief 当有新的定时器插入到队首时调用
         * @note 用于唤醒 epoll_wait 重新计算超时时间
         */
        virtual void onTimerInsertedAtFront() = 0;

    private:
        bool detectClockRollover(Timer::TimePoint now);

        /**
         * @brief 从集合中移除定时器（内部使用，调用前需持有写锁）
         */
        void removeTimerLocked(const Timer::TimerPtr& timer);

    private:
        mutable std::shared_mutex m_mutex;
        std::set<Timer::TimerPtr, Timer::Comparator> m_timers;
        std::atomic<bool> m_tickle{false};
        Timer::TimePoint m_previousTime;
    };
}