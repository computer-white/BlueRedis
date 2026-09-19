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
 * @file shceduler.h
 * @brief 调度器模块(重新实现了调度器模块为了配合c++20协程)
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.1
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <random>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <list>
#include "blue/task.h"
#include "blue/mthread.h"

namespace blue
{
    class IOManager;

    class Scheduler
    {
    public:
        using SchedulerPtr = std::shared_ptr<Scheduler>;

    protected:
        struct FuncAndId
        {
            std::function<void()> cb = nullptr; // 回调函数
            int threadId = -1;                  // 线程id,被指定执行cb的线程id

            FuncAndId() = default;
            FuncAndId(std::function<void()> cb, int thr)
                : cb(std::move(cb)), threadId(thr) {}
        };

        struct alignas(64) PerThreadQueue
        {
            friend class Scheduler;

        private:
            std::deque<FuncAndId> tasks; // 线程局部任务队列

        public:
            std::mutex mutex;               // 互斥变量
            std::condition_variable cv;     // 条件变量
            std::atomic<size_t> pending{0}; // 局部工作队列的计数器
        };

    protected:
        /**
         * @brief 用于iomanager唤醒epoll
         */
        virtual void tickle();

        /**
         * @brief 是否需要停止
         */
        virtual bool stopping();

        /**
         * @brief 任务拿取、执行的主要函数
         */
        virtual void run();

    public:
        /**
         * @brief 构造调度器对象
         * @param threads 线程数量
         * @param name 调度器名称
         */
        Scheduler(size_t threads = 1);

        /**
         * @brief 如果调度器没有停止,则析构来调用stop
         */
        virtual ~Scheduler();

        /**
         * @brief 开启调度器
         */
        virtual void start();

        /**
         * @brief 停止调度器
         */
        virtual void stop();

        /**
         * @brief 等待所有任务结束
         */
        virtual void wait_all();

        /**
         * @brief 提交一组协程任务
         * @param thr 指定工作线程
         */
        template <typename... Args>
        void scheduleMul(int thr, Args &&...args)
        {
            (schedule(std::forward<Args>(args), thr), ...);
        }

        /**
         * @brief 提交协程任务
         * @param task 一个返回Task<T>的函数,即协程
         * @param thr 指定工作线程
         */
        template <typename T>
        void schedule(Task<T> task, int thr = -1)
        {
            auto h = task.getHandleNoType(); // 保存裸句柄用于 lambda

            auto task_base = blue::makeTaskBase<T>(std::move(task));
            {
                std::lock_guard<std::mutex> lock(m_runningMutex);
                m_runningTasks.push_back(task_base);
            }

            schedule([task_base, h]() mutable
            {
                if (h && h.address() && !h.done())
                {
                    h.resume();
                }
                // 统一在 wait_all 里清理。
            }, thr);
        }

        /**
         * @brief 提交函数任务
         * @param cb 包装好的任务
         * @param thr 指定工作线程
         */
        void schedule(std::function<void()> cb, int thr = -1);

        /**
         * @brief 提交协程句柄
         * @param h 协程句柄
         * @param thr 指定工作线程
         */
        void schedule(std::coroutine_handle<> h, int thr = -1);

        /**
         * @brief 任务窃取
         * @param task 引用传递,可以拿到窃取到的任务
         * @param max_attempts 最大尝试窃取次数
         */
        bool stealTask(FuncAndId &task, int max_attempts = 3);

        /**
         * @brief m_running + val
         * @note 即将有一个正在运行的协程(类似go的WaitGroup 的 Add())
         */
        void runAdd(int val = 1) noexcept { m_running.fetch_add(val, std::memory_order_acq_rel); }

        /**
         * @brief m_running - val
         */
        void runSub(int val = 1) noexcept { m_running.fetch_sub(val, std::memory_order_acq_rel); }

        /**
         * @brief 清理已完成的顶层 Task（在 wait_all 里调用）
         */
        void clearFinishedTasks()
        {
            std::lock_guard<std::mutex> lock(m_runningMutex);
            m_runningTasks.remove_if([](const std::shared_ptr<blue::TaskBase> &t)
                                     { return t->done(); });
        }

    public:
        /**
         * @brief 设置调度器指针
         */
        static void setThis(Scheduler *t);

        /**
         * @brief 获取调度器指针(保证一定不为空)
         */
        static Scheduler *GetThis();

        /**
         * @brief 获取当前线程索引
         */
        static int GetThreadIndex();

        /**
         * @brief 获取线程数量
         */
        static int GetThreadCount();

        /**
         * @brief 设置线程索引
         * @param index 线程索引
         */
        static void setThreadIndex(int index) { t_threadIndex = index; }

    private:
        /**
         * @brief 执行完当前线程的本地任务队列
         * @param index 线程索引
         * @note 内部还是提交给全局调度器去调度.防止一些任务时间很长拖慢当前线程
         */
        void drainLocalQueue(int index);

    protected:
        // iomanager需要使用
        std::mutex m_doneMutex;                                      // wait_all用
        std::condition_variable m_doneCv;                            // wait_all用
        std::atomic<size_t> m_waiting{0};                            // 记录正在等 m_Schecv 的线程数，用于精准 notify
        std::vector<std::unique_ptr<blue::Mthread>> m_workers;       // 工作线程
        std::vector<std::unique_ptr<PerThreadQueue>> m_threadQueues; // 工作线程任务队列
        std::atomic<size_t> m_pending{0};                            // 全局队列任务计数
        std::atomic<size_t> m_running{0};                            // 有任务在运行计数
        std::atomic<bool> m_stopping{false};                         // 是否主动停止调度器

    private:
        size_t m_threadCount;             // 线程数量
        std::atomic<bool> m_stop{true};   // 调度器处于停止状态或开启状态
        std::mutex m_Schemutex;           // 互斥变量
        std::condition_variable m_Schecv; // 条件变量
        std::deque<FuncAndId> m_queue;    // 全局任务队列

        std::mutex m_runningMutex;
        std::list<std::shared_ptr<blue::TaskBase>> m_runningTasks; // 持有顶层 Task 直到完成/清理
    private:
        static thread_local Scheduler *t_Scheduler; // 线程局部调度器指针
        static thread_local int t_threadIndex;      // 线程索引,切换线程队列
    };
}