#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "io_manager.h"
#include "log.h"

namespace blue
{
    namespace xx
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    }
    enum class SSLIo : uint32_t
    {
        READ,
        WRITE,
        CONNECT,
        ACCEPT
    };

    template <SSLIo sslIo, bool WithTimeOut = false>
    class SSLAsyncIo
    {
    private:
        SSL *ssl;
        void *buf;
        size_t len;
        uint64_t timeout_ms = 0;
        ssize_t ret = -1;
        Timer::TimerPtr timer = nullptr;
        int fd = -1;
        IOManager::Event event;
        bool is_timeout = false;
        bool event_added = false;
        int old_error = 0;

    public:
        SSLAsyncIo(SSL *ssl, void *buf, size_t len)
            requires((sslIo == SSLIo::READ || sslIo == SSLIo::WRITE) && WithTimeOut == false)
            : ssl(ssl), buf(buf), len(len)
        {
        }
        SSLAsyncIo(SSL *ssl, void *buf, size_t len, uint64_t ms)
            requires(WithTimeOut && (sslIo == SSLIo::READ || sslIo == SSLIo::WRITE))
            : ssl(ssl), buf(buf), len(len), timeout_ms(ms)
        {
        }

        SSLAsyncIo(SSL *ssl)
            requires(sslIo == SSLIo::ACCEPT || sslIo == SSLIo::CONNECT)
            : ssl(ssl)
        {
        }

        SSLAsyncIo(SSL *ssl, uint64_t ms)
            requires((sslIo == SSLIo::ACCEPT || sslIo == SSLIo::CONNECT) && WithTimeOut)
            : ssl(ssl), timeout_ms(ms)
        {
        }

        ssize_t do_io()
        {
            if constexpr (sslIo == SSLIo::READ)
            {
                return SSL_read(ssl, buf, len);
            }
            else if constexpr (sslIo == SSLIo::WRITE)
            {
                return SSL_write(ssl, buf, len);
            }
            else if constexpr (sslIo == SSLIo::CONNECT)
            {
                return SSL_connect(ssl);
            }
            else if constexpr (sslIo == SSLIo::ACCEPT)
            {
                return SSL_accept(ssl);
            }
        }

        bool await_ready()
        {
            ret = do_io();
            if (ret >= 0)
            {
                return true;
            }
            int err = SSL_get_error(ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {
                errno = EAGAIN;
                old_error = EAGAIN;
                return false; // 挂起提交一个任务
            }
            if (err == SSL_ERROR_ZERO_RETURN)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "ssl 连接被关闭";
                ret = 0;
                return true;        // 返回0交给上级处理
            }
            return true; // error
        }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            int err = SSL_get_error(ssl, ret);
            fd = SSL_get_fd(ssl);  // 拿到底层 socket fd

            auto *iom = IOManager::GetThis();
            if (!iom)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "not have iom";
                ret = -1;
                errno = EIO;
                return h;       // 对称转移
            }
            if (err == SSL_ERROR_WANT_READ)
            {
                int tem = iom->addEvent(fd, IOManager::READ, h, nullptr);  // 等可读
                if (tem)
                {
                    BLUE_LOG_ERROR(xx::g_logger) << " await_suspend addEvent() error, fd : " << fd;
                    ret = -1;
                    errno = EIO;
                    return h;
                }
                event = IOManager::READ;
                event_added = true;
            } 
            else if (err == SSL_ERROR_WANT_WRITE) 
            {
                int tem = iom->addEvent(fd, IOManager::WRITE, h, nullptr);  // 等可写
                if (tem)
                {
                    BLUE_LOG_ERROR(xx::g_logger) << " await_suspend addEvent() error, fd : " << fd;
                    ret = -1;
                    errno = EIO;
                    return h;
                }
                event = IOManager::WRITE;
                event_added = true;
            } 
            else 
            {
                if constexpr (WithTimeOut)
                {
                    iom->schedule(h);  // 其他错误且是定时任务，直接调度
                }
                else
                {
                    return h;
                }
            }
            if constexpr (WithTimeOut)
            {
                timer = iom->addTimer(timeout_ms, nullptr, [this,h]{
                    is_timeout = true;
                    if (event_added)
                    {
                        IOManager::GetThis()->delEvent(fd,event);
                        event_added = false;
                    }
                    if (h && !h.done())
                    {
                        h.resume();
                    }
                });
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
            if (event_added)
            {
                IOManager::GetThis()->delEvent(fd, event);
                event_added = false;
            }
            if (is_timeout)
            {
                errno = ETIMEDOUT;
                return -1;
            }
            // errno == EAGIN
            if (ret < 0 && (old_error == EAGAIN || old_error == EWOULDBLOCK))
            {
                ret = do_io();
            }
            if (ret < 0)
            {
                int err = SSL_get_error(ssl, ret);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                {
                    errno = EAGAIN;
                }
                if (err == SSL_ERROR_ZERO_RETURN)
                {
                    ret = 0; // 连接关闭
                }
            }
            return ret;
        }
    };

    inline auto SRead(SSL *ssl, void *buf, size_t len)
    {
        return SSLAsyncIo<SSLIo::READ, false>{ssl, buf, len};
    }

    inline auto SWrite(SSL *ssl, const void *buf, size_t len)
    {
        return SSLAsyncIo<SSLIo::WRITE, false>{ssl, const_cast<void *>(buf), len};
    }

    inline auto SReadT(SSL *ssl, void *buf, size_t len, uint64_t ms = 0)
    {
        return SSLAsyncIo<SSLIo::READ, true>{ssl, buf, len, ms};
    }

    inline auto SWriteT(SSL *ssl, const void *buf, size_t len, uint64_t ms = 0)
    {
        return SSLAsyncIo<SSLIo::WRITE, true>{ssl, const_cast<void *>(buf), len, ms};
    }

    inline auto SConnect(SSL *ssl)
    {
        return SSLAsyncIo<SSLIo::CONNECT, false>{ssl};
    }

    inline auto SAccept(SSL *ssl)
    {
        return SSLAsyncIo<SSLIo::ACCEPT, false>{ssl};
    }

    inline auto SConnectT(SSL *ssl, uint64_t ms = 0)
    {
        return SSLAsyncIo<SSLIo::CONNECT, true>{ssl, ms};
    }

    inline auto SAcceptT(SSL *ssl, uint64_t ms = 0)
    {
        return SSLAsyncIo<SSLIo::ACCEPT, true>{ssl, ms};
    }

}