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

namespace newblue
{

    class Scheduler
    {
    public:
        using SchedulerPtr = std::shared_ptr<Scheduler>;

    public:
        Scheduler(size_t threads = 1, const std::string &name = "");
        ~Scheduler();

        void start();
        void stop();
        virtual void wait_all(); // 等待所有任务完成
        
        const std::string &getName() const { return m_name; }
        void schedule(Task task, int thr = -1);
        void schedule(std::function<void()> cb, int thr = -1);
        void schedule(std::coroutine_handle<> h);
        static void setThis(Scheduler* t);
        static Scheduler *GetThis();

    private:
        void run(); // 工作线程主循环
    protected:
        virtual void tickle();   // 唤醒一个线程
        virtual bool stopping();

    private:
        struct FuncAndId
        {
            std::function<void()> cb = nullptr;
            int threadId = -1;

            FuncAndId(std::function<void()> cb, int thr)
                : cb(std::move(cb)), threadId(thr) {}
        };

    private:
        std::mutex m_mutex;
        int m_gc_counter = 0;
        std::condition_variable m_cv;
        std::queue<FuncAndId> m_queue; // 统一用回调队列
        std::vector<std::shared_ptr<Task>> m_tasks;
        std::vector<std::thread> m_workers;
        std::string m_name;

    protected:
        std::atomic<size_t> m_pending{0};
        std::atomic<bool> m_stop{true};
        size_t m_threadCount;
    };

}
