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
#include "task.h"

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

        struct PerThreadQueue
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
        Scheduler(size_t threads = 1, const std::string &name = "");

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
         * @brief 获取调度器名称
         */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 提交协程任务
         * @param task 一个返回Task<T>的函数,即协程
         * @param thr 指定工作线程
         */
        template <typename T>
        void schedule(Task<T> task, int thr = -1)
        {
            auto task_holder = std::make_shared<Task<T>>(std::move(task));
            if (!(*task_holder) || task_holder->done())
                return;

            schedule([task_holder]() mutable
                     {
                if (*task_holder && !task_holder->done())
                {
                    task_holder->resume();
                } }, thr);
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
         * @brief 设置调度器指针
         */
        static void setThis(Scheduler *t);

        /**
         * @brief 获取调度器指针(保证一定不为空)
         */
        static Scheduler *GetThis();

        /**
         * @brief 获取不安全调度器指针(可能没有初始化)
         */
        static Scheduler *GetThisUnsafe() { return t_Scheduler; }

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
        void drainLocalQueue(size_t index);

    protected:
        std::mutex m_Schemutex;                                      // 互斥变量
        std::condition_variable m_Schecv;                            // 条件变量
        std::vector<std::thread> m_workers;                          // 工作线程
        std::vector<std::unique_ptr<PerThreadQueue>> m_threadQueues; // 工作线程任务队列
        std::atomic<size_t> m_pending{0};                            // 全局队列任务计数
        std::atomic<size_t> m_running{0};                            // 有任务在运行计数
        std::atomic<bool> m_stopping{false};                         // 是否主动停止调度器

    private:
        std::string m_name;             // 调度器名称
        size_t m_threadCount;           // 线程数量
        std::atomic<bool> m_stop{true}; // 调度器处于停止状态或开启状态
        std::deque<FuncAndId> m_queue;  // 全局任务队列

        std::mt19937 m_rng{std::random_device{}()};         // 任务窃取随机因子
        std::uniform_int_distribution<> m_steal_dist{0, 0}; // 随机值产生器

    private:
        static thread_local Scheduler *t_Scheduler; // 线程局部调度器指针
        static thread_local int t_threadIndex;      // 线程索引,切换线程队列
    };
}