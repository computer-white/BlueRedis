#include "blue/log.h"
#include "httpsession.h"
#include <string>

// session
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
        HttpSession::HttpSession(SocketStream::SocketStreamPtr stream, bool owner)
            : SocketStream(stream->getSock(), owner),
            m_stream(stream)
        {
        }

        HttpSession::ReturnType HttpSession::recvRequest()
        {
            m_isFinish = false;
            auto parser = Parser::CreateHttpRequestParser();
            parser->on_MessageComplate([this](blue::http::HttpRequest::HttpRequestPtr ptr) -> int
                                       {
                m_isFinish = true;
                return 0; });
            parser->Init();
            uint64_t requestbuffersize = HttpRequestParser::GetHttpRequestBufferSize();
            // 无脑vector 管理data
            std::vector<char> vec_data(requestbuffersize);
            auto data = vec_data.data();
            size_t offset = 0;
            do
            {
                ssize_t n = m_stream->read(data + offset, requestbuffersize - offset);
                if (n == 0) // 客户端主动关闭连接
                {
                    // 连接关闭，尝试finalize
                    parser->Finalize(); // 告诉已经发送完数据了
                    if (m_isFinish)     // 监测一个完整的http request解析完毕
                    {
                        return {HttpSession::RecvStatus::OK, parser->getData()};
                    }
                    return {HttpSession::RecvStatus::CLOSE, nullptr};
                }
                if (n < 0)
                {
                    if (errno == EINTR)
                        continue;
                    // hook帮我们hook住了，如果没有数据了最后回来了回到hook重新去读了
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;
                    return {HttpSession::RecvStatus::ERROR, nullptr};
                }
                parser->Execute(data + offset, n);
                offset += n;
                if (parser->getError() != HPE_OK)
                {
                    // CONNECT 请求的特殊处理：HPE_PAUSED_UPGRADE 是正常的
                    if (parser->getError() == HPE_PAUSED_UPGRADE)
                    {
                        // CONNECT 请求已解析完成，返回 OK
                        return {HttpSession::RecvStatus::OK, parser->getData()};
                    }
                    BLUE_LOG_ERROR(g_logger) << "http request 格式错误";
                    return {HttpSession::RecvStatus::ERROR, nullptr};
                }
                if (m_isFinish)
                {
                    return {HttpSession::RecvStatus::OK, parser->getData()};
                }

                // 数据不够放了
                if (offset >= requestbuffersize)
                {
                    HttpRequestParser::SetRequestBufferSize(offset * 2); // 热更新
                    size_t newsize = HttpRequestParser::GetHttpRequestBufferSize();

                    // 无脑vector 管理data
                    vec_data.resize(newsize);
                    data = vec_data.data();
                    requestbuffersize = newsize;
                }
            } while (true);

            return {HttpSession::RecvStatus::ERROR, nullptr};
        }

        int HttpSession::sendResponse(HttpResponse::HttpResponsePtr response, HttpRequest::HttpRequestPtr request)
        {
            std::string encoding = request->getHeader("Accept-Encoding", "");
            // BLUE_LOG_INFO(g_logger) << "query : " << request->getQuery() << " val : " << request->getParam("target");
            std::string body = response->getBody();
    
            if (!body.empty() && checkEncoding(encoding, "gzip"))
            {
                if (body.size() > 20)
                {
                    body = response->compress(body);
                    response->setBody(body);
                    response->setHeader("Content-Length", std::to_string(body.size()));
                    response->setHeader("Content-Encoding", "gzip");
                }
            }
            std::string str = response->toString();
            BLUE_LOG_WARN(g_logger) << "=== Final HTTP Response Headers ===";
            size_t header_end = str.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                BLUE_LOG_WARN(g_logger) << str.substr(0, header_end);
            }
            return m_stream->writeFixSize(str.c_str(), str.size());
        }

        bool HttpSession::checkEncoding(const std::string &encoding, const char *tem)
        {
            std::stringstream ss(encoding);
            std::string item;
            bool ans = false;
            while (std::getline(ss, item, ','))
            {
                item.erase(0, item.find_first_not_of(' '));
                item.erase(item.find_last_not_of(' ') + 1);
                if (item == tem || item.find(tem) != std::string::npos)
                {
                    ans = true;
                    break;
                }
            }
            return ans;
        }

    }
}
