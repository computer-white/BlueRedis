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
                
                /**
                 * @brief 重置
                 */
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
            int fd = -1;
            Event m_events = Event::NONE;
        };

        /**
         * @brief 构造
         * @param threads 线程数量
         */
        IOManager(size_t threads = 1);
        
        // 禁止拷贝和移动
        IOManager(const IOManager&) = delete;
        IOManager& operator=(const IOManager&) = delete;
        IOManager(IOManager&&) = delete;
        IOManager& operator=(IOManager&&) = delete;

        /**
         * @brief 析构
         * @note 停止调度器,关闭所有打开的句柄
         */
        ~IOManager() override;

        /**
         * @brief 往fd上注册event事件
         * @param fd 文件句柄
         * @param event 事件
         * @param h 协程句柄
         * @param cd 回调函数
         * @param thread_id 指定线程id执行
         */
        int addEvent(int fd, Event event, 
                     std::coroutine_handle<> h = nullptr, 
                     std::function<void()> cb = nullptr,
                     int thread_id = -1);
        
        /**
         * @brief 删除fd上event事件
         * @param fd 文件句柄
         * @param event 事件
         */
        bool delEvent(int fd, Event event);

        /**
         * @brief 取消fd上event事件
         * @param fd 文件句柄
         * @param event 事件
         */
        bool cancelEvent(int fd, Event event);

        /**
         * @brief 取消epoll上注册的fd所有事件
         * @param fd 需要取消的事件
         */
        bool cancelAll(int fd);

        /**
         * @brief 等待iomanager任务结束
         */
        void wait_all() override;
        
        /**
         * @brief 获取iomanager裸指针
         * @note 用于在iomanager之外的地方调用内部非静态非私有函数
         */
        static IOManager* GetThis();

    private:

        /**
         * @brief 名字沿用sylar项目,这里就是额外分配一个线程在idle上处理epoll事件
         * @brief 属于Scheduler::run()的一个上层战场
         * @note 之所以分配一个线程在idle上有因为我认为他只是在处理epoll事件并往scheduler上提交任务
         */
        void idle();

    protected:

        /**
         * @brief 用于唤醒epoll_wait
         */
        void tickle() override;

        /**
         * @brief 检测是否需要停止调度
         */
        bool stopping() override;

        /**
         * @brief 当有新的定时器插入到定时器集合首部时调用
         * @note 用于唤醒 epoll_wait 重新计算超时时间
         */
        void onTimerInsertedAtFront() override;

    private:
        mutable std::shared_mutex m_Iommutex;                               // 互斥变量
        int m_epfd = -1;                                                    // epoll句柄
        int m_eventfd = -1;                                                 // 使用 eventfd,来唤醒epoll_wait                    
        std::atomic<size_t> m_pendingEventCounts{0};                        // epoll事件计数
        std::unordered_map<int, std::unique_ptr<FdContext>> m_fdContexts;   // 句柄字典
        
        static constexpr int MAX_EPOLL_EVENTS = 64;                         // 最大epoll事件数量
        static constexpr int MAX_WAIT_TIMEOUT = 3000;                       // 最大超时时间
    };
}