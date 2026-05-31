#include "blue/log.h"
#include "httpconnection.h"
#include <string>
#include <fcntl.h>

// http connnection
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        std::string HttpResult::toString() const
        {
            std::stringstream ss;
            ss << "[HttpResult result : " << result
               << " error : " << error
            //    << " response : " << (response ? response->toString() : "nullptr")
               << "]";
            return ss.str();
        }

        HttpConnection::HttpConnection(SocketStream::SocketStreamPtr stream, uint64_t time, bool owner)
            : SocketStream(stream->getSock(), owner),
              m_stream(stream),
              m_createTime(time)
        {
        }

        HttpConnection::~HttpConnection()
        {
            BLUE_LOG_INFO(g_logger) << "HttpConnection::~HttpConnection";
        }

        HttpConnection::ReturnType HttpConnection::recvResponse()
        {
            m_isFinish = false;
            auto parser = Parser::CreateHttpResponseParser();
            parser->on_MessageComplate([this](blue::http::HttpResponse::HttpResponsePtr ptr) -> int
                                       {
                m_isFinish = true;
                return 0; });
            // parser->on_Body([](const std::string &data) -> int
            //                 {
            //     BLUE_LOG_INFO(g_logger) << data << std::flush;
            //     return 0; });
            parser->Init();
            uint64_t responsebuffersize = HttpResponseParser::GetHttpResponseBufferSize();
            // 无脑vector 管理data
            std::vector<char> vec_data(responsebuffersize);
            auto data = vec_data.data();
            size_t offset = 0;
            do
            {
                ssize_t n = m_stream->read(data + offset, responsebuffersize - offset);
                if (n == 0) // 对方主动关闭连接
                {
                    // 连接关闭，尝试finalize
                    parser->Finalize(); // 告诉已经发送完数据了
                    if (m_isFinish)     // 监测一个完整的http response解析完毕
                    {
                        return {HttpConnection::RecvStatus::OK, parser->getData()};
                    }
                    return {HttpConnection::RecvStatus::CLOSE, nullptr};
                }
                if (n < 0)
                {
                    if (errno == EINTR)
                        continue;
                    // hook帮我们hook住了，如果没有数据了最后回来了回到hook重新去读了
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;
                    return {HttpConnection::RecvStatus::ERROR, nullptr};
                }
                parser->Execute(data + offset, n);
                offset += n;
                if (m_isFinish)
                {
                    return {HttpConnection::RecvStatus::OK, parser->getData()};
                }
                if (parser->getError() != HPE_OK)
                {
                    BLUE_LOG_ERROR(g_logger) << "http response 格式错误";
                    return {HttpConnection::RecvStatus::ERROR, nullptr};
                }

                // 数据不够放了
                if (offset >= responsebuffersize)
                {
                    HttpResponseParser::SetResponseBufferSize(offset * 2); // 热更新
                    size_t newsize = HttpResponseParser::GetHttpResponseBufferSize();

                    // 无脑vector 管理data
                    vec_data.resize(newsize);
                    data = vec_data.data();
                    responsebuffersize = newsize;
                }
            } while (true);

            return {HttpConnection::RecvStatus::ERROR, nullptr};
        }

        int HttpConnection::sendRequest(HttpRequest::HttpRequestPtr request)
        {
            std::string body = request->getBody();
            if (!body.empty() && body.size() > 20)
            {
                body = request->compress(body);
                request->setBody(body);
                request->setHeader("Content-Encoding", "gzip");
                request->setHeader("Content-Length", std::to_string(body.size()));
            }
            // 希望服务器返回压缩响应
            request->setHeader("Accept-Encoding", "gzip");
            std::string str = request->toString();
            return m_stream->writeFixSize(str.c_str(), str.size());
        }

        std::shared_ptr<HttpResult> HttpConnection::DoGet(const std::string &url,
                                                          uint64_t timeout_ms,
                                                          const std::map<std::string, std::string> &header,
                                                          const std::string &body)
        {
            auto urlptr = blue::Url::CreateUrl(url);
            if (!urlptr)
            {
                // BLUE_LOG_INFO(g_logger) << "url failed";
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::INVALID_URL),
                                                    nullptr,
                                                    "invailed get url " + url);
            }
            return DoGet(urlptr, timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnection::DoPost(const std::string &url,
                                                           uint64_t timeout_ms,
                                                           const std::map<std::string, std::string> &header,
                                                           const std::string &body)
        {
            auto urlptr = blue::Url::CreateUrl(url);
            if (!urlptr)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::INVALID_URL),
                                                    nullptr,
                                                    "invailed post url " + url);
            }
            return DoPost(urlptr, timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnection::DoGet(blue::Url::UrlPtr url,
                                                          uint64_t timeout_ms,
                                                          const std::map<std::string, std::string> &header,
                                                          const std::string &body)
        {
            return DoRequest(http::HttpMethod::GET, url, timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnection::DoPost(blue::Url::UrlPtr url,
                                                           uint64_t timeout_ms,
                                                           const std::map<std::string, std::string> &header,
                                                           const std::string &body)
        {
            return DoRequest(http::HttpMethod::POST, url, timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnection::DoRequest(http::HttpMethod method,
                                                              const std::string &url,
                                                              uint64_t timeout_ms,
                                                              const std::map<std::string, std::string> &header,
                                                              const std::string &body)
        {
            auto urlptr = blue::Url::CreateUrl(url);
            if (!urlptr)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::INVALID_URL),
                                                    nullptr,
                                                    "invailed request url " + url);
            }

            return DoRequest(method, urlptr, timeout_ms, header, body);
        }
        std::shared_ptr<HttpResult> HttpConnection::DoRequest(http::HttpMethod method,
                                                              blue::Url::UrlPtr url,
                                                              uint64_t timeout_ms,
                                                              const std::map<std::string, std::string> &header,
                                                              const std::string &body)
        {
            auto request = std::make_shared<HttpRequest>();
            request->setMethod(method);
            request->setPath(url->getPath());
            request->setQuery(url->getQuery());
            request->setFragment(url->getFragment());
            bool hashost = false;
            for (auto &[key, val] : header)
            {
                if (strcasecmp(key.c_str(), "connection") == 0)
                {
                    request->setKeepAlive((strcasecmp(val.c_str(), "keep-alive") == 0));
                    continue;
                }
                if (!hashost && strcasecmp(key.c_str(), "host") == 0)
                {
                    hashost = !val.empty();
                }
                request->setHeader(key, val);
            }
            if (!hashost)
            {
                request->setHeader("Host", url->getHost());
            }
            request->setBody(body);
            return DoRequest(request, url, timeout_ms);
        }

        std::shared_ptr<HttpResult> HttpConnection::DoRequest(http::HttpRequest::HttpRequestPtr req,
                                                              blue::Url::UrlPtr url,
                                                              uint64_t timeout_ms)
        {
            auto addr = url->createAddress();
            if (!addr)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::INVALID_HOST),
                                                    nullptr,
                                                    "invailed requst host, unicodehost : " + url->getUnicodeHost() + "asciihost : " + url->getHost());
            }
            auto sock = blue::MSocket::CreateTcp(addr);
            if (!sock)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::CREATE_SOCKET_ERROR),
                                                    nullptr,
                                                    "create socket failed, addr : " + addr->toString() + " error " + std::to_string(errno) + " strerror : " + std::string(strerror(errno)));
            }
            bool ret = sock->connect(addr);
            if (!ret)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::CONNECT_FAILED),
                                                    nullptr,
                                                    "connected failed, addr : " + addr->toString() + " error " + std::to_string(errno) + " strerror : " + std::string(strerror(errno)));
            }


            // 新增https
            bool isSSl = (url->getScheme() == "https");
            BLUE_LOG_WARN(g_logger) << "Connecting to " << addr->toString() 
                        << ":" << url->getPort() 
                        << " isSSL=" << isSSl;
            std::shared_ptr<SSLSocket> ssl_sock;
            HttpConnection::HttpConnectionPtr connect;
            if (isSSl)
            {
                ssl_sock = std::make_shared<SSLSocket>(sock,true,false);
                SSL_set_tlsext_host_name(ssl_sock->getSSL(),url->getHost().c_str());
                if (!ssl_sock->isValid())
                {
                    return std::make_shared<HttpResult>((int)HttpResult::ResultStatus::SSL_INVALID_SSL,
                                    nullptr,
                                    "SSL invalied");
                }
                if (!ssl_sock->handshake()) 
                {
                    BLUE_LOG_ERROR(g_logger) << "SSL handshake failed for: " << url->getHost();
                    unsigned long e = ERR_get_error();
                    char buf[256];
                    ERR_error_string_n(e, buf, sizeof(buf));
                    BLUE_LOG_ERROR(g_logger) << "SSL error: " << buf;
                    return std::make_shared<HttpResult>((int)HttpResult::ResultStatus::SSL_HANDSHAKE_FAILED,
                        nullptr,
                        "SSL handshake failed");
                }
                connect = std::make_shared<HttpConnection>(ssl_sock);
            }
            else
            {
                auto stream = std::make_shared<SocketStream>(sock);
                connect = std::make_shared<HttpConnection>(stream);
            }

            sock->setRecvTimeout(timeout_ms);
            int rt = connect->sendRequest(req);
            if (rt == 0)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::SEND_CLOSE_BY_PEER),
                                                    nullptr,
                                                    "send request closed by peer : " + addr->toString());
            }
            if (rt < 0)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::SEND_SOCKET_ERROR),
                                                    nullptr,
                                                    "send socket, errno : " + std::to_string(errno) + " strerror : " + std::string(strerror(errno)));
            }
            uint64_t nowtimems = blue::GetCurrentMs();
            auto [status, response] = connect->recvResponse();
            uint64_t backtimes = blue::GetCurrentMs();
            if (!response)
            {
                if (nowtimems + timeout_ms < backtimes)
                {
                    return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::TIMEOUT),
                                                        nullptr,
                                                        "recvresponse timeout, addr : " + addr->toString() + " timeout_ms : " + std::to_string(timeout_ms));
                }
                return std::make_shared<HttpResult>((int)(status),
                                                    nullptr,
                                                    "recvresponse error, errno : " + std::to_string(errno) + " strerror : " + std::string(strerror(errno)));
            }
            return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::OK), response, "ok");
        }

        HttpConnectionPool::HttpConnectionPool(const std::string &host, const std::string &vhost,
                                               uint16_t port, uint64_t aliveTime, 
                                               uint32_t maxRequest, uint32_t maxSize)
            : m_host(host), m_vhost(vhost),
              m_maxAliveTime(aliveTime), m_maxSize(maxSize),
              m_maxRequest(maxRequest), m_port(port)
        {
        }

        uint32_t HttpConnectionPool::getIdleCounts() const
        {
            MmutexType::lockSco lock(m_mutex);
            return m_pool.size();
        }

        HttpConnection::HttpConnectionPtr HttpConnectionPool::getConnnection()
        {
            uint64_t nowms = blue::GetCurrentMs();
            std::vector<HttpConnection *> invalid_conn;
            HttpConnection *ptr = nullptr;
            MmutexType::lockSco lock(m_mutex);
            while (!m_pool.empty())
            {
                auto conn = *m_pool.begin();
                m_pool.pop_front();
                if (!conn->isConnected())
                {
                    invalid_conn.push_back(conn);
                    continue;
                }
                if (conn->m_createTime + m_maxAliveTime <= nowms)
                {
                    invalid_conn.push_back(conn);
                    continue;
                }
                bool excepted = false;
                if (!conn->m_isBusy.compare_exchange_strong(excepted, true))
                {
                    invalid_conn.push_back(conn);
                    continue;
                }
                ptr = conn;
                break;
            }
            lock.unlock();
            m_total -= invalid_conn.size();
            for (auto i : invalid_conn)
            {
                delete i;
            }
            if (!ptr)
            {
                auto addr = blue::Address::LookupAnyIpAddress(m_host);
                if (!addr)
                {
                    BLUE_LOG_ERROR(g_logger) << "get addr failed, host : " << m_host;
                    return nullptr;
                }
                addr->setPort(m_port);
                auto sock = blue::MSocket::CreateTcp(addr);
                if (!sock)
                {
                    BLUE_LOG_ERROR(g_logger) << "create socket failed, addr : " << addr->toString();
                    return nullptr;
                }
                if (!sock->connect(addr))
                {
                    BLUE_LOG_ERROR(g_logger) << "sock connect failed, addr : " << addr->toString();
                    return nullptr;
                }
                bool isSSl = (m_scheme == "https");
                std::shared_ptr<SSLSocket> ssl_sock;
                if (isSSl)
                {
                    ssl_sock = std::make_shared<SSLSocket>(sock,true,false);
                    SSL_set_tlsext_host_name(ssl_sock->getSSL(),m_host.c_str());
                    if (!ssl_sock->isValid())
                    {
                        BLUE_LOG_ERROR(g_logger) << "ssl_sock is not valid";
                        return nullptr;
                    }
                    if (!ssl_sock->handshake())
                    {
                        BLUE_LOG_ERROR(g_logger) << "SSl handshake failed, host : " << m_host;
                        unsigned long e = ERR_get_error();
                        char buf[256];
                        ERR_error_string_n(e, buf, sizeof(buf));
                        BLUE_LOG_ERROR(g_logger) << "SSL error: " << buf;
                        return nullptr;
                    }
                    ptr = new HttpConnection(ssl_sock);
                }
                else
                {
                    auto stream = std::make_shared<SocketStream>(sock);
                    ptr = new HttpConnection(stream,blue::GetCurrentMs());
                }
                m_total.fetch_add(1, std::memory_order_acq_rel);
            }
            return HttpConnection::HttpConnectionPtr(ptr, [self = this](HttpConnection *p)
                                                     { ReleasePtr(p, self); });
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doGet(const std::string &url,
                                                              uint64_t timeout_ms,
                                                              const std::map<std::string, std::string> &header,
                                                              const std::string &body)
        {
            return doRequest(blue::http::HttpMethod::GET, url, timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doPost(const std::string &url,
                                                               uint64_t timeout_ms,
                                                               const std::map<std::string, std::string> &header,
                                                               const std::string &body)
        {
            return doRequest(blue::http::HttpMethod::POST, url, timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doGet(blue::Url::UrlPtr url,
                                                              uint64_t timeout_ms,
                                                              const std::map<std::string, std::string> &header,
                                                              const std::string &body)
        {
            std::stringstream ss;
            ss << url->getPath()
               << (url->getQuery().empty() ? "" : "?")
               << url->getQuery()
               << (url->getFragment().empty() ? "" : "#")
               << url->getFragment();
            m_scheme = url->getScheme();
            return doGet(ss.str(), timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doPost(blue::Url::UrlPtr url,
                                                               uint64_t timeout_ms,
                                                               const std::map<std::string, std::string> &header,
                                                               const std::string &body)
        {
            std::stringstream ss;
            ss << url->getPath()
               << (url->getQuery().empty() ? "" : "?")
               << url->getQuery()
               << (url->getFragment().empty() ? "" : "#")
               << url->getFragment();
            m_scheme = url->getScheme();
            return doPost(ss.str(), timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doRequest(http::HttpMethod method,
                                                                  const std::string &url,
                                                                  uint64_t timeout_ms,
                                                                  const std::map<std::string, std::string> &header,
                                                                  const std::string &body)
        {
            auto request = std::make_shared<HttpRequest>();
            request->setMethod(method);
            request->setPath(url);
            request->setKeepAlive(true);
            bool hashost = false;
            for (auto &[key, val] : header)
            {
                if (strcasecmp(key.c_str(), "connection") == 0)
                {
                    request->setKeepAlive((strcasecmp(val.c_str(), "keep-alive") == 0));
                    continue;
                }
                if (!hashost && strcasecmp(key.c_str(), "host") == 0)
                {
                    hashost = !val.empty();
                }
                request->setHeader(key, val);
            }
            if (!hashost)
            {
                if (m_vhost.empty())
                {
                    request->setHeader("Host", m_host);
                }
                else
                {
                    request->setHeader("Host", m_vhost);
                }
            }
            request->setBody(body);
            return doRequest(request, timeout_ms);
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doRequest(http::HttpMethod method,
                                                                  blue::Url::UrlPtr url,
                                                                  uint64_t timeout_ms,
                                                                  const std::map<std::string, std::string> &header,
                                                                  const std::string &body)
        {
            std::stringstream ss;
            ss << url->getPath()
               << (url->getQuery().empty() ? "" : "?")
               << url->getQuery()
               << (url->getFragment().empty() ? "" : "#")
               << url->getFragment();
            m_scheme = url->getScheme();
            return doRequest(method, ss.str(), timeout_ms, header, body);
        }

        std::shared_ptr<HttpResult> HttpConnectionPool::doRequest(http::HttpRequest::HttpRequestPtr req,
                                                                  uint64_t timeout_ms)
        {
            auto conn = getConnnection();
            if (!conn)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::POOL_GET_CONNECTION_FAILED),
                                                    nullptr,
                                                    "pool host : " + m_host + ":" + std::to_string(m_port));
            }
            auto sock = conn->getSock();
            if (!sock)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::POOL_INVALID_CONNECTION),
                                                    nullptr,
                                                    "invalid connection, host : " + m_host + ":" + std::to_string(m_port));
            }
            sock->setRecvTimeout(timeout_ms);
            int rt = conn->sendRequest(req);
            if (rt == 0)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::SEND_CLOSE_BY_PEER),
                                                    nullptr,
                                                    "send request closed by peer : " + sock->getRemoteAddress()->toString());
            }
            if (rt < 0)
            {
                return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::SEND_SOCKET_ERROR),
                                                    nullptr,
                                                    "send socket, errno : " + std::to_string(errno) + " strerror : " + std::string(strerror(errno)));
            }
            uint64_t nowtimems = blue::GetCurrentMs();
            auto [status, response] = conn->recvResponse();
            uint64_t backtimes = blue::GetCurrentMs();
            if (!response)
            {
                if (nowtimems + timeout_ms < backtimes)
                {
                    return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::TIMEOUT),
                                                        nullptr,
                                                        "recvresponse timeout, addr : " + sock->getRemoteAddress()->toString() + " timeout_ms : " + std::to_string(timeout_ms));
                }
                return std::make_shared<HttpResult>((int)(status),
                                                    nullptr,
                                                    "recvresponse error, errno : " + std::to_string(errno) + " strerror : " + std::string(strerror(errno)));
            }
            return std::make_shared<HttpResult>((int)(HttpResult::ResultStatus::OK), response, "ok");
        }

        void HttpConnectionPool::ReleasePtr(HttpConnection *conn, HttpConnectionPool *pool)
        {
            // BLUE_LOG_INFO(g_logger) << "ReleasePtr: isConnected=" << conn->isConnected()
            //                         << " createTime+alive=" << (conn->m_createTime + pool->m_maxAliveTime <= blue::GetCurrentMs())
            //                         << " requestSize=" << conn->m_requestSize << " maxRequest=" << pool->m_maxRequest;
            conn->m_requestSize++;
            conn->m_isBusy.store(false, std::memory_order_release);
            if (!conn->isConnected() || conn->m_createTime + pool->m_maxAliveTime <= blue::GetCurrentMs() || conn->m_requestSize >= pool->m_maxRequest)
            {
                delete conn;
                pool->m_total.fetch_sub(1, std::memory_order_acq_rel);
                return;
            }
            auto sock = conn->getSock();
            if (sock && sock->isConnected())
            {
                int fd = sock->getSocketfd();

                // 循环读取并丢弃所有残留数据
                constexpr int kBUfferSize = 4096 * 2;
                auto buf = std::make_unique<char[]>(kBUfferSize);
                while (recv(fd, buf.get(), kBUfferSize, 0) > 0)
                {
                }
            }
            MmutexType::lockSco lock(pool->m_mutex);
            if (pool->m_pool.size() < pool->m_maxSize)
            {
                pool->m_pool.push_back(conn);
            }
            else
            {
                delete conn;
                pool->m_total.fetch_sub(1, std::memory_order_acq_rel);
            }
        }

    }
}
