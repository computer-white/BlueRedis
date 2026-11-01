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
 * @file io_manager.h
 * @brief io操作管理,使用epoll检测fd上的事件,然后提交给scheduler,目前主要提交协程或基于协程封装的回调函数
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.5.1
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include "src/core/resume_ticket.h"
#include "src/core/scheduler.h"
#include "src/core/timer.h"

namespace blue
{
    class IOManager : public Scheduler, public TimerManager
    {
        friend class Scheduler;

    public:
        enum Event : uint32_t
        {
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x4
        };

        struct FdContext
        {
            struct EventContext
            {
                Scheduler *scheduler = nullptr;
                std::coroutine_handle<> handle = nullptr;
                std::shared_ptr<blue::ResumeTicket> resumeTicket;
                std::function<void()> cb = nullptr;
                int thread_id = -1;

                /**
                 * @brief 重置
                 */
                void reset()
                {
                    scheduler = nullptr;
                    // 注意：handle 不 destroy。
                    // 协程帧的所有权属于 Task，IOManager 只临时持有引用用于恢复。
                    // 帧的销毁由 Task::destroySafe() 负责。
                    // if (handle && handle.address())
                    // {
                    //     handle.destroy();
                    // }
                    handle = nullptr;
                    resumeTicket = nullptr;
                    cb = nullptr;
                    thread_id = -1;
                }
            };

            EventContext &getEventContext(Event event);
            void resetEventContext(EventContext &ec);
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
        IOManager(const IOManager &) = delete;
        IOManager &operator=(const IOManager &) = delete;
        IOManager(IOManager &&) = delete;
        IOManager &operator=(IOManager &&) = delete;

        /**
         * @brief 析构
         * @note 停止调度器,关闭所有打开的句柄
         */
        ~IOManager() override;

        /**
         * @brief 往fd上注册event事件
         * @param fd 文件句柄
         * @param event 事件
         * @param rt 更安全的协程句柄包装
         * @param cd 回调函数
         * @param thread_id 指定线程id执行
         * @note
         */
        int addEvent(int fd, Event event,
                     std::shared_ptr<blue::ResumeTicket> rt,
                     std::function<void()> cb = nullptr,
                     int thread_id = -1);

        /**
         * @brief 往fd上注册event事件
         * @param fd 文件句柄
         * @param event 事件
         * @param h 协程句柄
         * @param cd 回调函数
         * @param thread_id 指定线程id执行
         * @warning 若直接使用提交裸句柄，需要调用方保证：
         * 1. 该句柄仅被 IOManager 持有，而不同时被调度器或定时器持有
         * 若无法保证，请使用 ResumeTicket
         * eg:
         *      auto resumeTicket = std::make_shared<blue::ResumeTicket>();
         *      resumeTicket->m_handle = h;
         *      resumeTicket->m_armed.store(true, std::memory_order_release);
         *      int tem = iom->addEvent(fd, event, resumeTicket, nullptr);
         */
        int addEvent(int fd, Event event,
                     std::coroutine_handle<> h = nullptr,
                     std::function<void()> cb = nullptr,
                     int thread_id = -1);

        /**
         * @brief 供测试使用
         */
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

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
        static IOManager *GetThis();

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
        mutable std::shared_mutex m_Iommutex;                             // 互斥变量
        int m_epfd = -1;                                                  // epoll句柄
        int m_eventfd = -1;                                               // 使用 eventfd,来唤醒epoll_wait
        std::atomic<size_t> m_pendingEventCounts{0};                      // epoll事件计数
        std::unordered_map<int, std::unique_ptr<FdContext>> m_fdContexts; // 句柄字典

        static constexpr int MAX_EPOLL_EVENTS = 64;   // 最大epoll事件数量
        static constexpr int MAX_WAIT_TIMEOUT = 3000; // 最大超时时间
    };
}