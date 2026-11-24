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
 * @file httpserver.h
 * @brief http server模块,继承TcpServer,作为http服务器,支持wbsocket, 代理等等
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.5.17
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_HTTP_HTTPSERVER_H
#define BLUE_HTTP_HTTPSERVER_H
#define USE_GUMBO
#include <random>
#include "src/http/httpconnection.h"
#include "src/http/httpsession.h"
#include "src/http/httprouter.h"
#include "src/net/tcpServer.h"
#include "src/core/task.h"

// http server
namespace blue
{
    namespace http
    {
        /* 服务器端 */
        class HttpServer : public TcpServer
        {
        public:
            using HttpServerPtr = std::shared_ptr<HttpServer>;
            using Handle = std::function<void(HttpRequest::HttpRequestPtr req,
                                              HttpResponse::HttpResponsePtr resp)>;

        public:
            /**
             * @brief http服务构造函数
             * @param keepAlive 服务器是否选择保持长连接,true表示保持长连接
             * @param manager 工作iomanager
             * @param acceptmanager 处理accpet的iomanager
             */
            HttpServer(bool keepAlive = false, IOManager *manager = IOManager::GetThis(),
                       IOManager *acceptmanager = IOManager::GetThis());

            /**
             * @brief GET
             * @param path 路径
             * @param Handle 回调函数
             */
            void Get(const std::string &path, Handle handle) { Register(HttpMethod::GET, path, handle); }

            /**
             * @brief POST
             * @param path 路径
             * @param Handle 回调函数
             */
            void Post(const std::string &path, Handle handle) { Register(HttpMethod::POST, path, handle); }

            /**
             * @brief PUT
             * @param path 路径
             * @param Handle 回调函数
             */
            void Put(const std::string &path, Handle handle) { Register(HttpMethod::PUT, path, handle); }

            /**
             * @brief DELETE
             * @param path 路径
             * @param Handle 回调函数
             */
            void Delete(const std::string &path, Handle handle) { Register(HttpMethod::DELETE, path, handle); }

            /**
             * @brief PATCH
             * @param path 路径
             * @param Handle 回调函数
             */
            void Patch(const std::string &path, Handle handle) { Register(HttpMethod::PATCH, path, handle); }

            /**
             * @brief 注册
             * @param path 路径
             * @param Handle 回调函数
             */
            void Register(HttpMethod method, const std::string &path, Handle handle);

            /**
             * @brief 关闭服务器
             */
            bool ShutDownServer();

        protected:
            /**
             * @brief 处理客户端和服务端的请求和响应
             * @param sock 客户端socket对象智能指针
             */
            virtual Task<void> handleClient(MSocket::MSocketPtr sock) override;

            /**
             * @brief 设置ip和port
             * @param address ip地址
             * @param ip 需要被设置的ip
             * @param port 需要被设置的port
             */
            void setIpAndPort(blue::IPAddress::IPAddressPtr address, std::string &ip, std::string &port);

        private:
            bool m_keepAlive;
            std::string m_remoteIP = "";
            std::string m_remotePort = "";
            std::string m_localIp = "";
            std::string m_localPort = "";
            std::shared_ptr<RouterDispatch> m_router;
            std::atomic<bool> m_shutdown{false};

        private:
            std::vector<MSocket::MSocketWPtr> m_clients;
        };

    }
}

#endif