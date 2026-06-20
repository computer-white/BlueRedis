#pragma once
#include <set>
#include <functional>
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <coroutine>
#include "newblue/task.h"

namespace newblue
{
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;

    public:
        using TimerPtr = std::shared_ptr<Timer>;
        using SClock = std::chrono::steady_clock;
        using TimePoint = SClock::time_point;

    public:
        /**
         * @brief 取消定时任务
         * @return 成功返回 true 失败 false
         */
        bool cancel();

        /**
         * @brief 重置定时器
         * @param ms 定时器循环周期，默认使用上次设置的循环周期
         * @param from_now 是否从当前定时，默认从当前开始
         */
        bool reset(uint64_t ms = -1, bool from_now = true);

    private:
        /**
         * @brief 构造函数
         * @param ms 定时器循环周期
         * @param h 被挂起的协程
         * @param cb 定时回调函数
         * @param recurring 是否设为循环定时器
         * @param manager TimerManager 指针
         * @return
         * @note 不允许隐式转换
         */
        explicit Timer(uint64_t ms, std::coroutine_handle<> h, std::function<void()> cb, bool recurring, TimerManager *manager);

        /**
         * @brief 构造函数
         * @param expire 定时器过期时间点
         * @return
         * @note 不允许隐式转换
         */
        explicit Timer(TimePoint expire);

    public:
        Timer() = default;

        /* 禁止拷贝和移动 */
        Timer(const Timer &) = delete;
        Timer &operator=(const Timer &) = delete;
        Timer(Timer &&) = delete;
        Timer &operator=(Timer &&) = delete;

        /**
         * @brief 按照过期时间创建定时器
         * @param expire 定时器过期时间点
         * @return 定时器智能指针
         */
        static std::shared_ptr<Timer> Create_by_next(TimePoint expire);

        /**
         * @brief 创建定时器
         * @param ms 定时器循环周期
         * @param h 被挂起的协程
         * @param cb 定时回调函数
         * @param recurring 是否设为循环定时器
         * @param manager TimerManager 指针
         * @return 定时器智能指针
         */
        static std::shared_ptr<Timer> Create_by_ms(uint64_t ms, std::coroutine_handle<> h,
                                                   std::function<void()> cb, bool recurring, TimerManager *manager);

    private:
        bool m_recurring = false;           // 是否循环定时
        uint64_t m_ms = 0;                  // 定时器循环周期
        TimePoint m_expire;                 // 定时器过期时间点
        std::coroutine_handle<> m_handle;   // 被挂起的协程
        std::function<void()> m_cb;         // 定时器任务
        TimerManager *m_managger = nullptr; // 定时器管理指针

    private:
        /**
         * @brief 定时器比较器
         * @return
         */
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

    public:
        /**
         * @brief TimerManager构造函数
         * @return
         */
        TimerManager();

        /* 禁止拷贝和移动 */
        TimerManager(const TimerManager &) = delete;
        TimerManager &operator=(const TimerManager &) = delete;
        TimerManager(TimerManager &&) = delete;
        TimerManager &operator=(TimerManager &&) = delete;
        virtual ~TimerManager() = default;

        /**
         * @brief 添加定时器
         * @param ms 定时器循环周期
         * @param h 被挂起的协程,ms后恢复协程
         * @param cb 定时回调函数
         * @param recurring 是否设为循环定时器
         * @return
         */
        Timer::TimerPtr addTimer(uint64_t ms, std::coroutine_handle<> h, std::function<void()> cb, bool recurring = false);

        /**
         * @brief 添加条件定时器
         * @param ms 定时器循环周期
         * @param h 被挂起的协程,ms后恢复协程
         * @param cb 定时回调函数
         * @param weak_cond 条件
         * @param recurring 是否设为循环定时器
         * @return
         */
        Timer::TimerPtr addConditionTimer(uint64_t ms, std::coroutine_handle<> h, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

        /**
         * @brief 获取下一次任务的执行时间
         * @return 下一次任务的执行时间
         */
        uint64_t getNextTime();

        /**
         * @brief 执行过期的任务
         * @return
         */
        void processExpired();

        /**
         * @brief 是否有定时器
         * @return 有返回true 否则 false
         */
        bool hasTimer();

    protected:
        /**
         * @brief 当某一任务被 新建，重置，刷新后,时间最小,那么需要调用此函数用于唤醒epoll_wait
         * @return
         */
        virtual void onTimerInsertedAtFront() = 0;

    private:
        /**
         * @brief 当系统时间被修改后需要做出调整
         * @param noe 当前时间
         * @return 修改了返回true 否则 false
         */
        bool detectClockRollover(Timer::TimePoint now);

    private:
        std::shared_mutex m_mutex;                             // 互斥变量
        std::set<Timer::TimerPtr, Timer::Comparator> m_timers; // 定时器集合
        bool m_tickle = false;                                 // fasle:防止频繁调用onTimerInsertedAtFront()
        Timer::TimePoint m_previousTime;                       // 系统上一次的时间
    };
}