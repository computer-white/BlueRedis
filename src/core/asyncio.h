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
 * @file asyncio.h
 * @brief 基于协程对io操作的封装
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.2
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include "src/core/resume_ticket.h"
#include "src/core/io_manager.h"
#include "src/util/log.h"

namespace blue
{
    namespace xx
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    }

    struct WithTimeoutTag
    {
    };
    inline constexpr WithTimeoutTag with_timeout{};

    template <typename OriginFunc, typename... Args>
    class AsyncIo
    {
    private:
        int fd;
        IOManager::Event event;
        OriginFunc func;
        std::tuple<Args...> io_args;
        uint64_t timeout_ms = 0;
        bool has_timer = false;
        ssize_t ret = -1;
        Timer::TimerPtr timer;
        std::shared_ptr<ResumeTicket> resumeTicket;
        std::shared_ptr<AsyncIoState> state;
        int old_error = 0;

    public:
        AsyncIo(int f, OriginFunc fun, IOManager::Event event, Args... args)
            : fd(f), event(event), func(fun), io_args(std::move(args)...) {}

        AsyncIo(int f, OriginFunc fun, IOManager::Event event, WithTimeoutTag, uint64_t ms, Args... args)
            : fd(f), event(event), func(fun), io_args(std::move(args)...), timeout_ms(ms) { has_timer = true; }

        ~AsyncIo()
        {
            if (resumeTicket)
            {
                resumeTicket->m_armed.store(false, std::memory_order_release);
            }
            if (timer)
            {
                timer->cancel();
                timer = nullptr;
            }
            if (state && state->event_added.exchange(false, std::memory_order_acq_rel))
            {
                IOManager::GetThis()->delEvent(fd, event);
            }
        }

        bool await_ready()
        {
            ret = std::apply(func, std::tuple_cat(std::make_tuple(fd), io_args));
            if (ret >= 0)
            {
                return true;
            }
            old_error = errno;
            if (old_error == EAGAIN || old_error == EWOULDBLOCK)
            {
                return false; // 挂起提交一个任务
            }
            BLUE_LOG_ERROR(xx::g_logger) << " AsyncRead error, fd : " << fd
                                         << " error : " << old_error
                                         << " strerrno : " << strerror(old_error);
            return true; // error
        }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            auto *iom = IOManager::GetThis();
            if (!iom)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "not have iom";
                ret = -1;
                errno = EIO;
                return h;
            }

            resumeTicket = std::make_shared<blue::ResumeTicket>();
            resumeTicket->m_handle = h;
            resumeTicket->m_armed.store(true, std::memory_order_release);

            state = std::make_shared<AsyncIoState>();
            state->event_added.store(true, std::memory_order_release);

            int tem = iom->addEvent(fd, event, resumeTicket, nullptr);
            if (tem)
            {
                BLUE_LOG_ERROR(xx::g_logger) << " await_suspend addEvent() error, fd : " << fd;
                resumeTicket->m_armed.store(false, std::memory_order_release);
                ret = -1;
                errno = EIO;
                return h;
            }

            if (has_timer)
            {
                timer = iom->addTimer(timeout_ms, nullptr, [state = state, resumeTicket = resumeTicket, fd = fd, event = event]
                                      {
                    state->is_timeout.store(true, std::memory_order_release);
                    if (state->event_added.exchange(false, std::memory_order_acq_rel))
                    {
                        IOManager::GetThis()->delEvent(fd, event);
                    }
                    try_resume(resumeTicket); });
            }
            return std::noop_coroutine();
        }
        ssize_t await_resume()
        {
            if (timer)
            {
                timer->cancel();
                timer = nullptr;
            }
            if (state && state->event_added.exchange(false))
            {
                IOManager::GetThis()->delEvent(fd, event);
            }
            if (state && state->is_timeout.load())
            {
                errno = ETIMEDOUT;
                return -1;
            }
            // errno == EAGIN 或 EWOULDBLOCK 再试一次
            if (ret < 0 && (old_error == EAGAIN || old_error == EWOULDBLOCK || old_error == EINTR))
            {
                ret = std::apply(func, std::tuple_cat(std::make_tuple(fd), io_args));
            }
            return ret;
        }
    };

    // read
    inline auto Read(int fd, void *buf, size_t len)
    {
        return AsyncIo{fd, ::read, IOManager::READ, buf, len};
    }

    // write
    inline auto Write(int fd, const void *buf, size_t len)
    {
        return AsyncIo{fd, ::write, IOManager::WRITE, buf, len};
    }

    // recv
    inline auto Recv(int sockfd, void *buf, size_t len, int flags = 0)
    {
        return AsyncIo{sockfd, ::recv, IOManager::READ, buf, len, flags};
    }

    // send
    inline auto Send(int sockfd, const void *buf, size_t len, int flags = 0)
    {
        return AsyncIo{sockfd, ::send, IOManager::WRITE, buf, len, flags};
    }

    // read 超时
    inline auto ReadT(int fd, void *buf, size_t len, uint64_t ms = 0)
    {
        return AsyncIo{fd, ::read, IOManager::READ, with_timeout, ms, buf, len};
    }

    // write 超时
    inline auto WriteT(int fd, const void *buf, size_t len, uint64_t ms = 0)
    {
        return AsyncIo{fd, ::write, IOManager::WRITE, with_timeout, ms, buf, len};
    }

    // recv 超时
    inline auto RecvT(int sockfd, void *buf, size_t len, int flags = 0, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::recv, IOManager::READ, with_timeout, ms, buf, len, flags};
    }

    // send 超时
    inline auto SendT(int sockfd, const void *buf, size_t len, int flags = 0, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::send, IOManager::WRITE, with_timeout, ms, buf, len, flags};
    }

    // connect
    inline auto Connect(int sockfd, const struct sockaddr *addr, socklen_t len)
    {
        return AsyncIo{sockfd, ::connect, IOManager::WRITE, addr, len};
    }

    // accept
    inline auto Accept(int sockfd, struct sockaddr *addr, socklen_t *len)
    {
        return AsyncIo{sockfd, ::accept, IOManager::READ, addr, len};
    }

    // connect 超时
    inline auto ConnectT(int sockfd, const struct sockaddr *addr, socklen_t len, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::connect, IOManager::WRITE, with_timeout, ms, addr, len};
    }

    // accept 超时
    inline auto AcceptT(int sockfd, struct sockaddr *addr, socklen_t *len, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::accept, IOManager::READ, with_timeout, ms, addr, len};
    }

    // readv
    inline auto Readv(int fd, const struct iovec *iov, int iocvnt)
    {
        return AsyncIo{fd, ::readv, IOManager::READ, iov, iocvnt};
    }

    // recvfrom
    inline auto Recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return AsyncIo{sockfd, ::recvfrom, IOManager::READ, buf, len, flags, src_addr, addrlen};
    }

    // recvmsg
    inline auto Recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return AsyncIo{sockfd, ::recvmsg, IOManager::READ, msg, flags};
    }

    // writev
    inline auto Writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return AsyncIo{fd, ::writev, IOManager::WRITE, iov, iovcnt};
    }

    // sendto
    inline auto Sendto(int sockfd, const void *buf, size_t len, int flags,
                       const struct sockaddr *dest_addr, socklen_t addrlen)
    {
        return AsyncIo{sockfd, ::sendto, IOManager::WRITE, buf, len, flags, dest_addr, addrlen};
    }

    // snedmsg
    inline auto Sendmsg(int sockfd, const struct msghdr *msg, int flags)
    {
        return AsyncIo{sockfd, ::sendmsg, IOManager::WRITE, msg, flags};
    }

    // readv 超时
    inline auto ReadvT(int fd, const struct iovec *iov, int iocvnt, uint64_t ms = 0)
    {
        return AsyncIo{fd, ::readv, IOManager::READ, with_timeout, ms, iov, iocvnt};
    }

    // recvfrom 超时
    inline auto RecvfromT(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::recvfrom, IOManager::READ, with_timeout, ms, buf, len, flags, src_addr, addrlen};
    }

    // recvmsg 超时
    inline auto RecvmsgT(int sockfd, struct msghdr *msg, int flags, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::recvmsg, IOManager::READ, with_timeout, ms, msg, flags};
    }

    // writev 超时
    inline auto WritevT(int fd, const struct iovec *iov, int iovcnt, uint64_t ms = 0)
    {
        return AsyncIo{fd, ::writev, IOManager::WRITE, with_timeout, ms, iov, iovcnt};
    }

    // sendto 超时
    inline auto SendtoT(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::sendto, IOManager::WRITE, with_timeout, ms, buf, len, flags, dest_addr, addrlen};
    }

    // sendmsg 超时
    inline auto SendmsgT(int sockfd, const struct msghdr *msg, int flags, uint64_t ms = 0)
    {
        return AsyncIo{sockfd, ::sendmsg, IOManager::WRITE, with_timeout, ms, msg, flags};
    }

} // namespace newblue