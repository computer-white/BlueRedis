#include "config.h"
#include "log.h"
#include "tcpServer.h"
#include <chrono>

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
          m_option(std::move(option)),
          m_isStop(true)
    {
    }

    template <typename T>
    TcpServer<T>::~TcpServer()
    {
        for (auto &sock : m_socks)
        {
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
    void TcpServer<T>::startAccept(MSocket::MSocketPtr sock)
    {
        // 处在连接状态
        auto self = this->shared_from_this();
        while (!m_isStop)
        {
            MSocket::MSocketPtr client = sock->accept();
            if (client)
            {
                client->setRecvTimeout(m_RecvTimeOut);
                m_worker->schedule([s = self,c = client](){
                    s->handleClient(c);
                });
            }
            else
            {
                BLUE_LOG_ERROR(g_logger) << "tcp accept failed error : " << errno
                                         << " strerror : " << strerror(errno);
            }
        }
    }

    template <typename T>
    bool TcpServer<T>::start()
    {
        if (!m_isStop)
        {
            return true;
        }
        m_isStop = false;
        auto self = this->shared_from_this();
        for (auto &sock : m_socks)
        {
            m_acceptworker->schedule([s = self,so = sock](){
                s->startAccept(so);
            });
        }
        return true;
    }

    template <typename T>
    bool TcpServer<T>::stop()
    {
        m_isStop = true;
        auto self = this->shared_from_this();
        m_worker->schedule([s = self](){
            for (auto& sock : s->m_socks)
            {
                sock->cancelAll();
                sock->close();
            }
            s->m_socks.clear();
        });
        return true;
    }

    template <typename T>
    void TcpServer<T>::handleClient(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(g_logger) << "handleClient : " << sock->toString();
    }

    template class blue::TcpServer<int>;
    template class blue::TcpServer<timeval>;
}