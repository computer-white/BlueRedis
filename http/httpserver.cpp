#include <regex>
#include <chrono>
#include <fcntl.h>
#include "blue/asyncio.h"
#include "blue/log.h"
#include "blue/dbmanager.h"
#include "blue/redismanager.h"
#include "proxy/rate_limiter.h"
#include "proxy/tunnel.h"
#include "proxy/url_rewriter.h"
#include "httpserver.h"
#ifdef USE_GUMBO
#include <gumbo.h>
#endif

// http server
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        // 全局连接池缓存
        static std::map<std::string, HttpConnectionPool::HttpConnectionPoolPtr> s_pools;
        static http::HttpConnectionPool::MmutexType s_poolMutex;

        extern std::string s_db_host;
        extern std::string s_db_user;
        extern std::string s_db_database;
        extern std::string s_db_passward;
        extern uint16_t s_db_port;
        extern blue::DbManager::DbManagerPtr s_dbmanager_ptr;

        extern std::string s_redis_host;
        extern uint16_t s_redis_port;
        extern std::string s_redis_passward;
        extern blue::RedisManager::RedisManagerPtr s_redismanager_ptr;

        extern uint64_t s_rate_limit;
        extern uint64_t s_rate_limit_expire;
        extern uint64_t s_cache_expire;

        extern uint64_t s_select_timeout;

        // 修改cookie的domain
        static std::string fix_cookie_domain(const std::string &set_cookie)
        {
            std::string result = set_cookie;

            // 1. 删除 domain=xxx 部分
            std::regex domain_re(R"(;\s*domain=[^;]*)", std::regex::icase);
            result = std::regex_replace(result, domain_re, "");

            // 2. 可选：设置 domain=localhost
            result += "; domain=localhost";

            // 3. 删除 Secure 标记（因为你是 http，不是 https）
            std::regex secure_re(R"(;\s*secure\b)", std::regex::icase);
            result = std::regex_replace(result, secure_re, "");

            return result;
        }

        template <typename T>
        HttpServer<T>::HttpServer(bool keepAlive, int level, int option_name, T option, IOManager *manager, IOManager *acceptmanager)
            : TcpServer<T>(level, option_name, option, manager, acceptmanager),
              m_keepAlive(keepAlive)
        {
            m_dispatch.reset(new ServletDispatch());
        }

        template <typename T>
        Task<void> HttpServer<T>::handleClient(MSocket::MSocketPtr sock)
        {
            auto remoteAddress = std::dynamic_pointer_cast<IPAddress>(sock->getRemoteAddress());
            auto localAddress = std::dynamic_pointer_cast<IPAddress>(sock->getLocalAddress());
            _setIpAndPort(remoteAddress, m_remoteIP, m_remotePort);
            _setIpAndPort(localAddress, m_localIp, m_localPort);

            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << remoteAddress->toString() << " ip : " << m_remoteIP << " port : " << m_remotePort;
            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << localAddress->toString()  << " ip : " << m_localIp  << " port : " << m_localPort;

            // ===== 检测 TLS，如果是就替换为 SSLSocket =====
            char first_byte;
            int peek_ret = co_await Recv(sock->getSocketfd(), &first_byte, 1, MSG_PEEK | MSG_DONTWAIT);

            std::shared_ptr<HttpSession> session;
            if (peek_ret == 1 && first_byte == 0x16)
            {
                // HTTPS：创建 SSLSocket，握手，然后直接当 SocketStream 用
                auto ssl_sock = std::make_shared<SSLSocket>(sock, true, true);
                if (!ssl_sock->isValid())
                {
                    BLUE_LOG_ERROR(g_logger) << "SSLSocket creation failed (cert not found?)";
                    sock->close();
                    TcpServer<T>::subConnection();
                    co_return;
                }
                bool tem = co_await ssl_sock->handshake();
                if (!tem)
                {
                    BLUE_LOG_ERROR(g_logger) << "SSL handshake failed";
                    sock->close();
                    TcpServer<T>::subConnection();
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
                auto responsePtr = std::make_shared<HttpResponse>(requestPtr->getVersion(), (requestPtr->isKeepAlive() || temkeepAlive));
                temkeepAlive = (requestPtr->isKeepAlive() || temkeepAlive);
                BLUE_LOG_INFO(g_logger) << "requestPtrKeepAlive: " << requestPtr->isKeepAlive() << " m_keepAlive: " << m_keepAlive;
                std::string path = requestPtr->getPath();
                std::string host = requestPtr->getHeader("Host");
                BLUE_LOG_INFO(g_logger) << "path : " << path << " host : " << host;
                std::string targeturl;
                std::string target_param = requestPtr->getParam("target", "");

                if (requestPtr->getMethod() == HttpMethod::CONNECT)
                {
                    BLUE_LOG_INFO(g_logger) << "CONNECT: " << requestPtr->getPath();
                    co_await _handleConnect(sock, requestPtr);
                    // 走外面减少connection并看服务是否需要停止,
                    // 在_handleConnect里面处理过sock的关闭但是close里面有保险措施,所以不怕重复调用close
                    break;
                }

                if (path == "/proxy.pac")
                {
                    // FindProxyForURL 浏览器每次请求调用
                    std::string pac = R"(function FindProxyForURL(url, host) {
                        // 本地地址直连,isPlainHostName判断是否是本机主机名
                        if (isPlainHostName(host) || host == "127.0.0.1" || host == "localhost")
                            return "DIRECT";
                        // 百度相关走代理
                        if (shExpMatch(host, "*.baidu.com") || shExpMatch(host, "*.bdstatic.com"))
                            return "PROXY localhost:8020";
                        // websocket
                        if (url.startsWith("ws://") || url.startsWith("wss://"))
                            return "PROXY localhost:8020";
                        // 其他直连
                        return "DIRECT";
                    })";
                    responsePtr->setBody(pac);
                    responsePtr->setHeader("Content-Type", "application/x-ns-proxy-autoconfig");
                    responsePtr->setHeader("Content-Length", std::to_string(pac.size()));
                    responsePtr->setStatus(HttpStatus::OK);
                }
                else if (path.find("/admin/") == 0 || path == "/admin")
                {
                    _handleAdmin(requestPtr, responsePtr, session);
                }
                else if (!target_param.empty())
                {
                    targeturl = target_param;
                    // 整个path带有/blue...
                    std::string extra = path;
                    size_t blue_pos = extra.find("/blue");
                    if (blue_pos != std::string::npos)
                    {
                        extra = extra.substr(blue_pos + strlen("/blue"));
                    }
                    if (!extra.empty() && extra != "/")
                    {
                        auto u = blue::Url::CreateUrl(targeturl);
                        if (u)
                        {
                            targeturl = u->getScheme() + "://" + u->getAuthority() + extra;
                            std::string q = u->getQuery();
                            if (!q.empty())
                                targeturl += "?" + q;
                        }
                    }
                    co_await _forwardRequest(requestPtr, responsePtr, targeturl, false);
                }

                // ===== 正向代理 =====
                // 正向代理：Host 不是 localhost，path 就是目标路径
                else if (!host.empty() &&
                         host.find("localhost") == std::string::npos &&
                         host.find("127.0.0.1") == std::string::npos)
                {
                    // websocket
                    std::string upgrade = requestPtr->getHeader("Upgrade");
                    if (strcasecmp(upgrade.c_str(), "websocket") == 0)
                    {
                        BLUE_LOG_INFO(g_logger) << "WebSocket upgrade: " << requestPtr->getPath();
                        if (path.find("http://") == 0 || path.find("https://") == 0)
                        {
                            targeturl = path;
                        }
                        else
                        {
                            targeturl = "http://" + host + path;
                            std::string query = requestPtr->getQuery();
                            if (!query.empty())
                                targeturl += "?" + query;
                        }
                        co_await _handleWebSocket(sock, requestPtr, responsePtr, targeturl);
                        break;
                    }
                    // 正常正向代理
                    if (path.find("http://") == 0 || path.find("https://") == 0)
                    {
                        targeturl = path;
                    }
                    else
                    {
                        targeturl = "http://" + host + path;
                        std::string query = requestPtr->getQuery();
                        if (!query.empty())
                            targeturl += "?" + query;
                    }
                    co_await _forwardRequest(requestPtr, responsePtr, targeturl, true);
                }

                // ===== 反向代理（路径前缀模式）=====
                else if (path.find("/blue/") == 0)
                {
                    size_t scheme_pos = path.find("http://");
                    if (scheme_pos == std::string::npos)
                    {
                        scheme_pos = path.find("https://");
                    }
                    if (scheme_pos != std::string::npos)
                    {
                        // /blue/xxx/https://www.baidu.com/news
                        //              ↑ scheme_pos
                        targeturl = path.substr(scheme_pos); // "https://www.baidu.com/news"

                        // 提取中间路径：/blue 和 scheme 之间的部分
                        std::string middle = path.substr(strlen("/blue"), scheme_pos - strlen("/blue"));
                        BLUE_LOG_INFO(g_logger) << "middle : " << middle;
                        // middle = "/xxx"
                        // 如果 middle 不为空，拼到 target URL 的 path 上
                        if (!middle.empty() && middle != "/")
                        {
                            // 去掉 middle 尾部斜杠
                            while (!middle.empty() && middle.back() == '/')
                            {
                                middle.pop_back();
                            }
                            // 去掉 middle 首部斜杠
                            if (!middle.empty() && middle.front() == '/')
                            {
                                middle.erase(0, 1);
                            }

                            auto u = blue::Url::CreateUrl(targeturl);
                            if (u)
                            {
                                // 用 middle 作为实际路径
                                targeturl = u->getScheme() + "://" + u->getAuthority() + "/" + middle;
                                std::string q = u->getQuery();
                                if (!q.empty())
                                    targeturl += "?" + q;
                            }
                        }
                        co_await _forwardRequest(requestPtr, responsePtr, targeturl, false);
                    }
                    else
                    {
                        m_dispatch->handle(requestPtr, responsePtr, session);
                    }
                }
                else
                {
                    m_dispatch->handle(requestPtr, responsePtr, session);
                }
                // 自动补全缺失的响应头
                if (responsePtr->getHeader("Content-Type").empty())
                {
                    responsePtr->setHeader("Content-Type", "text/html; charset=utf-8");
                }
                if (responsePtr->getHeader("Content-Length").empty())
                {
                    responsePtr->setHeader("Content-Length", std::to_string(responsePtr->getBody().size()));
                }
                co_await session->sendResponse(responsePtr, requestPtr);
                BLUE_LOG_INFO(g_logger) << sock->getRemoteAddress()->toString()
                                        << " \"" << http::HttpMethodToChars(requestPtr->getMethod())
                                        << " " << requestPtr->getPath()
                                        << " HTTP/" << requestPtr->versionToStr() << "\" "
                                        << http::HttpStatusToChars(responsePtr->getStatus())
                                        << " " << responsePtr->getBody().size() << "B "
                                        << requestPtr->getHeader("User-Agent");
            } while (temkeepAlive);
            session->close();
            TcpServer<T>::subConnection();
            // 如果正在关闭且没有活跃连接，停止服务器
            if (m_shutdown.load(std::memory_order_acquire) && TcpServer<T>::getConnection() == 0)
            {
                bool end = co_await TcpServer<T>::stop();
                if (end)
                {
                    BLUE_LOG_INFO(xx::g_logger) << "tcpserver stoped";
                }
            }
            co_return;
        }

        template <typename T>
        void HttpServer<T>::_setIpAndPort(blue::IPAddress::IPAddressPtr address, std::string &ip, std::string &port)
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

        template <typename T>
        Task<void> HttpServer<T>::_forwardRequest(HttpRequest::HttpRequestPtr request,
                                                  HttpResponse::HttpResponsePtr response,
                                                  std::string targeturl,
                                                  bool isForwardProxy)
        {
            // 尝试redis缓存
            const auto &[cache_key, use_cache, cached] = tryCache(targeturl, response, request->getMethod(), isForwardProxy);
            if (use_cache && !cached.empty())
            {
                co_return;
            }

            // 对同一个客户端进行限流
            blue::proxy::RateLimiter::instance().setLimit(s_rate_limit);
            blue::proxy::RateLimiter::instance().setExpire(s_rate_limit_expire);
            if (!blue::proxy::RateLimiter::instance().allow(m_remoteIP))
            {
                response->setStatus(blue::http::HttpStatus::TOO_MANY_REQUESTS);
                response->setBody("Rate limit exceeded");
                co_return;
            }

            // 设置下游(client)ip(remoteip)
            setXForwardedFor(targeturl, request, response);

            // 解析目标url
            auto UrlPtr = blue::Url::CreateUrl(targeturl);
            if (!UrlPtr)
            {
                response->setStatus(blue::http::HttpStatus::BAD_REQUEST);
                response->setBody("invalid target url");
                co_return;
            }

            // 将客户端的请求中的header拿出来作为我们发给targeturl的header
            std::map<std::string, std::string> headers;
            PrepareHeaders(request, headers);

            // 从连接池拿连接
            auto pool = getConnectionPool(UrlPtr);

            // 转发
            auto now = std::chrono::steady_clock::now();
            // 用连接池发请求（复用连接）
            auto result = co_await pool->doRequest(request->getMethod(), UrlPtr, 5000, headers, request->getBody());
            auto end = std::chrono::steady_clock::now();
            int64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();

            // 记录日志
            logRequest(request, targeturl, UrlPtr, result, duration, isForwardProxy);

            // 处理结果
            if (result->result == (int)blue::http::HttpResult::ResultStatus::OK && result->response)
            {
                for (auto &header : result->response->getHeaders())
                {
                    response->setHeader(header.first, header.second);
                }
                response->delHeader("Transfer-Encoding"); // 和 Content-Length 互斥
                int status = (int)result->response->getStatus();

                // ===== 正向代理：透传 + 存缓存 =====
                if (isForwardProxy)
                {
                    response->setBody(result->response->getBody());
                    response->setHeader("Content-Length", std::to_string(result->response->getBody().size()));
                    response->setStatus((blue::http::HttpStatus)status);
                    // 缓存响应（60 秒）
                    if (use_cache && status == 200)
                    {
                        s_redismanager_ptr->set(cache_key, result->response->getBody(), s_cache_expire);
                        response->setHeader("X-Cache", "MISS");
                    }
                    co_return;
                }

                // 反向代理：处理html和css
                if (isRedirect(status))
                {
                    std::string location = result->response->getHeader("Location");
                    if (!location.empty())
                    {
                        // 处理重定向和设置缓存
                        if (handleRedirect(response, location, targeturl, cache_key, use_cache, status))
                        {
                            co_return;
                        }
                    }
                }

                // 处理反向代理
                processResponseContent(response, result, targeturl, status);
                // 设置缓存
                if (use_cache)
                {
                    s_redismanager_ptr->set(cache_key, response->getBody(), s_cache_expire);
                }
            }
            else
            {
                response->setStatus(blue::http::HttpStatus::BAD_GATEWAY);
                response->setBody("forward failed: " + result->error);
            }
            co_return;
        }

        template <typename T>
        void HttpServer<T>::_handleAdmin(HttpRequest::HttpRequestPtr request,
                                         HttpResponse::HttpResponsePtr response,
                                         HttpSession::HttpSessionPtr session)
        {
            std::string path = request->getPath();

            // 处理登出
            if (path == "/admin/logout")
            {
                response->setHeader("Set-Cookie", "admin_token=; Path=/; Max-Age=0");
                response->setStatus(HttpStatus::FOUND);
                response->setHeader("Location", "/admin/login");
                return;
            }

            // 处理登录
            if (path == "/admin/login")
            {
                if (request->getMethod() == HttpMethod::POST)
                {
                    std::string password = request->getParam("password");
                    if (password == m_admin_password)
                    {
                        setAdminCookie(response);
                        response->setStatus(HttpStatus::FOUND);
                        response->setHeader("Location", "/admin/");
                        return;
                    }
                    else
                    {
                        response->setBody(R"(<html><body><h1>Invalid Password</h1>
                            <a href="/admin/login">Try again</a></body></html>)");
                        response->setStatus(HttpStatus::OK);
                        return;
                    }
                }

                // 显示登录页面
                std::string login_page = R"(<!DOCTYPE html>
                    <html><head><meta charset="UTF-8"><title>Admin Login</title>
                    <style>
                    *{margin:0;padding:0;box-sizing:border-box}
                    body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;display:flex;justify-content:center;align-items:center;height:100vh}
                    .card{background:#16213e;padding:40px;border-radius:8px;min-width:350px;box-shadow:0 4px 20px rgba(0,0,0,0.3)}
                    h1{color:#1677ff;margin-bottom:20px;text-align:center}
                    input{width:100%;padding:12px;margin:10px 0;background:#1a1a2e;border:1px solid #333;color:#e0e0e0;border-radius:4px;font-size:14px}
                    input:focus{outline:none;border-color:#1677ff}
                    button{width:100%;padding:12px;background:#1677ff;color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:16px;margin-top:10px}
                    button:hover{background:#0f3460}
                    .error{color:#e94560;margin-top:10px;text-align:center}
                    </style></head><body>
                    <div class="card">
                    <h1>🔐 Admin Login</h1>
                    <form method="POST">
                    <input type="password" name="password" placeholder="Password" autofocus>
                    <button type="submit">Login</button>
                    </form>
                    </div></body></html>)";
                response->setBody(login_page);
                response->setHeader("Content-Type", "text/html; charset=utf-8");
                response->setStatus(HttpStatus::OK);
                return;
            }

            // 检查认证
            if (!isAdminAuthenticated(request))
            {
                response->setStatus(HttpStatus::FOUND);
                response->setHeader("Location", "/admin/login");
                return;
            }

            // 处理 shutdown
            if (path == "/admin/shutdown")
            {
                if (request->getMethod() == HttpMethod::POST)
                {
                    BLUE_LOG_INFO(g_logger) << "Admin triggered shutdown from web interface";
                    m_shutdown.store(true, std::memory_order_release);

                    std::string response_body = R"(<!DOCTYPE html>
                        <html><head><meta charset="UTF-8"><title>Shutting Down</title>
                        <style>
                        *{margin:0;padding:0;box-sizing:border-box}
                        body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px;text-align:center}
                        .card{background:#16213e;padding:40px;border-radius:8px;max-width:500px;margin:100px auto}
                        h1{color:#e94560;margin-bottom:20px}
                        .info{background:#1a1a2e;padding:15px;border-radius:4px;margin:20px 0}
                        button{padding:10px 20px;background:#1677ff;color:#fff;border:none;border-radius:4px;cursor:pointer;margin-top:20px}
                        button:hover{background:#0f3460}
                        </style></head><body>
                        <div class="card">
                        <h1>🔐 Server Shutting Down</h1>
                        <p>Shutdown signal sent.</p>
                        <div class="info">📊 Current connections: )" +
                                                std::to_string(this->getConnection()) + R"(</div>
                        <p>Server will stop after all connections close.</p>
                        <a href="/"><button>Back to Home</button></a>
                        </div></body></html>)";

                    response->setBody(response_body);
                    response->setHeader("Content-Type", "text/html; charset=utf-8");
                    response->setStatus(HttpStatus::OK);
                    return;
                }

                // GET 请求：显示确认页面
                std::string confirm_page = R"(<!DOCTYPE html>
                    <html><head><meta charset="UTF-8"><title>Confirm Shutdown</title>
                    <style>
                    *{margin:0;padding:0;box-sizing:border-box}
                    body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                    .card{background:#16213e;padding:40px;border-radius:8px;max-width:500px;margin:50px auto;text-align:center}
                    h1{color:#e94560;margin-bottom:20px}
                    .info{background:#1a1a2e;padding:15px;border-radius:4px;margin:20px 0;font-size:16px}
                    .warning{color:#e94560;margin:15px 0}
                    button{padding:12px 24px;margin:10px;border:none;border-radius:4px;cursor:pointer;font-size:16px}
                    .btn-danger{background:#e94560;color:#fff}
                    .btn-cancel{background:#555;color:#fff}
                    .btn-danger:hover{background:#c73a52}
                    .btn-cancel:hover{background:#777}
                    </style></head><body>
                    <div class="card">
                    <h1>⚠️ Shutdown Server</h1>
                    <p>Are you sure you want to shutdown the proxy server?</p>
                    <div class="info">📊 Active Connections: )" +
                                           std::to_string(this->getConnection()) + R"(</div>
                    <div class="warning">⚠️ Shutdown will wait for all connections to complete.</div>
                    <form method="POST">
                    <button type="submit" class="btn-danger">✅ Yes, Shutdown Server</button>
                    <a href="/admin/"><button type="button" class="btn-cancel">❌ Cancel</button></a>
                    </form>
                    </div></body></html>)";

                response->setBody(confirm_page);
                response->setHeader("Content-Type", "text/html; charset=utf-8");
                response->setStatus(HttpStatus::OK);
                return;
            }

            std::string sub_path = request->getPath().substr(strlen("/admin"));
            if (sub_path.empty() || sub_path == "/")
            {
                sub_path = "/index";
            }
            std::string body;
            if (sub_path == "/api/stats")
            {
                body = "{\"labels\":[],\"values\":[]}";
                if (s_dbmanager_ptr)
                {
                    auto res = s_dbmanager_ptr->query(
                        "SELECT DATE_FORMAT(created_at,'%H:%i') as minute, COUNT(*) as cnt "
                        "FROM request_logs WHERE created_at > DATE_SUB(NOW(), INTERVAL 60 MINUTE) "
                        "GROUP BY minute ORDER BY minute");
                    if (res)
                    {
                        std::string labels = "[", values = "[";
                        MYSQL_ROW row;
                        bool first = true;
                        while ((row = mysql_fetch_row(res)))
                        {
                            if (!first)
                            {
                                labels += ",";
                                values += ",";
                            }
                            labels += "\"" + std::string(row[0]) + "\"";
                            values += std::string(row[1]);
                            first = false;
                        }
                        labels += "]";
                        values += "]";
                        body = "{\"labels\":" + labels + ",\"values\":" + values + "}";
                        mysql_free_result(res);
                    }
                }
                response->setBody(body);
                response->setHeader("Content-Type", "application/json");
                response->setHeader("Content-Length", std::to_string(body.size()));
                response->setStatus(blue::http::HttpStatus::OK);
                return;
            }
            else if (sub_path == "/index")
            {
                body = R"(<!DOCTYPE html>
                <html><head><meta charset="UTF-8"><title>Blue Proxy</title>
                <style>
                *{margin:0;padding:0;box-sizing:border-box}
                body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                h1{color:#1677ff;margin-bottom:20px}
                .card{background:#16213e;border-radius:8px;padding:20px;margin-bottom:20px}
                .card h2{color:#e94560;margin-bottom:10px;font-size:16px}
                .stat{display:inline-block;margin:10px 20px 10px 0}
                .stat .val{font-size:24px;color:#1677ff}
                .stat .label{font-size:12px;color:#888}
                table{width:100%;border-collapse:collapse;margin-top:10px}
                th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                td{padding:8px;border-bottom:1px solid #333;font-size:13px}
                tr:hover{background:#0f3460}
                a{color:#1677ff;text-decoration:none}
                .menu{display:flex;gap:20px;margin-bottom:20px;flex-wrap:wrap}
                .menu a{padding:8px 16px;background:#16213e;border-radius:4px}
                .menu a:hover{background:#0f3460}
                .btn-danger{background:#e94560;color:#fff}
                .btn-danger:hover{background:#c73a52}
                </style></head><body>
                <h1>🔵 Blue Proxy</h1>
                <div class="menu">
                <a href="/admin/index">Dashboard</a>
                <a href="/admin/logs">Request Logs</a>
                <a href="/admin/pools">Pool Stats</a>
                <a href="/admin/config">Config</a>
                <a href="/admin/shutdown" class="btn-danger">⚠️ Shutdown Server</a>
                </div>
                <div class="card">
                <h2>Server Info</h2>
                <div class="stat"><div class="val">)" +
                       std::to_string(Scheduler::GetThis()->GetThreadCount()) + R"(</div><div class="label">Thread Count</div></div>
                <div class="stat"><div class="val">)" +
                       std::to_string(s_pools.size()) + R"(</div><div class="label">Connection Pools</div></div>
                <div class="stat"><div class="val">)" +
                       std::to_string(TcpServer<T>::getConnection()) + R"(</div><div class="label">Active Connections</div></div>
                </div>
                <div class="card">
                <h2>📊 Requests Per Minute</h2>
                <canvas id="chart" width="800" height="300"></canvas>
                </div>
                <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
                <script>
                fetch('/admin/api/stats')
                .then(r => r.json())
                .then(data => {
                    new Chart(document.getElementById('chart'), {
                    type: 'line',
                    data: {
                        labels: data.labels,
                        datasets: [{
                        label: 'Requests/min',
                        data: data.values,
                        borderColor: '#1677ff',
                        backgroundColor: 'rgba(22,119,255,0.1)',
                        tension: 0.3
                        }]
                    },
                    options: {
                        responsive: true,
                        plugins: { legend: { labels: { color: '#e0e0e0' } } },
                        scales: {
                        x: { ticks: { color: '#888' }, grid: { color: '#333' } },
                        y: { ticks: { color: '#888' }, grid: { color: '#333' }, beginAtZero: true }
                        }
                    }
                    });
                });
                </script>
                </body></html>)";
            }
            else if (sub_path == "/logs")
            {
                std::string search = request->getParam("search", "");
                body = R"(<html><head><meta charset='UTF-8'><title>Logs</title>
                    <style>body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                    h1{color:#1677ff}.card{background:#16213e;border-radius:8px;padding:20px;margin:20px 0}
                    table{width:100%;border-collapse:collapse;margin-top:10px}
                    th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                    td{padding:8px;border-bottom:1px solid #333;font-size:13px;max-width:400px;overflow:hidden}
                    tr:hover{background:#0f3460}
                    a{color:#1677ff;text-decoration:none}
                    input{padding:8px;border:1px solid #333;background:#1a1a2e;color:#e0e0e0;border-radius:4px;width:300px}
                    button{padding:8px 16px;background:#1677ff;color:#fff;border:none;border-radius:4px;cursor:pointer}
                    .menu{display:flex;gap:20px;margin-bottom:20px}
                    .menu a{padding:8px 16px;background:#16213e;border-radius:4px}
                    </style></head><body>
                    <h1>📋 Request Logs</h1>
                    <div class="menu">
                    <a href='/admin/index'>← Back to Dashboard</a>
                    <a href='/admin/shutdown' class="btn-danger">⚠️ Shutdown</a>
                    </div>
                    <form style='margin:20px 0'>
                    <input name='search' value=')" +
                       search + R"(' placeholder='Search by URL or IP...'>
                    <button type='submit'>Search</button>
                    </form>)";

                if (s_dbmanager_ptr)
                {
                    int page = atoi(request->getParam("page", "1").c_str());
                    if (page < 1)
                        page = 1;
                    int limit = 20;
                    int offset = (page - 1) * limit;
                    std::string sql = "SELECT id,client_ip,target_url,status_code,duration_ms,created_at FROM request_logs ";
                    if (!search.empty())
                    {
                        sql += "WHERE target_url LIKE '%" + s_dbmanager_ptr->escape(search) + "%' OR client_ip LIKE '%" + s_dbmanager_ptr->escape(search) + "%' ";
                    }
                    sql += "ORDER BY id DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

                    auto res = s_dbmanager_ptr->query(sql);
                    if (res)
                    {
                        body += "<table><tr><th>ID</th><th>IP</th><th>Target</th><th>Status</th><th>Time</th><th>Date</th></tr>";
                        MYSQL_ROW row;
                        while ((row = mysql_fetch_row(res)))
                        {
                            body += "<tr><td>" + std::string(row[0] ? row[0] : "-") +
                                    "</td><td>" + std::string(row[1] ? row[1] : "-") +
                                    "</td><td style='max-width:300px;overflow:hidden'>" + std::string(row[2] ? row[2] : "-") +
                                    "</td><td>" + std::string(row[3] ? row[3] : "-") +
                                    "</td><td>" + std::string(row[4] ? row[4] : "-") + "ms" +
                                    "</td><td>" + std::string(row[5] ? row[5] : "-") + "</td></tr>";
                        }
                        body += "</table>";
                        mysql_free_result(res);
                    }
                    body += "<div style='margin-top:20px'>";
                    body += "<a href='/admin/logs?page=" + std::to_string(page > 1 ? page - 1 : 1) + "'>← Prev</a> ";
                    body += " Page " + std::to_string(page) + " ";
                    body += "<a href='/admin/logs?page=" + std::to_string(page + 1) + "'>Next →</a>";
                    body += "</div>";
                }
                body += "</body></html>";
            }
            else if (sub_path == "/pools")
            {
                body = R"(<html><head><meta charset='UTF-8'><title>Pool Stats</title>
                <style>body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                h1{color:#1677ff}.card{background:#16213e;border-radius:8px;padding:20px;margin:20px 0}
                table{width:100%;border-collapse:collapse}
                th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                td{padding:8px;border-bottom:1px solid #333}
                a{color:#1677ff;text-decoration:none}
                .menu{display:flex;gap:20px;margin-bottom:20px}
                .menu a{padding:8px 16px;background:#16213e;border-radius:4px}
                </style></head><body>
                <h1>🔗 Connection Pools</h1>
                <div class="menu">
                <a href='/admin/index'>← Back to Dashboard</a>
                <a href='/admin/shutdown' class="btn-danger">⚠️ Shutdown</a>
                </div>
                <div class='card'><table>
                <tr><th>Pool Key</th><th>Total</th><th>Idle</th></tr>)";

                for (auto &[key, pool] : s_pools)
                {
                    body += "<tr><td>" + key + "</td><td>" + std::to_string(pool->getTotalCounts()) +
                            "</td><td>" + std::to_string(pool->getIdleCounts()) + "</td></tr>";
                }
                body += R"(</table></div></body></html>)";
            }
            else if (sub_path == "/config")
            {
                body = R"(<html><head><meta charset='UTF-8'><title>Config</title>
                <style>body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                h1{color:#1677ff}.card{background:#16213e;border-radius:8px;padding:20px;margin:20px 0}
                table{width:100%;border-collapse:collapse}
                th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                td{padding:8px;border-bottom:1px solid #333}
                a{color:#1677ff;text-decoration:none}
                .menu{display:flex;gap:20px;margin-bottom:20px}
                .menu a{padding:8px 16px;background:#16213e;border-radius:4px}
                </style></head><body>
                <h1>⚙️ Configuration</h1>
                <div class="menu">
                <a href='/admin/index'>← Back to Dashboard</a>
                <a href='/admin/shutdown' class="btn-danger">⚠️ Shutdown</a>
                </div>
                <div class='card'><table>
                <tr><th>Key</th><th>Value</th><th>Description</th></tr>
                <tr><td>proxy.rate_limit</td><td>)" +
                       std::to_string(s_rate_limit) + R"(</td><td>Rate limit per minute</td></tr>
                <tr><td>proxy.cache_ttl</td><td>)" +
                       std::to_string(s_cache_expire) + R"(</td><td>Cache TTL (seconds)</td></tr>
                <tr><td>db.host</td><td>)" +
                       s_db_host + R"(</td><td>Database host</td></tr>
                <tr><td>db.database</td><td>)" +
                       s_db_database + R"(</td><td>Database name</td></tr>
                <tr><td>redis.host</td><td>)" +
                       s_redis_host + R"(</td><td>Redis host</td></tr>
                </table></div></body></html>)";
            }

            response->setBody(body);
            response->setHeader("Content-Type", "text/html; charset=utf-8");
            response->setHeader("Content-Length", std::to_string(body.size()));
            response->setStatus(HttpStatus::OK);
        }

        template <typename T>
        Task<void> HttpServer<T>::_handleConnect(MSocket::MSocketPtr sock,
                                                 HttpRequest::HttpRequestPtr request)
        {
            // CONNECT 的 path 是 host:port
            std::string host_port = request->getHeader("Host");
            if (host_port.empty())
            {
                host_port = request->getPath();
            }

            // 解析 host 和 port
            std::string host = host_port;
            uint16_t port = 443;
            size_t colon = host_port.find(':');
            if (colon != std::string::npos)
            {
                host = host_port.substr(0, colon);
                port = std::stoi(host_port.substr(colon + 1));
            }

            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel to " << host << ":" << port;

            // 连接目标站
            auto addr = blue::Address::LookupAnyIpAddress(host);
            if (!addr)
            {
                co_return;
            }
            addr->setPort(port);

            auto remote_sock = blue::MSocket::CreateTcp(addr);
            bool tem = co_await remote_sock->connect(addr);
            if (!tem)
            {
                co_return;
            }

            // 返回 200 给浏览器，表示隧道建立
            std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
            co_await Send(sock->getSocketfd(), ok.c_str(), ok.size(), 0);
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel established";

            co_await blue::proxy::Tunnel::create(sock, remote_sock);
            remote_sock->close();

            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel closed";
            co_return;
        }

        template <typename T>
        Task<void> HttpServer<T>::_handleWebSocket(MSocket::MSocketPtr sock,
                                                   HttpRequest::HttpRequestPtr request,
                                                   HttpResponse::HttpResponsePtr response,
                                                   std::string targeturl)
        {
            // 1. 连接目标站
            auto url = blue::Url::CreateUrl(targeturl);
            auto addr = url->createAddress();
            auto remote = blue::MSocket::CreateTcp(addr);
            bool conn = co_await remote->connect(addr);
            if (!conn)
            {
                co_return;
            }

            // 2. 转发升级请求到目标站
            std::string req_str = request->toString();
            co_await remote->send(req_str.c_str(), req_str.size());

            // 3. 读取目标站的 101 响应
            char buf[4096];
            ssize_t n = co_await remote->recv(buf, sizeof(buf), 0);

            // 4. 透传 101 给浏览器
            co_await Send(sock->getSocketfd(), buf, n, 0);

            co_await blue::proxy::Tunnel::create(sock, remote);
            remote->close();

            co_return;
        }

        template <typename T>
        void HttpServer<T>::PrepareHeaders(HttpRequest::HttpRequestPtr request, std::map<std::string, std::string> &headers)
        {
            for (auto &[key, val] : request->getHeaders())
            {
                if (strcasecmp(key.c_str(), "host") == 0)
                {
                    continue;
                }
                if (strcasecmp(key.c_str(), "referer") == 0)
                {
                    continue;
                }
                if (strncasecmp(key.c_str(), "sec-", 4) == 0)
                {
                    continue;
                }
                headers[key] = val;
            }
        }

        template <typename T>
        std::tuple<std::string, bool, std::string> HttpServer<T>::tryCache(const std::string &targeturl, HttpResponse::HttpResponsePtr response,
                                                                           HttpMethod method, bool isForwardProxy)
        {
            std::string cache_key;
            bool use_cache = false;
            if (isForwardProxy && method == HttpMethod::GET && s_redismanager_ptr)
            {
                use_cache = true;
                cache_key = "cache:" + targeturl;
                std::string cached = s_redismanager_ptr->get(cache_key);
                if (!cached.empty())
                {
                    response->setBody(cached);
                    response->setHeader("Content-Length", std::to_string(cached.size()));
                    response->setHeader("X-Cache", "HIT");
                    response->setStatus(blue::http::HttpStatus::OK);
                    return std::make_tuple(cache_key, use_cache, cached);
                }
            }
            if (!isForwardProxy && method == HttpMethod::GET && s_redismanager_ptr)
            {
                use_cache = true;
                cache_key = "rcache:" + targeturl;
                std::string cached = s_redismanager_ptr->get(cache_key);
                if (!cached.empty())
                {
                    response->setBody(cached);
                    response->setHeader("Content-Length", std::to_string(cached.size()));
                    response->setHeader("X-Cache", "HIT");
                    response->setStatus(blue::http::HttpStatus::OK);
                    return std::make_tuple(cache_key, use_cache, cached);
                }
            }
            return std::make_tuple(cache_key, use_cache, "");
        }

        template <typename T>
        void HttpServer<T>::setXForwardedFor(const std::string &targeturl, HttpRequest::HttpRequestPtr request, HttpResponse::HttpResponsePtr response)
        {
            std::string xxf = request->getHeader("X-Forwarded-For");
            if (!xxf.empty())
            {
                xxf += ", ";
            }
            xxf += m_remoteIP;
            request->setHeader("X-Forwarded-For", xxf);
            return;
        }

        template <typename T>
        HttpConnectionPool::HttpConnectionPoolPtr HttpServer<T>::getConnectionPool(blue::Url::UrlPtr UrlPtr)
        {
            std::string poolKey = UrlPtr->getScheme() + "://" + UrlPtr->getHost() + ":" + std::to_string(UrlPtr->getPort());
            blue::http::HttpConnectionPool::MmutexType::lockSco lock(s_poolMutex);
            auto it = s_pools.find(poolKey);
            if (it != s_pools.end())
            {
                return it->second;
            }
            auto pool = std::make_shared<HttpConnectionPool>(
                UrlPtr->getHost(), "", UrlPtr->getPort(), 60000, 100, UrlPtr->getScheme(), 10);
            s_pools[poolKey] = pool;
            return pool;
        }

        template <typename T>
        void HttpServer<T>::logRequest(HttpRequest::HttpRequestPtr request, const std::string &targeturl,
                                       blue::Url::UrlPtr UrlPtr, std::shared_ptr<HttpResult> result, int64_t duration, bool isForwardProxy)
        {
            if (s_dbmanager_ptr)
            {
                std::string method_str = http::HttpMethodToChars(request->getMethod());
                int status_code = 0;
                int body_size = 0;
                std::string error_msg;

                if (result->response)
                {
                    status_code = (int)result->response->getStatus();
                    body_size = result->response->getBody().size();
                }
                if (!result->error.empty())
                {
                    error_msg = result->error;
                }

                s_dbmanager_ptr->logRequest(m_remoteIP,
                                            method_str,
                                            targeturl, UrlPtr->getHost(),
                                            status_code, body_size,
                                            request->getHeader("User-Agent"),
                                            duration, isForwardProxy, false, error_msg);
            }
        }

        template <typename T>
        void HttpServer<T>::processResponseContent(HttpResponse::HttpResponsePtr response,
                                                   std::shared_ptr<HttpResult> result, const std::string &targeturl,
                                                   int status)
        {
            std::string set_cookie = result->response->getHeader("Set-Cookie");
            if (!set_cookie.empty())
            {
                std::string fixed_cookie = fix_cookie_domain(set_cookie);
                response->setHeader("Set-Cookie", fixed_cookie);
            }

            std::string ContentType = result->response->getHeader("Content-Type");
            blue::proxy::UrlRewriter rewriter(targeturl, "/blue");
            if (ContentType.find("text/html") != std::string::npos)
            {
                auto original_html = result->response->getBody();
                auto modified_body = rewriter.process_html(original_html);
                response->setBody(modified_body);
                response->setStatus((blue::http::HttpStatus)status);
                // 设置的修改后未压缩大小,稍后在sendresponse时会设置压缩后的length
                response->setHeader("Content-Length", std::to_string(modified_body.size()));
            }
            else if (ContentType.find("text/css") != std::string::npos)
            {
                auto original_css = result->response->getBody();
                auto modified_css = rewriter.process_css(original_css);
                response->setBody(modified_css);
                response->setStatus((blue::http::HttpStatus)status);
                // 设置的修改后未压缩大小,稍后在sendresponse时会设置压缩后的length
                response->setHeader("Content-Length", std::to_string(modified_css.size()));
            }
            else
            {
                response->setBody(result->response->getBody());
                response->setHeader("Content-Length", std::to_string(result->response->getBody().size()));
                response->setStatus((blue::http::HttpStatus)status);
            }
            if (!ContentType.empty())
            {
                response->setHeader("Content-Type", ContentType);
            }
        }

        template <typename T>
        bool HttpServer<T>::isRedirect(int status)
        {
            return status == 301 || status == 302 || status == 307 || status == 308;
        }

        template <typename T>
        bool HttpServer<T>::handleRedirect(HttpResponse::HttpResponsePtr response,
                                           const std::string &location, const std::string &targeturl,
                                           const std::string &cache_key, bool use_cache,
                                           int status)
        {
            if (location == targeturl || location == targeturl + "/")
            {
                BLUE_LOG_WARN(g_logger) << "Redirect loop detected: " << location;
                std::string err = "Redirect loop detected";
                response->setStatus(blue::http::HttpStatus::BAD_GATEWAY);
                response->setBody(err);
                response->setHeader("Content-Length", std::to_string(err.size()));
                if (use_cache)
                {
                    s_redismanager_ptr->set(cache_key, err, s_cache_expire);
                }
                return true;
            }

            // 把重定向 URL 也改成代理模式
            std::string new_location = "/blue/" + location;
            response->setHeader("Location", new_location);
            response->delHeader("Content-Length"); // 重定向没有 body
            response->setBody("");                 // 清空 body
            response->setStatus((blue::http::HttpStatus)status);
            if (use_cache)
            {
                s_redismanager_ptr->set(cache_key, "", s_cache_expire);
            }
            return true;
        }
        template class blue::http::HttpServer<int>;
        template class blue::http::HttpServer<timeval>;
    }
}