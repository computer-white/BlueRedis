#include <unistd.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include "io_manager.h"
#include "macro.h"
#include "log.h"

namespace blue
{
    static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    // FdContext 实现
    IOManager::FdContext::EventContext &
    IOManager::FdContext::getEventContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::Event::READ:
            return read;
        case IOManager::Event::WRITE:
            return write;
        default:
            BLUE_ASSERT2(false, "getEventContext: invalid event");
            throw std::invalid_argument("Invalid event type");
        }
    }

    void IOManager::FdContext::resetEventContext(IOManager::FdContext::EventContext &ec)
    {
        ec.reset();
    }

    void IOManager::FdContext::triggerContext(IOManager::Event event)
    {
        BLUE_ASSERT(m_events & event);

        // 清除事件标志
        m_events = static_cast<Event>(m_events & ~event);

        // 获取并调度事件上下文
        EventContext &ctx = getEventContext(event);

        // BLUE_LOG_INFO(g_logger) << "[trigger] fd: " << fd
        //                         << " event: " << event
        //                         << " has_handle: " << (ctx.handle != nullptr)
        //                         << " has_cb: " << (ctx.cb != nullptr);

        Scheduler *scheduler = ctx.scheduler;
        std::function<void()> cb = std::move(ctx.cb);
        std::coroutine_handle<> handle = ctx.handle;
        int thread_id = ctx.thread_id;

        ctx.reset();

        // 调度任务
        if (scheduler)
        {
            if (cb)
            {
                scheduler->schedule(std::move(cb), thread_id);
            }
            else if (handle)
            {
                scheduler->schedule(handle, thread_id);
            }
        }
        else
        {
            BLUE_LOG_ERROR(g_logger) << "triggerContext: scheduler is null";

            // 降级处理：直接在当前线程执行
            if (cb)
                cb();
            if (handle && !handle.done())
                handle.resume();
        }
    }

    // IOManager 实现
    IOManager::IOManager(size_t threads)
        : Scheduler(threads)
    {
        // 创建 epoll 实例
        m_epfd = epoll_create1(EPOLL_CLOEXEC);
        BLUE_ASSERT(m_epfd > 0);

        m_eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        BLUE_ASSERT(m_eventfd > 0);

        // 注册 eventfd 到 epoll
        epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_eventfd;

        int rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_eventfd, &event);
        BLUE_ASSERT(rt == 0);

        start();

        // 分配一个线程在idle上运行
        m_workers.emplace_back([this]()
                               {
            Scheduler::setThis(this);
            this->idle(); });
    }

    IOManager::~IOManager()
    {
        // 先停止
        if (!m_stopping.load(std::memory_order_acquire))
        {
            stop();
        }

        // 清理所有 fd 上下文,保证内部存储的fd在外部已经全部关闭
        {
            std::unique_lock<std::shared_mutex> lock(m_Iommutex);
            m_fdContexts.clear();
        }

        // 关闭文件描述符
        if (m_eventfd != -1)
        {
            close(m_eventfd);
            m_eventfd = -1;
        }

        if (m_epfd != -1)
        {
            close(m_epfd);
            m_epfd = -1;
        }
    }

    int IOManager::addEvent(int fd, Event event,
                            std::coroutine_handle<> h,
                            std::function<void()> cb,
                            int thread_id)
    {
        if (fd < 0)
        {
            return -1;
        }

        // 获取或创建 FdContext
        FdContext *fd_ctx = nullptr;

        {
            std::shared_lock<std::shared_mutex> read_lock(m_Iommutex);
            auto it = m_fdContexts.find(fd);

            if (it != m_fdContexts.end())
            {
                fd_ctx = it->second.get();
            }
            else
            {
                read_lock.unlock();
                std::unique_lock<std::shared_mutex> write_lock(m_Iommutex);

                // 双重检查
                auto [new_it, inserted] = m_fdContexts.try_emplace(
                    fd, std::make_unique<FdContext>());
                fd_ctx = new_it->second.get();

                if (inserted)
                {
                    fd_ctx->fd = fd;
                }
            }
        }

        if (!fd_ctx)
        {
            return -1;
        }

        // 锁定 fd 上下文
        std::lock_guard<std::mutex> lock(fd_ctx->mutex);

        // 检查是否已注册相同事件
        if (fd_ctx->m_events & event)
        {
            BLUE_LOG_ERROR(g_logger) << "addEvent: event already registered, fd="
                                     << fd << " event=" << event;
            return -1;
        }

        // 确定 epoll 操作
        int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        Event new_event = static_cast<Event>(fd_ctx->m_events | event);

        // 设置 epoll 事件
        epoll_event epevent;
        memset(&epevent, 0, sizeof(epevent));
        epevent.events = EPOLLET | static_cast<uint32_t>(new_event);
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error: "
                                     << "epfd=" << m_epfd
                                     << " op=" << op
                                     << " fd=" << fd
                                     << " errno=" << errno
                                     << " (" << strerror(errno) << ")";
            return -1;
        }

        // 更新状态
        ++m_pendingEventCounts;
        fd_ctx->m_events = new_event;

        // 设置事件上下文
        FdContext::EventContext &event_ctx = fd_ctx->getEventContext(event);
        event_ctx.scheduler = Scheduler::GetThis();

        if (cb)
        {
            event_ctx.cb = std::move(cb);
            event_ctx.thread_id = thread_id;
        }
        else if (h)
        {
            event_ctx.handle = h;
            event_ctx.thread_id = thread_id;
        }
        else
        {
            // 没有有效的回调或协程句柄
            BLUE_LOG_ERROR(g_logger) << "addEvent: no callback or handle provided";
            delEvent(fd, event);
            return -1;
        }

        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        if (fd < 0)
            return false;

        std::shared_lock<std::shared_mutex> read_lock(m_Iommutex);
        auto it = m_fdContexts.find(fd);

        if (it == m_fdContexts.end())
        {
            return false;
        }

        FdContext *fd_ctx = it->second.get();
        read_lock.unlock();

        std::lock_guard<std::mutex> lock(fd_ctx->mutex);

        if (!(fd_ctx->m_events & event))
        {
            return false;
        }

        // 计算新的事件集
        Event new_event = static_cast<Event>(fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        epoll_event epevent;
        memset(&epevent, 0, sizeof(epevent));
        epevent.events = EPOLLET | static_cast<uint32_t>(new_event);
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "delEvent: epoll_ctl error: "
                                     << "epfd=" << m_epfd
                                     << " op=" << op
                                     << " fd=" << fd
                                     << " errno=" << errno
                                     << " (" << strerror(errno) << ")";
            return false;
        }

        // --m_pendingEventCounts;
        fd_ctx->m_events = new_event;

        // 重置事件上下文
        FdContext::EventContext &event_ctx = fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(event_ctx);
        --m_pendingEventCounts;
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        if (fd < 0)
            return false;

        std::shared_lock<std::shared_mutex> read_lock(m_Iommutex);
        auto it = m_fdContexts.find(fd);

        if (it == m_fdContexts.end())
        {
            return false;
        }

        FdContext *fd_ctx = it->second.get();
        read_lock.unlock();

        std::lock_guard<std::mutex> lock(fd_ctx->mutex);

        if (!(fd_ctx->m_events & event))
        {
            return false;
        }

        // 计算新的事件集
        Event new_event = static_cast<Event>(fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        epoll_event epevent;
        memset(&epevent, 0, sizeof(epevent));
        epevent.events = EPOLLET | static_cast<uint32_t>(new_event);
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "cancelEvent: epoll_ctl error: "
                                     << "epfd=" << m_epfd
                                     << " op=" << op
                                     << " fd=" << fd
                                     << " errno=" << errno
                                     << " (" << strerror(errno) << ")";
            return false;
        }

        // 触发事件（调度相应的回调/协程）
        fd_ctx->triggerContext(event);
        --m_pendingEventCounts;

        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        if (fd < 0)
            return false;

        std::shared_lock<std::shared_mutex> read_lock(m_Iommutex);
        auto it = m_fdContexts.find(fd);

        if (it == m_fdContexts.end())
        {
            return false;
        }

        FdContext *fd_ctx = it->second.get();
        read_lock.unlock();

        std::lock_guard<std::mutex> lock(fd_ctx->mutex);

        if (!fd_ctx->m_events)
        {
            return false;
        }

        // 从 epoll 中移除
        epoll_event epevent;
        memset(&epevent, 0, sizeof(epevent));

        int rt = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &epevent);
        if (rt && errno != ENOENT) // ENOENT 表示已经删除
        {
            BLUE_LOG_ERROR(g_logger) << "cancelAll: epoll_ctl error: "
                                     << "epfd=" << m_epfd
                                     << " fd=" << fd
                                     << " errno=" << errno
                                     << " (" << strerror(errno) << ")";
            return false;
        }

        // 触发所有已注册的事件
        bool has_read = fd_ctx->m_events & Event::READ;
        bool has_write = fd_ctx->m_events & Event::WRITE;

        if (has_read)
        {
            fd_ctx->triggerContext(Event::READ);
            --m_pendingEventCounts;
        }

        if (has_write)
        {
            fd_ctx->triggerContext(Event::WRITE);
            --m_pendingEventCounts;
        }

        fd_ctx->m_events = Event::NONE;

        return true;
    }

    bool IOManager::stopping()
    {
        return m_pendingEventCounts.load(std::memory_order_acquire) == 0 && !TimerManager::hasTimer() && Scheduler::stopping();
    }

    void IOManager::idle()
    {
        Scheduler::setThis(this);
        std::array<epoll_event, MAX_EPOLL_EVENTS> events;

        while (!stopping())
        {
            int rt = 0;
            int timeout = MAX_WAIT_TIMEOUT;

            // 获取下一个定时器超时时间
            do
            {
                uint64_t next_timeout = TimerManager::getNextTime();
                if (next_timeout != ~0ull)
                {
                    timeout = std::min(static_cast<int>(next_timeout), MAX_WAIT_TIMEOUT);
                }

                rt = epoll_wait(m_epfd, events.data(), events.size(), timeout);

                if (rt < 0 && errno == EINTR)
                {
                    // 被信号中断，继续等待
                    continue;
                }
                else
                {
                    break;
                }
            } while (true);

            // 处理过期的定时器
            TimerManager::processExpired();

            // 再次检查是否需要停止
            if (stopping())
            {
                BLUE_LOG_INFO(g_logger) << "IOManager idle stopping: " << getName();
                break;
            }

            // 处理 I/O 事件
            for (int i = 0; i < rt; i++)
            {
                epoll_event &event = events[i];

                // 检查是否是唤醒事件(tickle)
                if (event.data.fd == m_eventfd)
                {
                    uint64_t value;
                    while (read(m_eventfd, &value, sizeof(value)) > 0)
                    {
                    }
                    continue;
                }

                // 处理 fd 事件
                FdContext *fd_ctx = static_cast<FdContext *>(event.data.ptr);
                if (!fd_ctx)
                    continue;

                std::lock_guard<std::mutex> lock(fd_ctx->mutex);

                // 处理错误和挂起事件
                uint32_t triggered_events = event.events;
                if (triggered_events & (EPOLLERR | EPOLLHUP))
                {
                    triggered_events |= EPOLLIN | EPOLLOUT;
                }

                // 确定触发的事件类型
                Event real_event = Event::NONE;
                if (triggered_events & EPOLLIN)
                {
                    real_event = static_cast<Event>(real_event | Event::READ);
                }
                if (triggered_events & EPOLLOUT)
                {
                    real_event = static_cast<Event>(real_event | Event::WRITE);
                }

                // 检查是否有我们关心的事件
                Event active_events = static_cast<Event>(fd_ctx->m_events & real_event);
                if (active_events == Event::NONE)
                {
                    continue;
                }

                // 计算剩余事件
                Event left_events = static_cast<Event>(fd_ctx->m_events & ~real_event);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

                epoll_event new_event;
                memset(&new_event, 0, sizeof(new_event));
                new_event.events = EPOLLET | static_cast<uint32_t>(left_events);
                new_event.data.ptr = fd_ctx;

                // 更新 epoll
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &new_event);
                if (rt2)
                {
                    BLUE_LOG_ERROR(g_logger) << "idle: epoll_ctl error: "
                                             << "epfd=" << m_epfd
                                             << " op=" << op
                                             << " fd=" << fd_ctx->fd
                                             << " errno=" << errno
                                             << " (" << strerror(errno) << ")";
                    continue;
                }

                // 触发事件
                if (active_events & Event::READ)
                {
                    fd_ctx->triggerContext(Event::READ);
                    --m_pendingEventCounts;
                }

                if (active_events & Event::WRITE)
                {
                    fd_ctx->triggerContext(Event::WRITE);
                    --m_pendingEventCounts;
                }
            }
        }

        BLUE_LOG_INFO(g_logger) << "IOManager idle exited: " << getName();
    }

    void IOManager::tickle()
    {
        // 使用 eventfd 唤醒 epoll_wait
        uint64_t value = 1;
        ssize_t ret = write(m_eventfd, &value, sizeof(value));
        if (ret != sizeof(value))
        {
            BLUE_LOG_DEBUGE(g_logger) << "tickle: write to eventfd returned " << ret;
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle(); // 唤醒 idle 线程以重新计算超时时间
    }

    void IOManager::wait_all()
    {
        std::unique_lock<std::mutex> lock(m_Schemutex);
        m_Schecv.wait(lock, [this]
                      {
            size_t total = m_pending.load(std::memory_order_acquire);
            for (auto& queue : m_threadQueues)
            {
                total += queue->pending.load(std::memory_order_acquire);
            }
            // BLUE_LOG_INFO(g_logger) << "total: " << total << " m_pendingEventCounts: " << m_pendingEventCounts.load(std::memory_order_acquire)
            //                         << " hasTimer: " << TimerManager::hasTimer();
            return total == 0 
                   && m_pendingEventCounts.load(std::memory_order_acquire) == 0
                   && !TimerManager::hasTimer()
                   && m_running.load(std::memory_order_acquire) == 0; });
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }
}