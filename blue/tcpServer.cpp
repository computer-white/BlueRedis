#include <chrono>
#include "config.h"
#include "log.h"
#include "tcpServer.h"
#include "await.h"

// tcp server
namespace blue
{
    static blue::ConfigVar<uint64_t>::ConfigVarPtr g_tcp_server_read_timeout =
        blue::Config::Lookup<uint64_t>("tcp_server.read_timeout",
                                       (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    
    template <typename T>
    TcpServer<T>::TcpServer(int level, int option_name, T option, IOManager *manager, IOManager *acceptmanager)
        : m_worker(manager),
          m_acceptworker(acceptmanager),
          m_name("blue/1.0.0"),
          m_RecvTimeOut(g_tcp_server_read_timeout->getValue()),
          m_level(level),
          m_option_name(option_name),
          m_option(std::move(option))
    {
    }

    template <typename T>
    TcpServer<T>::~TcpServer()
    {
        BLUE_LOG_INFO(g_logger) << "~TcpServer";
        m_isStop.store(true,std::memory_order_release);
        if (m_socks.empty())
        {
            BLUE_LOG_INFO(g_logger) << "m_socks is empty";
        }
        for (auto &sock : m_socks)
        {
            sock->cancelAll();
            sock->shutdown(SHUT_RDWR);
            sock->close();
        }
        m_socks.clear();
    }

    template <typename T>
    bool TcpServer<T>::bind(Address::AddressPtr address)
    {
        std::vector<Address::AddressPtr> adds, fails;
        adds.push_back(address);
        return bind(adds, fails);
    }

    template <typename T>
    bool TcpServer<T>::bind(std::vector<Address::AddressPtr> &address, std::vector<Address::AddressPtr> &fails)
    {
        for (auto &add : address)
        {
            MSocket::MSocketPtr sock = MSocket::CreateTcp(add);
            if (!sock->bind(add))
            {
                BLUE_LOG_ERROR(g_logger) << "tcp server bind error : " << errno
                                         << " strerror : " << strerror(errno)
                                         << " addr : [" << add->toString();
                fails.push_back(add);
                continue;
            }
            if (m_level != -1 && m_option_name != -1 
                && m_option_name != SO_REUSEADDR 
                && m_option_name != SO_REUSEPORT 
                && m_option_name != (SO_REUSEADDR | SO_REUSEPORT))
            {
                sock->setOption(m_level,m_option_name,m_option);
            }
            if (!sock->listen())
            {
                BLUE_LOG_ERROR(g_logger) << "tcp server listen error : " << errno
                                         << " strerror : " << strerror(errno)
                                         << " addr : [" << add->toString();
                fails.push_back(add);
                continue;
            }
            m_socks.push_back(sock);
        }
        if (!fails.empty())
        {
            m_socks.clear();
            return false;
        }

        for (auto &sock : m_socks)
        {
            BLUE_LOG_INFO(g_logger) << "tcp server bind success : " << sock->toString();
        }
        return true;
    }

    template <typename T>
    Task<void> TcpServer<T>::startAccept(MSocket::MSocketPtr sock)
    {
        sock->setNoBlocking();
        // 处在连接状态
        while (!m_isStop.load(std::memory_order_acquire))
        {
            // 连接限制
            if (m_connections.load(std::memory_order_acquire) >= getMaxClientCount())
            {
                BLUE_LOG_WARN(g_logger) << "Max clients reached: " << m_connections.load(std::memory_order_acquire) 
                                    << "/" << getMaxClientCount();
                m_rejected_connections++;
                co_await sleepFor(60);  // 等待一分钟再试
                continue;
            }
            MSocket::MSocketPtr client = co_await sock->acceptT(500);
            if (client)
            {
                BLUE_LOG_INFO(g_logger) << "accept new client, ptr=" << client.get() 
                            << " fd=" << client->getSocketfd();
                client->setRecvTimeout(m_RecvTimeOut);
                client->setNoBlocking();
                m_worker->schedule(handleClient(client));
                addConnection();
            }
            else
            {
                // 检查是否因为 socket 关闭导致的错误
                if (errno == EBADF || errno == EINVAL || sock->getSocketfd() < 0)
                {
                    BLUE_LOG_ERROR(g_logger) << "socket closed, stop accept";
                    m_isStop.store(true, std::memory_order_release);
                    break;
                }
                
                BLUE_LOG_ERROR(g_logger) << "tcp accept failed error : " << errno
                                        << " strerror : " << strerror(errno);

                co_await sleepFor(1);
            }
        }
        co_return;
    }

    template <typename T>
    Task<bool> TcpServer<T>::start()
    {
        if (!m_isStop.load(std::memory_order_acquire))
        {
            co_return true;
        }
        m_isStop.store(false,std::memory_order_release);
        for (auto &sock : m_socks)
        {
            m_acceptworker->schedule(startAccept(sock));
        }
        co_return true;
    }

    template <typename T>
    Task<bool> TcpServer<T>::stop()
    {
        if (m_connections.load(std::memory_order_acquire))
        {
            co_return false;
        }
        m_isStop.store(true,std::memory_order_release);
        for (auto& sock : m_socks)
        {
            sock->cancelAll();
            sock->shutdown(SHUT_RDWR);        // 关闭读写端不在接收连接
            sock->close();
        }
        m_socks.clear();
        co_return true;
    }

    template <typename T>
    Task<void> TcpServer<T>::handleClient(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(g_logger) << "handleClient : " << sock->toString();
        char buf[1024];
        while (true) {
            ssize_t n = co_await sock->recv(buf, sizeof(buf));
            BLUE_LOG_INFO(g_logger) << "recv returned: n=" << n 
                                    << " errno=" << errno 
                                    << " strerror=" << strerror(errno);
            if (n <= 0) {
                BLUE_LOG_INFO(g_logger) << "client closed, breaking";
                break;
            }
            ssize_t sent = co_await sock->send(buf, n);
            BLUE_LOG_INFO(g_logger) << "send returned: sent=" << sent;
            if (sent <= 0) break;
        }
        sock->close();
        BLUE_LOG_INFO(g_logger) << "handleClient done";
        co_return;
    }

    template class blue::TcpServer<int>;
    template class blue::TcpServer<timeval>;
}