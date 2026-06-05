#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include "io_manager.h"
#include "log.h"

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
        bool is_timeout = false;

    public:
        AsyncIo(int f, OriginFunc fun, IOManager::Event event, Args... args)
            : fd(f), event(event), func(fun), io_args(std::move(args)...) {}

        AsyncIo(int f, OriginFunc fun, IOManager::Event event, WithTimeoutTag, uint64_t ms, Args... args)
            : fd(f), event(event), func(fun), io_args(std::move(args)...), timeout_ms(ms) { has_timer = true; }

        bool await_ready()
        {
            ret = std::apply(func, std::tuple_cat(std::make_tuple(fd), io_args));
            if (ret >= 0)
            {
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return false; // 挂起提交一个任务
            }
            BLUE_LOG_ERROR(xx::g_logger) << " AsyncRead error, fd : " << fd
                                         << " error : " << errno
                                         << " strerrno : " << strerror(errno);
            return true; // error
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            auto *iom = IOManager::GetThis();
            if (!iom)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "not have iom";
                ret = -1;
                errno = EIO;
                h.resume();
                return;
            }
            int tem = iom->addEvent(fd, event, h, nullptr);
            if (tem)
            {
                BLUE_LOG_ERROR(xx::g_logger) << " await_suspend addEvent() error, fd : " << fd;
                ret = -1;
                errno = EIO;
                h.resume();
                return;
            }
            if (has_timer)
            {
                timer = iom->addTimer(timeout_ms, nullptr, [this, h] {
                    is_timeout = true;
                    h.resume();
                });
            }
        }
        ssize_t await_resume()
        {
            if (timer)
            {
                timer->cancel();
                timer = nullptr;
            }
            if (is_timeout)
            {
                IOManager::GetThis()->delEvent(fd, event);
                errno = ETIMEDOUT;
                return 0;
            }
            // errno == EAGIN 或 EWOULDBLOCK 再试一次
            if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
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