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
#include <chrono>
#include "config.h"
#include "log.h"
#include "tcpServer.h"
#include "await.h"

// tcp server
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    TcpServer::TcpServer(IOManager *manager, IOManager *acceptmanager)
        : m_worker(manager),
          m_acceptworker(acceptmanager)
    {
    }

    TcpServer::~TcpServer()
    {
        if (m_worker)
        {
            delete m_worker;
        }
        m_isStop.store(true, std::memory_order_release);
        if (m_socks.empty())
        {
            return;
        }
        closeAllFds();
    }

    void TcpServer::closeAllFds()
    {
        for (auto &sock : m_socks)
        {
            sock->cancelAll();
            sock->shutdown(SHUT_RDWR); // 关闭读写端不在接收连接
            sock->close();
        }
        m_socks.clear();
    }

    Task<void> TcpServer::startAccept(MSocket::MSocketPtr sock)
    {
        auto self = this->shared_from_this();
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
                co_await sleepFor(60); // 等待一分钟再试
                continue;
            }
            MSocket::MSocketPtr client = co_await sock->acceptT(500);
            if (client)
            {
                // 设置1s的socket fd上的超时
                client->setRecvTimeout(1000);
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
                // 超时不显示
                if (errno == ETIMEDOUT)
                {
                    co_await sleepFor(1);
                    continue;
                }
                BLUE_LOG_ERROR(g_logger) << "tcp accept failed error : " << errno
                                         << " strerror : " << strerror(errno);

                co_await sleepFor(1);
            }
        }
        co_return;
    }

    Task<bool> TcpServer::start()
    {
        if (!m_isStop.load(std::memory_order_acquire))
        {
            co_return true;
        }
        m_isStop.store(false, std::memory_order_release);
        for (auto &sock : m_socks)
        {
            m_acceptworker->schedule(startAccept(sock));
        }
        co_return true;
    }

    Task<bool> TcpServer::stop()
    {
        if (m_connections.load(std::memory_order_acquire))
        {
            co_return false;
        }
        m_isStop.store(true, std::memory_order_release);
        closeAllFds();
        co_return true;
    }

    Task<void> TcpServer::handleClient(MSocket::MSocketPtr sock)
    {
        // auto self = this->shared_from_this();
        BLUE_LOG_INFO(g_logger) << "handleClient : " << sock->toString();
        char buf[1024];
        while (true)
        {
            ssize_t n = co_await sock->recv(buf, sizeof(buf));
            if (n <= 0)
            {
                BLUE_LOG_INFO(g_logger) << "client closed, breaking";
                break;
            }
            ssize_t sent = co_await sock->send(buf, n);
            if (sent <= 0)
            {
                break;
            }
        }
        sock->close();
        BLUE_LOG_INFO(g_logger) << "handleClient done";
        co_return;
    }
}