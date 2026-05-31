#pragma once
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include "scheduler.h"
#include "timer.h"

namespace blue
{
    class IOManager : public Scheduler, public TimerManager
    {
        friend class Scheduler;
        
    public:
        enum Event : uint32_t
        {
            NONE  = 0x0,
            READ  = 0x1,
            WRITE = 0x4
        };
        
        struct FdContext
        {
            struct EventContext
            {
                Scheduler* scheduler = nullptr;
                std::coroutine_handle<> handle = nullptr;
                std::function<void()> cb = nullptr;
                int thread_id = -1;
                
                void reset()
                {
                    scheduler = nullptr;
                    handle = nullptr;
                    cb = nullptr;
                    thread_id = -1;
                }
            };

            EventContext& getEventContext(Event event);
            void resetEventContext(EventContext& ec);
            void triggerContext(Event event);

            std::mutex mutex;
            EventContext read;
            EventContext write;
            int fd = 0;
            Event m_events = Event::NONE;
            bool m_closing = false;
        };

        IOManager(size_t threads = 1);
        
        IOManager(const IOManager&) = delete;
        IOManager& operator=(const IOManager&) = delete;
        IOManager(IOManager&&) = delete;
        IOManager& operator=(IOManager&&) = delete;

        ~IOManager() override;

        int addEvent(int fd, Event event, 
                     std::coroutine_handle<> h = nullptr, 
                     std::function<void()> cb = nullptr,
                     int thread_id = -1);
        
        bool delEvent(int fd, Event event);

        bool cancelEvent(int fd, Event event);

        bool cancelAll(int fd);

        void wait_all() override;
        
        static IOManager* GetThis();

    private:
        void idle();

    protected:
        void tickle() override;
        bool stopping() override;
        void onTimerInsertedAtFront() override;

    private:
        mutable std::shared_mutex m_Iommutex;
        int m_epfd = -1;
        int m_eventfd = -1;  // 使用 eventfd 替代 pipe
        std::atomic<size_t> m_pendingEventCounts{0};
        std::unordered_map<int, std::unique_ptr<FdContext>> m_fdContexts;
        
        static constexpr int MAX_EPOLL_EVENTS = 64;
        static constexpr int MAX_WAIT_TIMEOUT = 3000;
    };
}