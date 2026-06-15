#ifndef BLUE_HTTP_HTTPSERVER_H
#define BLUE_HTTP_HTTPSERVER_H
#define USE_GUMBO
#include <random>
#include "httpconnection.h"
#include "httpsession.h"
#include "httpservlet.h"
#include "blue/tcpServer.h"
#include "blue/task.h"

// http server
namespace blue
{
    namespace http
    {
        /* 服务器端 */
        template <typename T>
        class HttpServer : public TcpServer<T>
        {
        public:
            using HttpServerPtr = std::shared_ptr<HttpServer<T>>;

        public:
            /**
             * @brief http服务构造函数
             * @param keepAlive 服务器是否选择保持长连接,true表示保持长连接
             * @param level 协议例如SOL_SOCKET 默认 -1
             * @param option_name 选项名称 例如SO_REUSEADDR 默认 -1
             * @param option 选项值 1开启 0禁用 默认 0
             * @param manager 工作iomanager
             * @param acceptmanager 处理accpet的iomanager
             */
            HttpServer(bool keepAlive = false, int level = -1, int option_name = -1, T option = T(), IOManager *manager = IOManager::GetThis(),
                       IOManager *acceptmanager = IOManager::GetThis());

            /**
             * @brief 获取servlet diapatch的智能指针对象
             * @return servlet diapatch的智能指针对象
             */
            std::shared_ptr<ServletDispatch> getDispatch() const { return m_dispatch; }

            /**
             * @brief 设置新的servlet dispatch
             * @param dispatch 新的servlet dispatch对象智能指针
             */
            void setDispatch(std::shared_ptr<ServletDispatch> dispatch) { m_dispatch = dispatch; }

        protected:
            /**
             * @brief 处理客户端和服务端的请求和响应
             * @param sock 客户端socket对象智能指针
             */
            virtual Task<void> handleClient(MSocket::MSocketPtr sock) override;

        private:
            /**
             * @brief 处理webSocket
             * @param sock socket 对象指针
             * @param request 拿到的解析后得http请求智能指针
             * @param response 响应智能指针
             */
            Task<void> _handleWebSocket(MSocket::MSocketPtr sock, HttpRequest::HttpRequestPtr request, HttpResponse::HttpResponsePtr response, std::string targeturl);

            /**
             * @brief 处理web管理页面(请求localhost:8020/admin)
             * @param request 拿到的解析后得http请求智能指针
             * @param response 响应智能指针
             * @param session 会话
             */
            void _handleAdmin(HttpRequest::HttpRequestPtr request, HttpResponse::HttpResponsePtr response, HttpSession::HttpSessionPtr session);

            /**
             * @brief 处理隧道连接
             * @param sock socket 对象指针
             * @param request 拿到的解析后得http请求智能指针
             */
            Task<void> _handleConnect(MSocket::MSocketPtr sock,
                                      HttpRequest::HttpRequestPtr request);
            /**
             * @brief 将客户端请求头部信息设置到headers中作为我们代理发给其他目标的header
             * @param request 拿到的解析后得http请求智能指针
             * @param headers 需要被设置请求头部信息
             */
            void PrepareHeaders(HttpRequest::HttpRequestPtr request, std::map<std::string, std::string> &headers);

            /**
             * @brief 设置ip和port
             * @param address ip地址
             * @param ip 需要被设置的ip
             * @param port 需要被设置的port
             */
            void _setIpAndPort(blue::IPAddress::IPAddressPtr address, std::string &ip, std::string &port);

            /**
             * @brief 转发远端请求到target
             * @param request 原始请求
             * @param targeturl 目标url
             */
            Task<void> _forwardRequest(HttpRequest::HttpRequestPtr request, HttpResponse::HttpResponsePtr response, std::string targeturl, bool isForwardProxy = false);

        private:
            // 辅助方法(_forwardRequest)

            /**
             * @brief 尝试redis缓存
             * @return tuple(cache_key,use_cache,cached)
             */
            std::tuple<std::string, bool, std::string> tryCache(const std::string &targetUrl, HttpResponse::HttpResponsePtr response, HttpMethod method, bool isForwardProxy);

            /**
             * @brief 设置X-Forwarded-For
             */
            void setXForwardedFor(const std::string &targeturl, HttpRequest::HttpRequestPtr request, HttpResponse::HttpResponsePtr response);

            /**
             * @brief 获取连接池
             */
            HttpConnectionPool::HttpConnectionPoolPtr getConnectionPool(blue::Url::UrlPtr UrlPtr);

            /**
             * @brief 写入日志到数据库
             */
            void logRequest(HttpRequest::HttpRequestPtr request, const std::string &targeturl, blue::Url::UrlPtr UrlPtr, std::shared_ptr<HttpResult> result, int64_t duration, bool isForardProxy);

            /**
             * @brief 处理反向代理
             */
            void processResponseContent(HttpResponse::HttpResponsePtr response, std::shared_ptr<HttpResult> result, const std::string &targeturl, int status);

            /**
             * @brief 判断是否是重定向
             */
            bool isRedirect(int status);

            /**
             * @brief 处理重定向
             */
            bool handleRedirect(HttpResponse::HttpResponsePtr response,
                                const std::string &location, const std::string &targeturl,
                                const std::string &cache_key, bool use_cache,
                                int status);

        private:
            bool isAdminAuthenticated(HttpRequest::HttpRequestPtr request)
            {
                auto cookie = request->getHeader("Cookie");
                if (cookie.empty())
                {
                    return false;
                }

                // 查找 session token
                size_t pos = cookie.find("admin_token=");
                if (pos == std::string::npos)
                {
                    return false;
                }

                pos += 12;
                size_t end = cookie.find(";", pos);
                std::string token = cookie.substr(pos, end - pos);

                // 验证 token（简单实现）
                return token == m_admin_token;
            }

            void setAdminCookie(HttpResponse::HttpResponsePtr response)
            {
                std::string token = generateToken();
                m_admin_token = token;
                response->setHeader("Set-Cookie", "admin_token=" + token + "; Path=/; HttpOnly");
            }

            std::string generateToken()
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_int_distribution<> dis(0, 15);

                const char *hex = "0123456789abcdef";
                std::string token;
                for (int i = 0; i < 32; i++)
                {
                    token += hex[dis(gen)];
                }
                return token;
            }

        private:
            std::atomic<bool> m_shutdown{false};
            std::string m_admin_token;
            std::string m_admin_password = "admin123"; // 可从配置读取
        private:
            bool m_keepAlive;
            std::string m_remoteIP = "";
            std::string m_remotePort = "";
            std::string m_localIp = "";
            std::string m_localPort = "";
            std::shared_ptr<ServletDispatch> m_dispatch;
        };

    }
}

#endif