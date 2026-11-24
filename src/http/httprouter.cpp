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
#include <fnmatch.h>
#include "src/http/httprouter.h"

// servlet
namespace blue
{
    namespace http
    {
        FunctionRouter::FunctionRouter(CallBack cb)
            : m_cb(cb)
        {
        }

        void FunctionRouter::handle(http::HttpRequest::HttpRequestPtr request,
                                    http::HttpResponse::HttpResponsePtr response)
        {
            m_cb(request, response);
        }

        RouterDispatch::RouterDispatch()
        {
            m_default.reset(new NotFoundRouter());
        }

        void RouterDispatch::handle(http::HttpRequest::HttpRequestPtr request,
                                    http::HttpResponse::HttpResponsePtr response)
        {
            auto router = matchRouter(request->getMethod(), request->getPath());
            if (router)
            {
                return router->handle(request, response);
            }
            return;
        }

        void RouterDispatch::addRouter(HttpMethod method, const std::string &path, std::shared_ptr<Router> router)
        {
            std::unordered_map<std::string, std::shared_ptr<Router>> tem;
            tem[path] = router;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pinpointRouter[method] = tem;
        }

        void RouterDispatch::addRouter(HttpMethod method, const std::string &path, FunctionRouter::CallBack cb)
        {
            auto router = std::make_shared<FunctionRouter>(cb);
            std::unordered_map<std::string, std::shared_ptr<Router>> tem;
            tem[path] = router;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pinpointRouter[method] = tem;
        }
        void RouterDispatch::addBlurRouter(HttpMethod method, const std::string &path, std::shared_ptr<Router> router)
        {
            std::unordered_map<std::string, std::shared_ptr<Router>> tem;
            tem[path] = router;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_blurRouter[method] = tem;
        }
        void RouterDispatch::addBlurRouter(HttpMethod method, const std::string &path, FunctionRouter::CallBack cb)
        {
            auto router = std::make_shared<FunctionRouter>(cb);
            std::unordered_map<std::string, std::shared_ptr<Router>> tem;
            tem[path] = router;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_blurRouter[method] = tem;
        }
        void RouterDispatch::delRouter(HttpMethod method)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pinpointRouter.find(method);
            if (it == m_pinpointRouter.end())
            {
                return;
            }
            m_pinpointRouter.erase(it);
        }
        void RouterDispatch::delBlurRouter(HttpMethod method)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_blurRouter.find(method);
            if (it == m_blurRouter.end())
            {
                return;
            }
            m_blurRouter.erase(it);
        }
        void RouterDispatch::delRouterWithPath(HttpMethod method, const std::string &path)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pinpointRouter.find(method);
            if (it == m_pinpointRouter.end())
            {
                return;
            }
            auto pathRouter = it->second;
            auto pathit = pathRouter.find(path);
            if (pathit == pathRouter.end())
            {
                return;
            }
            pathRouter.erase(pathit);
        }
        void RouterDispatch::delBlurRouterWithPath(HttpMethod method, const std::string &path)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_blurRouter.find(method);
            if (it == m_blurRouter.end())
            {
                return;
            }
            auto pathRouter = it->second;
            auto pathit = std::find_if(pathRouter.begin(), pathRouter.end(), [p = path](const auto &u)
                                       { return !(fnmatch(u.first.c_str(), p.c_str(), 0)); });
            if (pathit == pathRouter.end())
            {
                return;
            }
            pathRouter.erase(pathit);
        }

        std::shared_ptr<Router> RouterDispatch::matchRouter(HttpMethod method, const std::string &path)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pinpointRouter.find(method);
            if (it != m_pinpointRouter.end())
            {
                auto pathRouter = it->second;
                auto pathit = pathRouter.find(path);
                if (pathit != pathRouter.end())
                {
                    return pathit->second;
                }
            }

            auto blurit = m_blurRouter.find(method);
            if (blurit != m_blurRouter.end())
            {
                auto pathRouter = blurit->second;
                auto pathit = std::find_if(pathRouter.begin(), pathRouter.end(), [p = path](const auto& u){
                    return !(fnmatch(u.first.c_str(), p.c_str(), 0));
                });
                if (pathit != pathRouter.end())
                {
                    return pathit->second;
                }
            }

            return m_default;
        }

        void NotFoundRouter::handle(http::HttpRequest::HttpRequestPtr request,
                                    http::HttpResponse::HttpResponsePtr response)
        {
            static const char *page_404 =
                "<html>\r\n"
                "<head>\r\n"
                "<meta charset='UTF-8'>\r\n"
                "<title>404 Not Found</title>\r\n"
                "<style>\r\n"
                "body {\r\n"
                "    margin: 0;\r\n"
                "    padding: 0;\r\n"
                "    background-color: #f5f5f5;\r\n"
                "    font-family: Arial, sans-serif;\r\n"
                "    display: flex;\r\n"
                "    justify-content: center;\r\n"
                "    align-items: center;\r\n"
                "    height: 100vh;\r\n"
                "}\r\n"
                ".container {\r\n"
                "    text-align: center;\r\n"
                "}\r\n"
                "h1 {\r\n"
                "    font-size: 72px;\r\n"
                "    color: #333;\r\n"
                "    margin: 0;\r\n"
                "}\r\n"
                "p {\r\n"
                "    font-size: 18px;\r\n"
                "    color: #999;\r\n"
                "    margin-top: 10px;\r\n"
                "}\r\n"
                "hr {\r\n"
                "    width: 200px;\r\n"
                "    border: none;\r\n"
                "    border-top: 1px solid #ddd;\r\n"
                "    margin: 20px auto;\r\n"
                "}\r\n"
                ".server {\r\n"
                "    font-size: 14px;\r\n"
                "    color: #ccc;\r\n"
                "}\r\n"
                "</style>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "<div class='container'>\r\n"
                "<h1>404</h1>\r\n"
                "<p>Not Found</p>\r\n"
                "<hr>\r\n"
                "<div class='server'>blue/1.0.0</div>\r\n"
                "</div>\r\n"
                "</body>\r\n"
                "</html>\r\n";
            response->setStatus(blue::http::HttpStatus::NOT_FOUND);
            response->setHeader("Server", "blue/1.0.0");
            response->setHeader("Content-Type", "text/html");
            response->setHeader("Content-Length", std::to_string(strlen(page_404)));
            response->setBody(page_404);
            return;
        }
    }
}