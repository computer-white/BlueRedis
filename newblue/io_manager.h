#pragma once
#include <sys/epoll.h>
#include <shared_mutex>
#include "scheduler.h"
#include "newblue/timer.h"
namespace newblue
{

    class IOManager : public Scheduler, public TimerManager
    {
    public:
        enum Event
        {
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x4
        };
        // 句柄及对应的事件
        struct FdContext
        {
            // 事件
            struct EventContext
            {
                Scheduler *scheduler = nullptr;     // 事件的schedular
                std::coroutine_handle<> handle = nullptr;
                std::function<void()> cb = nullptr; // 事件回调函数
            };

            /**
             * @brief 获取任务对应的事件内容
             * @param event 事件
             * @return 任务对应的事件内容
             */
            EventContext &getEventContext(Event event);

            /**
             * @brief 清空任务
             * @param ec 事件内容的左值引用,清空这个事件内容
             * @return
             */
            void resetEventContext(EventContext &ec);

            /**
             * @brief 删除m_events中的event,并提交event事件对应的任务
             * @return
             */
            void triggerContext(Event event);

            std::mutex mutex;
            EventContext read;            // 读事件
            EventContext write;           // 写事件
            int fd = 0;                   // 事件关联的句柄
            Event m_events = Event::NONE; // 已注册的事件
        };
        IOManager(size_t threads = 1);

        /* 禁止拷贝和移动 */
        IOManager(const IOManager &) = delete;
        IOManager &operator=(const IOManager &) = delete;
        IOManager(IOManager &&) = delete;
        IOManager &operator=(IOManager &&) = delete;
        
        ~IOManager();

        // 注册事件：fd 可读/可写时恢复协程
        int addEvent(int fd, Event event, std::coroutine_handle<> h, std::function<void()> cb);

        bool delEvent(int fd, Event event);

        bool cancelEvent(int fd, Event event);

        bool cancelAll(int fd);

        virtual void wait_all() override; // 等待所有任务完成
        /**
         * @brief 设置IOManager指针
         * @return OManager指针
         */
        static IOManager *GetThis();

    private:
        void idle(); // epoll_wait 循环

    protected:
        virtual void tickle() override;   // 唤醒一个线程
        virtual bool stopping() override;
        virtual void onTimerInsertedAtFront() override;

    private:
        std::shared_mutex m_mutex;
        std::condition_variable_any m_cv;
        int m_epfd;
        int m_ticklefds[2];
        std::atomic<size_t> m_pendingEventCounts = {0}; // 等待执行的任务计数
        std::unordered_map<int, FdContext*> m_fdContexts;
    };

} // namespace newblue