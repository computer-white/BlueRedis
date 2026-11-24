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
 * @file httpservlet.h
 * @brief http servlet模块,用于url路由，根据不同url返回不同响应
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.5.14
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_HTTP_HTTPSERVLET_H
#define BLUE_HttP_HTTPSERVLET_H
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "src/util/mthread.h"
#include "src/http/http.h"
#include "src/http/httpsession.h"

namespace blue
{
    namespace http
    {
        class Router
        {
        public:
            using RouterPtr = std::shared_ptr<Router>;

        public:
            Router() = default;
            virtual ~Router() = default;

            /**
             * @brief 关键函数,实现对request中url的路由后选择合适的子类进行处理
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             */
            virtual void handle(http::HttpRequest::HttpRequestPtr request,
                                http::HttpResponse::HttpResponsePtr response) = 0;
        };

        class FunctionRouter : public Router
        {
        public:
            using FunctionRouterPtr = std::shared_ptr<FunctionRouter>;
            using CallBack = std::function<void(http::HttpRequest::HttpRequestPtr request,
                                                http::HttpResponse::HttpResponsePtr response)>;

        public:
            /**
             * @brief 将匹配成功的url与cb函数对应
             * @param cb 对于不同匹配成功的url选择和设置的不同的回调函数
             */
            FunctionRouter(CallBack cb);

            /**
             * @brief 关键函数,实现对request中url的路由后传递给用户设置的回调函数中进行处理
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             */
            virtual void handle(http::HttpRequest::HttpRequestPtr request,
                                http::HttpResponse::HttpResponsePtr response) override;

        private:
            CallBack m_cb;
        };

        class RouterDispatch : public Router
        {
        public:
            using RouterDispatchPtr = std::shared_ptr<RouterDispatch>;
            using PathRouter = std::unordered_map<std::string, std::shared_ptr<Router>>;
            using MethodRouter = std::map<HttpMethod, PathRouter>;

        public:
            /**
             * @brief 实现对url的精准和模糊匹配并派遣相应处理的Router
             */
            RouterDispatch();

            /**
             * @brief 关键函数,实现对request中url进行模糊和精准匹配后选择合适的Router对象进行处理
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             * @note 若模糊和精准都没匹配上则选择使用默认servlet(404 not found)
             */
            virtual void handle(http::HttpRequest::HttpRequestPtr request,
                                http::HttpResponse::HttpResponsePtr response) override;

            /**
             * @brief 添加 method 方法 对应的精准 path 的 router
             */
            void addRouter(HttpMethod method, const std::string &path, std::shared_ptr<Router> router);

            /**
             * @brief 添加 method 方法 对应的精准 path 的 cb
             */
            void addRouter(HttpMethod method, const std::string &path, FunctionRouter::CallBack cb);

            /**
             * @brief 添加 method 方法 对应的模糊 path 的 router
             */
            void addBlurRouter(HttpMethod method, const std::string &path, std::shared_ptr<Router> router);

            /**
             * @brief 添加 method 方法 对应的模糊 path 的 cb
             */
            void addBlurRouter(HttpMethod method, const std::string &path, FunctionRouter::CallBack cb);

            /**
             * @brief 删除 method 方法对应的所有精准 router
             */
            void delRouter(HttpMethod method);

            /**
             * @brief 删除 method 方法对应的所有模糊 router
             */
            void delBlurRouter(HttpMethod method);

            /**
             * @brief 删除 method 方法对应的精准 path 的 router
             */
            void delRouterWithPath(HttpMethod method, const std::string &path);

            /**
             * @brief 删除 method 方法对应的模糊 path 的 router
             */
            void delBlurRouterWithPath(HttpMethod method, const std::string &path);

            /**
             * @brief 找出匹配 method 方法对应的 path 的 router
             * @note 优先匹配精准
             */
            std::shared_ptr<Router> matchRouter(HttpMethod method, const std::string &path);

            /**
             * @brief 获取默认处理servlet智能指针
             * @return 默认处理servlet智能指针
             */
            std::shared_ptr<Router> getDefault() const { return m_default; }

            /**
             * @brief 设置默认处理servlet智能指针
             * @param defval 想要设置的默认的servlet智能指针
             */
            void setDefault(std::shared_ptr<Router> defval) { m_default = defval; }

        private:
            std::mutex m_mutex;
            MethodRouter m_pinpointRouter; // 精准匹配
            MethodRouter m_blurRouter;     // 模糊匹配
            std::shared_ptr<Router> m_default;
        };

        class NotFoundRouter : public Router
        {
        public:
            using NotFoundRouterPtr = std::shared_ptr<NotFoundRouter>;

        public:
            NotFoundRouter() = default;

            /**
             * @brief 关键函数,实现对request中url进行模糊和精准匹配后没有匹配到处理函数
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             */
            virtual void handle(http::HttpRequest::HttpRequestPtr request,
                                http::HttpResponse::HttpResponsePtr response) override;
        };
    }
}

#endif