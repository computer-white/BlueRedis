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
                return false; // 挂起提交一个任务
            }
            if (err == SSL_ERROR_ZERO_RETURN)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "ssl 连接被关闭";
                ret = 0;
                return false;
            }
            return true; // error
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            if (ret == 0)
            {
                h.resume();
                return;
            }
            int err = SSL_get_error(ssl, ret);
            int fd = SSL_get_fd(ssl);  // 拿到底层 socket fd
            
            auto *iom = IOManager::GetThis();
            if (!iom)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "not have iom";
                ret = -1;
                errno = EIO;
                h.resume();
                return;
            }
            if (err == SSL_ERROR_WANT_READ)
            {
                iom->addEvent(fd, IOManager::READ, h, nullptr);  // 等可读
            } 
            else if (err == SSL_ERROR_WANT_WRITE) 
            {
                iom->addEvent(fd, IOManager::WRITE, h, nullptr);  // 等可写
            } 
            else 
            {
                iom->schedule(h);  // 其他错误，直接调度
            }
            if constexpr (WithTimeOut)
            {
                timer = iom->addTimer(timeout_ms, h, nullptr);
            }
        }
        ssize_t await_resume()
        {
            // errno == EAGIN
            if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
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
            if (timer)
            {
                timer->cancel();
                timer = nullptr;
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