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
#include <regex>
#include <chrono>
#include <fcntl.h>
#include "src/core/asyncio.h"
#include "src/util/log.h"
#include "src/util/configinit.h"
#include "src/util/config_parser.h"
#include "src/http/httpserver.h"
#ifdef USE_GUMBO
#include <gumbo.h>
#endif

// http server
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        HttpServer::HttpServer(bool keepAlive, IOManager *manager, IOManager *acceptmanager)
            : TcpServer(manager, acceptmanager),
              m_keepAlive(keepAlive)
        {
            m_router.reset(new RouterDispatch());
        }

        bool HttpServer::ShutDownServer()
        {
            // 设置 http server 停止标识
            m_shutdown.store(true, std::memory_order_release);

            // 停止 accept 循环
            m_isStop.store(true, std::memory_order_release);

            // 关闭所有 accept fd
            this->closeAllFds();

            // 关闭已经成功连接的 socket fd
            for (auto wptr : m_clients)
            {
                if (wptr.expired())
                {
                    continue;
                }
                auto ptr = wptr.lock();
                if (ptr)
                {
                    ptr->cancelAll();
                    ptr->shutdown(SHUT_RDWR);
                    ptr->close();
                }
            }
            m_clients.clear();
            return true;
        }

        void HttpServer::Register(HttpMethod method, const std::string &path, Handle handle)
        {
            if (path.back() == '*')
            {
                m_router->addBlurRouter(method, path, handle);
                return;
            }
            m_router->addBlurRouter(method, path, handle);
        }

        Task<void> HttpServer::handleClient(MSocket::MSocketPtr sock)
        {
            m_clients.push_back(sock);
            auto remoteAddress = std::dynamic_pointer_cast<IPAddress>(sock->getRemoteAddress());
            auto localAddress = std::dynamic_pointer_cast<IPAddress>(sock->getLocalAddress());
            setIpAndPort(remoteAddress, m_remoteIP, m_remotePort);
            setIpAndPort(localAddress, m_localIp, m_localPort);

            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << remoteAddress->toString() << " ip : " << m_remoteIP << " port : " << m_remotePort;
            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << localAddress->toString()  << " ip : " << m_localIp  << " port : " << m_localPort;

            char first_byte;
            int peek_ret = co_await Recv(sock->getSocketfd(), &first_byte, 1, MSG_PEEK | MSG_DONTWAIT);

            std::shared_ptr<HttpSession> session;
            if (peek_ret == 1 && first_byte == 0x16)
            {
                auto ssl_sock = std::make_shared<SSLSocket>(sock, true, true);
                if (!ssl_sock->isValid())
                {
                    BLUE_LOG_ERROR(g_logger) << "SSLSocket creation failed (cert not found?)";
                    sock->close();
                    this->subConnection();
                    co_return;
                }
                bool tem = co_await ssl_sock->handshake();
                if (!tem)
                {
                    BLUE_LOG_ERROR(g_logger) << "SSL handshake failed";
                    sock->close();
                    this->subConnection();
                    co_return;
                }
                session = std::make_shared<HttpSession>(ssl_sock);
            }
            else
            {
                auto stream = std::make_shared<SocketStream>(sock);
                session = std::make_shared<HttpSession>(stream);
            }

            bool temkeepAlive = m_keepAlive;
            do
            {
                // 若双方有一个是长连接就长连接
                // 错误由recvRequest处理
                auto [recvstatus, requestPtr] = co_await session->recvRequest();
                if (recvstatus == http::HttpSession::RecvStatus::ERROR ||
                    recvstatus == http::HttpSession::RecvStatus::CLOSE)
                {
                    break;
                }

                auto responsePtr = std::make_shared<HttpResponse>(requestPtr->getVersion(),
                                                                  (requestPtr->isKeepAlive() || temkeepAlive));
                temkeepAlive = (requestPtr->isKeepAlive() || temkeepAlive);

                // 构建 Context
                auto ctx = std::make_shared<HttpContext>(requestPtr, responsePtr);

                // 构建链: 中间件 + 路由目标 Handle
                MiddlewareChain chain;
                for (auto &mw : m_middlewares)
                {
                    chain.Use(mw);
                }

                // 设置最后的用户 Handle
                chain.SetFinal([this](HttpContext &c) -> void
                               { m_router->handle(c.Request(), c.Response()); });

                // 开始运行 Chain
                co_await chain.Run(*ctx);

                // 补上 body 的长度
                if (!responsePtr->hasHeader("Content-Length"))
                {
                    responsePtr->setHeader("Content-Length", std::to_string(responsePtr->getBody().size()));
                }

                co_await session->sendResponse(responsePtr, requestPtr);
                BLUE_LOG_INFO(g_logger) << sock->getRemoteAddress()->toString()
                                        << " \"" << http::HttpMethodToChars(requestPtr->getMethod())
                                        << " " << requestPtr->getPath()
                                        << " HTTP/" << requestPtr->versionToStr() << "\" "
                                        << http::HttpStatusToChars(responsePtr->getStatus())
                                        << " " << blue::util::ConfigParser::FormatSize(responsePtr->getBody().size())
                                        << requestPtr->getHeader("User-Agent");
            } while (temkeepAlive);

            session->close();
            this->subConnection();
            co_return;
        }

        void HttpServer::setIpAndPort(blue::IPAddress::IPAddressPtr address, std::string &ip, std::string &port)
        {
            if (address->getFamily() == AF_INET)
            {
                ip = std::dynamic_pointer_cast<IPv4Address>(address)->getIp();
            }
            else if (address->getFamily() == AF_INET6)
            {
                ip = std::dynamic_pointer_cast<IPv6Address>(address)->getIp();
            }
            else
            {
                ip = std::dynamic_pointer_cast<UnknowAddress>(address)->getIp();
            }
            port = std::to_string(address->getPort());
        }
    }
}