#ifndef BLUE_HTTP_HTTPCONNECTION_H
#define BLUE_HTTP_HTTPCONNECTION_H
#include <list>
#include "blue/msocketstream.h"
#include "blue/sslsocket.h"
#include "blue/mthread.h"
#include "blue/url.h"
#include "http.h"
#include "httpParser.h"

// http connnection
namespace blue
{
    namespace http
    {
        struct HttpResult
        {
            using HttpResultPtr = std::shared_ptr<HttpResult>;
            enum class ResultStatus
            {
                OK = 0,                         // ok
                INVALID_URL = 1,                // url无效
                INVALID_HOST = 2,               // host无效
                CREATE_SOCKET_ERROR = 3,        // 创建socket失败
                CONNECT_FAILED = 4,             // 连接失败
                SEND_CLOSE_BY_PEER = 5,         // 连接被对方关闭
                SEND_SOCKET_ERROR = 6,          // send出现错误
                TIMEOUT = 7,                    // 超时
                POOL_GET_CONNECTION_FAILED = 8, // 连接池获取connection失败
                POOL_INVALID_CONNECTION = 9,    // 无效connection
                SSL_INVALID_SSL = 10,           // 无效SSL
                SSL_HANDSHAKE_FAILED = 11       // SSL handshake failed
            };
            HttpResult(int res, HttpResponse::HttpResponsePtr response, const std::string &error)
                : result(res),
                  response(response),
                  error(error) {}
            std::string toString() const;
            int result;
            HttpResponse::HttpResponsePtr response;
            std::string error;
        };

        class HttpConnectionPool;

        class HttpConnection : public SocketStream
        {
            friend class HttpConnectionPool;

        public:
            enum class RecvStatus
            {
                OK = 0,    // 解析完成
                ERROR = 1, // 真正的错误
                CLOSE = 2  // 对方主动关闭
            };

        public:
            using HttpConnectionPtr = std::shared_ptr<HttpConnection>;
            using ReturnType = std::pair<RecvStatus, HttpResponse::HttpResponsePtr>;

        public:
            /**
             * @brief get 请求
             * @param url 请求目标url
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            static std::shared_ptr<HttpResult> DoGet(const std::string &url,
                                                     uint64_t timeout,
                                                     const std::map<std::string, std::string> &header = {},
                                                     const std::string &body = "");

            /**
             * @brief post 请求
             * @param url 请求目标url
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            static std::shared_ptr<HttpResult> DoPost(const std::string &url,
                                                      uint64_t timeout,
                                                      const std::map<std::string, std::string> &header = {},
                                                      const std::string &body = "");

            /**
             * @brief get 请求
             * @param url 请求目标url智能指针
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            static std::shared_ptr<HttpResult> DoGet(blue::Url::UrlPtr url,
                                                     uint64_t timeout,
                                                     const std::map<std::string, std::string> &header = {},
                                                     const std::string &body = "");

            /**
             * @brief post 请求
             * @param url 请求目标url智能指针
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            static std::shared_ptr<HttpResult> DoPost(blue::Url::UrlPtr url,
                                                      uint64_t timeout,
                                                      const std::map<std::string, std::string> &header = {},
                                                      const std::string &body = "");
            /**
             * @brief request请求
             * @param mothod 方法
             * @param url 请求目标url
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            static std::shared_ptr<HttpResult> DoRequest(http::HttpMethod method,
                                                         const std::string &url,
                                                         uint64_t timeout,
                                                         const std::map<std::string, std::string> &header = {},
                                                         const std::string &body = "");

            /**
             * @brief request请求
             * @param mothod 方法
             * @param url 请求目标url智能指针
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            static std::shared_ptr<HttpResult> DoRequest(http::HttpMethod method,
                                                         blue::Url::UrlPtr url,
                                                         uint64_t timeout,
                                                         const std::map<std::string, std::string> &header = {},
                                                         const std::string &body = "");

            /**
             * @brief 将request请求发到指定的url并带有超时时间
             * @param req 请求智能指针
             * @param url 指定url智能指针
             * @param timeout 超时时长(ms)
             */
            static std::shared_ptr<HttpResult> DoRequest(http::HttpRequest::HttpRequestPtr req,
                                                         blue::Url::UrlPtr url,
                                                         uint64_t timeout);
            /**
             * @brief 服务器端与客户端一次连接
             * @param stream 为支持ssl改用msockstream的父类stream
             * @param time 连接创建的时间ms
             * @param owner 是否完全交给连接管理服务器端sockfd对象,默认 true
             */
            HttpConnection(SocketStream::SocketStreamPtr stream, uint64_t time = 0, bool owner = true);

            /**
             * @brief 析构
             */
            ~HttpConnection();
            /**
             * @brief 接收服务器端响应并解析
             * @return 返回一对值(recvStatus,httpResponsePtr)
             * @note recvStatus返回有 ok,error,close(客户端主动关闭连接)
             */
            ReturnType recvResponse();

            /**
             * @brief 对服务器端发送请求
             * @param request 要发送的请求对象智能指针
             */
            int sendRequest(HttpRequest::HttpRequestPtr request);

        private:
            bool m_isFinish = false;
            SocketStream::SocketStreamPtr m_stream;
            std::atomic<bool> m_isBusy = {false};
            uint64_t m_requestSize = 0;
            uint64_t m_createTime;
        };

        // 连接池最大大小
        extern uint32_t s_httpconnpool_mxsize;

        class HttpConnectionPool
        {
        public:
            using HttpConnectionPoolPtr = std::shared_ptr<HttpConnectionPool>;
            using MmutexType = blue::Mmutex;

        public:
            /**
             * @brief httpconnection pool 构造函数
             * @param host 目标真实目标主机(实际连接的 IP)
             * @param vhost 虚拟主机
             * @param port 目标端口
             * @param aliveTime 连接最大存活时间
             * @param maxRequest 最大请求数量
             * @param maxSize 连接池最大大小 具有默认大小
             */
            HttpConnectionPool(const std::string &host,
                               const std::string &vhost,
                               uint16_t port,
                               uint64_t aliveTime,
                               uint32_t maxRequest,
                               uint32_t maxSize = s_httpconnpool_mxsize);

            /**
             * @brief 获取连接实例指针
             * @return httpconnectionPtr
             */
            HttpConnection::HttpConnectionPtr getConnnection();

            /**
             * @brief get 请求
             * @param url 请求目标url
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            std::shared_ptr<HttpResult> doGet(const std::string &url,
                                              uint64_t timeout,
                                              const std::map<std::string, std::string> &header = {},
                                              const std::string &body = "");

            /**
             * @brief post 请求
             * @param url 请求目标url
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            std::shared_ptr<HttpResult> doPost(const std::string &url,
                                               uint64_t timeout,
                                               const std::map<std::string, std::string> &header = {},
                                               const std::string &body = "");

            /**
             * @brief get 请求
             * @param url 请求目标url智能指针
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            std::shared_ptr<HttpResult> doGet(blue::Url::UrlPtr url,
                                              uint64_t timeout,
                                              const std::map<std::string, std::string> &header = {},
                                              const std::string &body = "");

            /**
             * @brief post 请求
             * @param url 请求目标url智能指针
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            std::shared_ptr<HttpResult> doPost(blue::Url::UrlPtr url,
                                               uint64_t timeout,
                                               const std::map<std::string, std::string> &header = {},
                                               const std::string &body = "");
            /**
             * @brief request请求
             * @param mothod 方法
             * @param url 请求目标url
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            std::shared_ptr<HttpResult> doRequest(http::HttpMethod method,
                                                  const std::string &url,
                                                  uint64_t timeout,
                                                  const std::map<std::string, std::string> &header = {},
                                                  const std::string &body = "");

            /**
             * @brief request请求
             * @param mothod 方法
             * @param url 请求目标url智能指针
             * @param timeout 超时时长(ms)
             * @param header 头部key-val
             * @param body body
             */
            std::shared_ptr<HttpResult> doRequest(http::HttpMethod method,
                                                  blue::Url::UrlPtr url,
                                                  uint64_t timeout,
                                                  const std::map<std::string, std::string> &header = {},
                                                  const std::string &body = "");

            /**
             * @brief 发送request请求
             * @param req 请求智能指针
             * @param timeout 超时时长(ms)
             */
            std::shared_ptr<HttpResult> doRequest(http::HttpRequest::HttpRequestPtr req,
                                                  uint64_t timeout);

            /**
             * @brief 获取idle connection 数量(pool大小)
             */
            uint32_t getIdleCounts() const;

            /**
             * @brief 获取total connection 数量
             */
            uint32_t getTotalCounts() const { return m_total.load(std::memory_order_acquire); }

        private:
            static void ReleasePtr(HttpConnection *conn, HttpConnectionPool *pool);

        private:
            mutable MmutexType m_mutex;          // 互斥变量
            std::string m_scheme;                // scheme
            std::string m_host;                  // 真实目标主机(实际连接的 IP)
            std::string m_vhost;                 // 虚拟主机
            uint64_t m_maxAliveTime;             // 每个连接的最大存活时长
            uint32_t m_maxSize;                  // 连接池的最大大小
            uint32_t m_maxRequest;               // 最大支持的连接数
            uint16_t m_port;                     // 端口
            std::list<HttpConnection *> m_pool;  // 连接池
            std::atomic<uint32_t> m_total = {0}; // 共有多少连接
        };
    }
}

#endif