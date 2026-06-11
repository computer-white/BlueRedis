#pragma once
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
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

    public:
        Scheduler(size_t threads = 1, const std::string &name = "");
        virtual ~Scheduler();

        virtual void start();
        virtual void stop();
        virtual void wait_all();
        
        const std::string &getName() const { return m_name; }
        
        static int GetThreadIndex();
        static int GetThreadCount();
        static void setThreadIndex(int index) { t_threadIndex = index; }
        template <typename T>
        void schedule(Task<T> task, int thr = -1)
        {
            auto task_holder = std::make_shared<Task<T>>(std::move(task));
            if (!(*task_holder) || task_holder->done()) return;
            
            schedule([task_holder]() mutable {
                if (*task_holder && !task_holder->done())
                {
                    task_holder->resume();
                }
            }, thr);
        }
        
        void schedule(std::function<void()> cb, int thr = -1);
        void schedule(std::coroutine_handle<> h, int thr = -1);
        
        static void setThis(Scheduler* t);
        static Scheduler *GetThis();
        static Scheduler* GetThisUnsafe() { return t_Scheduler; }

    protected:
        virtual void tickle();
        virtual bool stopping();
        virtual void run();
        
        void drainLocalQueue(size_t index);

    protected:
        struct FuncAndId
        {
            std::function<void()> cb = nullptr;
            int threadId = -1;

            FuncAndId() = default;
            FuncAndId(std::function<void()> cb, int thr)
                : cb(std::move(cb)), threadId(thr) {}
        };

        struct PerThreadQueue
        {
            friend class Scheduler;
        private:
            std::queue<FuncAndId> tasks;
        public:
            std::mutex mutex;
            std::condition_variable cv;
            std::atomic<size_t> pending{0};
        };

    protected:
        std::mutex m_Schemutex;
        std::condition_variable m_Schecv;
        std::vector<std::thread> m_workers;
        std::vector<std::unique_ptr<PerThreadQueue>> m_threadQueues;
        std::atomic<size_t> m_pending{0};        // 全局队列任务计数
        std::atomic<bool> m_stopping{false};
    private:
        std::string m_name;
        size_t m_threadCount;
        std::atomic<bool> m_stop{true};
        std::queue<FuncAndId> m_queue;
        
    private:
        static thread_local Scheduler* t_Scheduler;
        static thread_local int t_threadIndex;
    };
}