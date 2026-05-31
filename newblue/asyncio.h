#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "io_manager.h"
#include "blue/log.h"

namespace newblue
{
    blue::Logger::LoggerPtr logger = BLUE_LOG_NAME("system");
    enum class IoOp { READ, WRITE, RECV, SEND, CONNECT, ACCEPT };

    template <IoOp Op, bool WithTimeout = false>
    class AsyncIo
    {
    private:
        int fd;
        char* buf;
        size_t len;
        int flags;
        sockaddr* addr;
        socklen_t socklen;
        uint64_t timeout_ms = 0;
        ssize_t ret = -1;
        Timer::TimerPtr timer;
    public:
        // read or write
        AsyncIo(int f, char* buf, size_t len)
        requires (Op == IoOp::READ || Op == IoOp::WRITE)
        : fd(f), buf(buf), len(len) {}

        // recv or send
        AsyncIo(int f, void* b, size_t l, int fl)
        requires (Op == IoOp::RECV || Op == IoOp::SEND)
        : fd(f), buf((char*)b), len(l), flags(fl) {}
        
        // read or write with timeout
        AsyncIo(int f, void* b, size_t l, uint64_t ms)
        requires ((Op == IoOp::READ || Op == IoOp::WRITE) && WithTimeout)
        : fd(f), buf((char*)b), len(l), timeout_ms(ms) {}

        // recv or send with timeout
        AsyncIo(int f, void* b, size_t l, int fl, uint64_t ms)
        requires ((Op == IoOp::RECV || Op == IoOp::SEND) && WithTimeout)
        : fd(f), buf((char*)b), len(l), flags(fl), timeout_ms(ms) {}

        // connect or accept
        AsyncIo(int f, struct sockaddr* addr, socklen_t len)
        requires (Op == IoOp::CONNECT || Op == IoOp::ACCEPT)
        : fd(f), addr(addr), socklen(len) {}

        // connect or accept with timeout
        AsyncIo(int f, struct sockaddr* addr, socklen_t len, uint64_t ms)
        requires ((Op == IoOp::CONNECT || Op == IoOp::ACCEPT) && WithTimeout)
        : fd(f), addr(addr), socklen(len), timeout_ms(ms) {}

        ssize_t do_io() 
        {
            if constexpr (Op == IoOp::READ) return ::read(fd, buf, len);
            else if constexpr (Op == IoOp::WRITE) return ::write(fd, buf, len);
            else if constexpr (Op == IoOp::RECV) return ::recv(fd, buf, len, flags);
            else if constexpr (Op == IoOp::SEND) return ::send(fd, buf, len, flags);
            else if constexpr (Op == IoOp::CONNECT) return ::connect(fd,(const sockaddr*)addr,socklen);
            else if constexpr (Op == IoOp::ACCEPT) return ::accept(fd,addr,&socklen);
            return -1;
        }

        IOManager::Event io_event() 
        {
            if constexpr (Op == IoOp::READ || Op == IoOp::RECV || Op == IoOp::ACCEPT) return IOManager::READ;
            else return IOManager::WRITE;
        }

        bool await_ready()
        {
            ret = do_io();
            if (ret >= 0)
            {
                return true;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return false; // 挂起提交一个任务
            }
            BLUE_LOG_ERROR(logger) << " AsyncRead error, fd : " << fd
                                    << " error : " << errno
                                    << " strerrno : " << strerror(errno);
            return true; // error
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            auto *iom = IOManager::GetThis();
            int tem = iom->addEvent(fd, io_event(), h, nullptr);
            if (tem)
            {
                BLUE_LOG_ERROR(logger) << " await_suspend addEvent() error, fd : " << fd;
                ret = -1;
                errno = EIO;
                h.resume();
                return;
            }
            if constexpr (WithTimeout)
            {
                timer = iom->addTimer(timeout_ms,h,nullptr);
            }
        }
        ssize_t await_resume()
        {
            // errno == EAGIN 或 EWOULDBLOCK 再试一次
            if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                ret = do_io();
            }
            // 如果还是 EAGAIN，说明是定时器先触发没有经过epoll的唤醒（超时）
            if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                IOManager::GetThis()->delEvent(fd, io_event());
                BLUE_LOG_ERROR(logger) << " Asyncio error, fd : " << fd
                                        << " error : " << errno
                                        << " strerrno : " << strerror(errno)
                                        <<  (int)io_event()
                                        << " event be canceled";
                return 0; // 超时返回 0
            }
            // 到这里要么出错且不是EAGIN等,要么成功
            if (timer)
            {
                timer->cancel();
            }
            return ret;
        }

    };

    auto Read(int fd, void* buf, size_t len) 
    { 
        return AsyncIo<IoOp::READ>{fd, (char*)buf, len}; 
    }
    auto Write(int fd, const void* buf, size_t len) 
    { 
        return AsyncIo<IoOp::WRITE>{fd, (char*)buf, len}; 
    }
    auto Recv(int sockfd, void* buf, size_t len, int flags = 0) 
    { 
        return AsyncIo<IoOp::RECV>{sockfd, (char*)buf, len, flags}; 
    }
    auto Send(int sockfd, const void* buf, size_t len, int flags = 0) 
    { 
        return AsyncIo<IoOp::SEND>{sockfd, (char*)buf, len, flags}; 
    }

    auto ReadT(int fd, void* buf, size_t len, uint64_t ms) 
    { 
        return AsyncIo<IoOp::READ, true>{fd, (char*)buf, len, ms}; 
    }

    auto WriteT(int fd, const void* buf, size_t len, uint64_t ms) 
    { 
        return AsyncIo<IoOp::WRITE, true>{fd, (char*)buf, len, ms}; 
    }
    
    auto RecvT(int sockfd, void* buf, size_t len, int flags = 0, uint64_t ms = 0)
    {
        return AsyncIo<IoOp::RECV,true>{sockfd, buf, len, flags, ms};
    }

    auto SendT(int sockfd, const void* buf, size_t len, int flags = 0, uint64_t ms = 0)
    {
        return AsyncIo<IoOp::SEND, true>{sockfd, (char*)buf, len, flags, ms};
    }

    auto Connect(int sockfd, const struct sockaddr* addr, socklen_t len)
    {
        return AsyncIo<IoOp::CONNECT>{sockfd, (sockaddr*)addr,len};
    }

    auto Accept(int sockfd, struct sockaddr* addr, socklen_t* len)
    {
        return AsyncIo<IoOp::ACCEPT>{sockfd,addr,*len};
    }

    auto ConnectT(int sockfd, const struct sockaddr* addr, socklen_t len, uint64_t ms)
    {
        return AsyncIo<IoOp::CONNECT, true>{sockfd, (sockaddr*)addr,len, ms};
    }

    auto Accept(int sockfd, struct sockaddr* addr, socklen_t len, uint64_t ms)
    {
        return AsyncIo<IoOp::ACCEPT, true>{sockfd,addr,len, ms};
    }

} // namespace newblue