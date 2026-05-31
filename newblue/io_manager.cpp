#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include "io_manager.h"
#include "blue/macro.h"
#include "blue/log.h"

namespace newblue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::Event::READ:
            return read;
        case IOManager::Event::WRITE:
            return write;
        default:
            BLUE_ASSERT2(false, "getEventContext");
            break;
        }
    }

    void IOManager::FdContext::resetEventContext(IOManager::FdContext::EventContext &ec)
    {
        ec.scheduler = nullptr;
        ec.handle = nullptr;
        ec.cb = nullptr;
    }

    void IOManager::FdContext::triggerContext(IOManager::Event event)
    {
        BLUE_ASSERT(m_events & event);
        // 哪个对象调用删除哪个对象的event事件,并提交event任务
        m_events = (Event)(m_events & ~event);
        // 获取event事件对应的任务
        EventContext &ctx = getEventContext(event);
        BLUE_LOG_INFO(g_logger) << "[trigger] event : " << event << "has_handle : " << (ctx.handle != nullptr);
        if (ctx.cb)
        {
            auto cb = ctx.cb;
            ctx.cb = nullptr;
            ctx.scheduler->schedule(cb);
        }
        if (ctx.handle)
        {
            auto t = ctx.handle;
            ctx.handle = nullptr;
            ctx.scheduler->schedule(t);
        }
        ctx.scheduler = nullptr;
        return;
    }

    IOManager::IOManager(size_t threads)
    :Scheduler(threads)
    {
        m_epfd = epoll_create(5);
        BLUE_ASSERT(m_epfd > 0);
        int rt = pipe(m_ticklefds);
        BLUE_ASSERT(rt == 0);
        // 设置默认epoll event
        epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN | EPOLLET; // 边缘触发 EPOLLET
        event.data.fd = m_ticklefds[0];   // 设置文件描述符

        // 设置读文件描述符状态为非阻塞模式(O_NONBLOCK)
        rt = fcntl(m_ticklefds[0], F_SETFL, O_NONBLOCK);
        BLUE_ASSERT(rt == 0);

        // 添加(EPOLL_CTL_ADD)一个监听对象(m_ticklefds[0]),并注册一个fd相关的event事件的就绪回调,
        // 若这个对象(读端)被写入数据,则触发可读事件
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_ticklefds[0], &event);
        BLUE_ASSERT(rt == 0);
        std::thread([this]() {      // 需要有一个线程运行在idle上处理epoll_wait返回的事件
            Scheduler::setThis(this);
            this->idle();
        }).detach();
        start();
        // std::thread([this]() {      // 需要有一个线程运行在idle上处理epoll_wait返回的事件
        //     Scheduler::setThis(this);
        //     this->idle();
        // }).detach();
    }
    IOManager::~IOManager()
    {
        stop();
        close(m_epfd);
        close(m_ticklefds[0]);
        close(m_ticklefds[1]);
        for (auto it : m_fdContexts)
        {
            delete it.second;
        }
        
    }

    int IOManager::addEvent(int fd, Event event, std::coroutine_handle<> h, std::function<void()> cb)
    {
        std::shared_lock<std::shared_mutex> lock1(m_mutex);
        FdContext *fd_ctx = nullptr;

        auto it = m_fdContexts.find(fd);
        if (it == m_fdContexts.end())
        {
            lock1.unlock();
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            if (it == m_fdContexts.end())
            {
                m_fdContexts[fd] = new FdContext;
                m_fdContexts[fd]->fd = fd;
                fd_ctx = m_fdContexts[fd];
            }
        }
        else
        {
            fd_ctx = it->second;
            lock1.unlock();
        }

        std::lock_guard<std::mutex> lock(fd_ctx->mutex);
        // 重复提交相同任务
        if (fd_ctx->m_events & event)
        {
            BLUE_LOG_ERROR(g_logger) << "addEvent assert id : " << fd
                                     << " event : " << event
                                     << " fc->m_events : " << fd_ctx->m_events;
            BLUE_ASSERT(!(fd_ctx->m_events & event));
        }
        int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        Event newevent = (Event)(fd_ctx->m_events | event);
        epoll_event epevent;
        epevent.events = EPOLLET | (unsigned)newevent;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return -1;
        }
        ++m_pendingEventCounts;
        // 将event添加为已注册的事件
        fd_ctx->m_events = (Event)(fd_ctx->m_events | event);

        FdContext::EventContext &event_context = fd_ctx->getEventContext(event);
        BLUE_ASSERT(!event_context.scheduler &&
                    !event_context.handle&&
                    !event_context.cb);
        event_context.scheduler = Scheduler::GetThis();
        if (cb)
        {
            event_context.cb.swap(cb);
        }
        else
        {
            event_context.handle = h;
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_fdContexts.find(fd);
        if (it == m_fdContexts.end())
        {
            return false;
        }
        auto* fd_ctx = m_fdContexts[fd];
        lock.unlock();
        std::lock_guard<std::mutex> lock1(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event))
        {
            return false;
        }
        Event new_event = (Event)(fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | (unsigned)new_event;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }
        --m_pendingEventCounts;
        // 删除某些事件后的新的事件
        fd_ctx->m_events = new_event;

        FdContext::EventContext &event_context = fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(event_context);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_fdContexts.find(fd);
        if (it == m_fdContexts.end())
        {
            return false;
        }
        auto* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        std::lock_guard<std::mutex> lock1(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event))
        {
            return false;
        }
        // 删除掉传进来的event后,加入新的监听
        Event new_event = (Event)(fd_ctx->m_events & ~event);

        // 删除或修改
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        // epevent
        epoll_event epevent;
        epevent.events = EPOLLET | (unsigned)new_event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }
        // triggerContext里面会删除fd_ctx->m_events中的event事件
        fd_ctx->triggerContext(event);
        --m_pendingEventCounts;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_fdContexts.find(fd);
        if (it == m_fdContexts.end())
        {
            return false;
        }
        auto* fd_ctx = m_fdContexts[fd];
        lock.unlock();
        std::lock_guard<std::mutex> lock2(fd_ctx->mutex);
        if (!fd_ctx->m_events)
        {
            return false;
        }
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }
        if (fd_ctx->m_events & Event::READ)
        {
            fd_ctx->triggerContext(Event::READ);
            --m_pendingEventCounts;
        }
        if (fd_ctx->m_events & Event::WRITE)
        {
            fd_ctx->triggerContext(Event::WRITE);
            --m_pendingEventCounts;
        }
        BLUE_ASSERT(fd_ctx->m_events == 0);
        return true;
    }

    bool IOManager::stopping()
    {
        return m_pendingEventCounts.load(std::memory_order_acquire) == 0 && !TimerManager::hasTimer() && Scheduler::stopping();
    }

    void IOManager::idle()
    {
        std::array<epoll_event, 64> epevent;

        while (true)
        {
            if (stopping())
            {
                BLUE_LOG_INFO(g_logger) << "name : " << getName() << " io idle stopping";
                break;
            }
            int rt = 0;
            do
            {
                static const int MAX_WAIT = 3000;
                uint64_t next_timeout = TimerManager::getNextTime();
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_WAIT ? MAX_WAIT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_WAIT;
                }
                // BLUE_LOG_DEBUGE(g_logger) << "next_timeout : " << next_timeout;
                rt = epoll_wait(m_epfd, &epevent[0], 64, (int)next_timeout);
                // 非阻塞IO,返回-1并设置errno = EINTR表示我们要的任务还没有被准备好
                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                else
                {
                    break; // 有任务了
                }
            } while (true);
            TimerManager::processExpired();
            // 处理可以执行的任务(epoll_wait返回的任务数)
            for (int i = 0; i < rt; i++)
            {
                epoll_event &event = epevent[i];
                BLUE_LOG_INFO(g_logger) << "[epoll] 收到事件, fd : " << event.data.fd;
                if (event.data.fd == m_ticklefds[0])
                {
                    // uint32_t dummy;
                    char dummy[6];
                    while (read(m_ticklefds[0], &dummy, 5) == 5)
                    {
                        // BLUE_LOG_INFO(g_logger) << dummy;
                    };
                    continue;
                }

                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                std::lock_guard<std::mutex> lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= EPOLLIN | EPOLLOUT;
                }

                int real_event = Event::NONE;
                if (event.events & EPOLLIN)
                {
                    real_event |= Event::READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_event |= Event::WRITE;
                }
                // 没有事件
                if ((fd_ctx->m_events & real_event) == Event::NONE)
                {
                    continue;
                }
                // 把事件拿出来后在epoll里面要么删除要么更改
                int left_events = (fd_ctx->m_events & ~real_event);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                             << "," << op << "," << fd_ctx->fd << "," << event.events << "):"
                                             << rt2 << "(" << errno << "," << strerror(errno) << ")";
                    continue;
                }

                int trigger_events = real_event & fd_ctx->m_events;
                if (trigger_events & Event::READ)
                {
                    fd_ctx->triggerContext(Event::READ);
                    --m_pendingEventCounts;
                }
                if (trigger_events & Event::WRITE)
                {
                    fd_ctx->triggerContext(Event::WRITE);
                    --m_pendingEventCounts;
                }
            }
        }
    }

    void IOManager::tickle()
    {
        write(m_ticklefds[1], "hello", 5);
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    void IOManager::wait_all()
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_cv.wait(lock,[this]{return m_pending.load(std::memory_order_acquire) == 0 
            && m_pendingEventCounts.load(std::memory_order_acquire) == 0;});
        
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
}