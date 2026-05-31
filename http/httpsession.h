#ifndef BLUE_HTTP_HTTPSESSION_H
#define BLUE_HTTP_HTTPSESSION_H
#include <string>
#include <vector>
#include "blue/msocketstream.h"
#include "blue/sslsocket.h"
#include "http.h"
#include "httpParser.h"

// session
namespace blue
{
    namespace http
    {
        class HttpSession : public SocketStream
        {
        public:
            enum class RecvStatus
            {
                OK,    // 解析完成
                ERROR, // 真正的错误
                CLOSE  // 用户主动关闭
            };

        public:
            using HttpSessionPtr = std::shared_ptr<HttpSession>;
            using ReturnType = std::pair<RecvStatus, HttpRequest::HttpRequestPtr>;

        public:
            /**
             * @brief 服务器端与客户端一次连接的抽象
             * @param sock 用客户端sockfd封装的对象
             * @param owner 是否完全交给会话管理客户端sockfd对象,默认 true
             */
            HttpSession(SocketStream::SocketStreamPtr stream, bool owner = true);

            /**
             * @brief 接收客户端请求并解析
             * @return 返回一对值(recvStatus,httpRequestPtr)
             * @note recvStatus返回有 ok,error,close(客户端主动关闭连接)
             */
            ReturnType recvRequest();

            /**
             * @brief 对客户端发送的请求予以响应
             * @param response 响应对象智能指针
             * @param request 解析后的request.拿到发送发有没有Accept-Encoding
             */
            int sendResponse(HttpResponse::HttpResponsePtr response, HttpRequest::HttpRequestPtr request);

            /**
             * @brief 检查encoding是否含有tem
             * @param encoding 一系列解压和压缩的方式字符串
             * @param tem 检查是否包含tem方式
             */
            bool checkEncoding(const std::string &encoding, const char *tem);

        private:
            bool m_isFinish = false;
            SocketStream::SocketStreamPtr m_stream;
        };
    }
}

#endif