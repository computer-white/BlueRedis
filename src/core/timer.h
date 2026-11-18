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
 * @file timer.h
 * @brief 定时器模块,基本结构还是跟sylar的一样，只是内部处理细节上有些差异，还有一些api我认为不合理或夯杂的就没再采用
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.1
 * @copyright Copyright (c) 2026年 blue
 */
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

    private:
        explicit Timer(uint64_t ms, std::coroutine_handle<> h, 
                      std::function<void()> cb, int thr, bool recurring, TimerManager *manager);
        explicit Timer(TimePoint expire);

    public:
        Timer() = default;

        Timer(const Timer &) = delete;
        Timer &operator=(const Timer &) = delete;
        Timer(Timer &&) = delete;
        Timer &operator=(Timer &&) = delete;

        static std::shared_ptr<Timer> Create_by_next(TimePoint expire);
        static std::shared_ptr<Timer> Create_by_ms(uint64_t ms, std::coroutine_handle<> h,
                                                   std::function<void()> cb, 
                                                   int thr,
                                                   bool recurring, 
                                                   TimerManager *manager);

    private:
        bool m_recurring = false;           // 是否是循环定时器
        uint64_t m_ms = 0;                  // 定时周期
        int m_thr = -1;                     // 指定线程
        TimePoint m_expire;                 // 过期时间点
        std::coroutine_handle<> m_handle;   // 协程句柄
        std::function<void()> m_cb;         // CallBack函数
        TimerManager *m_manager = nullptr;  // Timer管理者指针

        // 自定义比较器
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

        /**
         * @brief 添加一个定时器
         * @param ms 定时器时长
         * @param h 协程句柄
         * @param cb CallBack 函数
         * @param thr 指定线程
         * @param recurring 是否循环定时器
         * @note thr -1 表示优先提交到当前线程队列，同时会被其他线程窃取。也就是 thr不为-1，就不会被窃取
         */
        Timer::TimerPtr addTimer(uint64_t ms, std::coroutine_handle<> h, 
                                 std::function<void()> cb, int thr = -1, bool recurring = false);

        /**
         * @brief 添加一个条件定时器
         * @param ms 定时器时长
         * @param h 协程句柄
         * @param cb CallBack 函数
         * @param thr 指定线程
         * @param weak_cond 条件（后续需要更明确的修复这块）
         * @param recurring 是否循环定时器
         * @note thr -1 表示优先提交到当前线程队列，同时会被其他线程窃取。也就是 thr不为-1，就不会被窃取
         */
        Timer::TimerPtr addConditionTimer(uint64_t ms, std::coroutine_handle<> h, 
                                          std::function<void()> cb, 
                                          int thr,
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

        /**
         * @brief 清空所有定时器
         */
        void clear()
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (auto& timer : m_timers)
            {
                if (timer)
                {
                    timer->m_handle = nullptr;
                    timer->m_cb = nullptr;
                }
            }
            m_timers.clear();
        }

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