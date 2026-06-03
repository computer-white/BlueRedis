#include "http/httpconnection.h"
#include "blue/address.h"
#include "blue/await.h"
#include <blue/io_manager.h>
#include <map>

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

blue::Task<void> test()
{
    auto address = blue::Address::LookupAnyIpAddress("httpbin.org");
    if (!address)
    {
        BLUE_LOG_ERROR(g_logger) << "get address error, error : " << errno << " strerror : " << strerror(errno);
        co_return;
    }

    auto sock = blue::MSocket::CreateTcp(address);
    bool ret = co_await sock->connect(address);
    if (!ret)
    {
        BLUE_LOG_ERROR(g_logger) << "connect error, address : " << address->toString();
        co_return;
    }

    auto stream = std::make_shared<blue::SocketStream>(sock);
    auto httpconnection = std::make_shared<blue::http::HttpConnection>(stream);
    auto request = std::make_shared<blue::http::HttpRequest>();
    request->setMethod(blue::http::HttpMethod::GET);
    // request->setPath("/gzip");   // 测试gzip ok
    // request->setPath("/stream-bytes/256?chunk_size=32");  // 测试chunk,返回256字节,每块32字节 ok
    // request->setPath("/stream/10");    // 测试chunk ok
    request->setPath("/drip?numbytes=128&duration=5&delay=1");  // 测试流式处理 ok
    request->setHeader("Host", "httpbin.org");
    httpconnection->setStreaming(true);
    co_await httpconnection->sendRequest(request);
    auto [status,response] = co_await httpconnection->recvResponse();
    if (status != blue::http::HttpConnection::RecvStatus::OK)
    {
        BLUE_LOG_ERROR(g_logger) << "response error, request : \n" << request->toString();
        co_return;
    }
    BLUE_LOG_INFO(g_logger) << "response : \n" << response->toString();
    BLUE_LOG_INFO(g_logger) << "response body size : " << response->getBody().size();

    // BLUE_LOG_INFO(g_logger) << "===============================================";
    // auto retGet =  co_await blue::http::HttpConnection::DoGet("http://www.baidu.com",300);
    // BLUE_LOG_INFO(g_logger) << "result : " << retGet->result
    //                         << " error : " << retGet->error
    //                         << " response : " << retGet->response->toString();
    
    // BLUE_LOG_INFO(g_logger) << "===============================================";


}


int main()
{
    blue::IOManager iom(2);
    iom.schedule(test());
    iom.wait_all();
}