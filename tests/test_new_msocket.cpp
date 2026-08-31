#include "blue/msocket.h"
#include "blue/address.h"
#include "blue/io_manager.h"
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger  = BLUE_LOG_MASSAGE_ROOT();
using namespace blue;
Task<void> test()
{
    auto addr = Address::LookupAnyIpAddress("www.baidu.com");
    if (!addr)
    {
        BLUE_LOG_ERROR(g_logger) << "get address failed";
        co_return;
    }
    addr->setPort(80);
    BLUE_LOG_INFO(g_logger) << "get address : " << addr->toString();
    auto sock = MSocket::CreateTcp(addr);
    if (!co_await sock->connect(addr))
    {
        BLUE_LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed";
        co_return;
    }
    BLUE_LOG_INFO(g_logger) << "connect : " << addr->toString() << " connect successful";
    
    const char* request = 
    "GET / HTTP/1.0\r\n"
    "Host: www.baidu.com\r\n"
    "Connection: close\r\n"
    "\r\n";
    ssize_t rt = co_await sock->send(request, strlen(request), 0);
    if (rt <= 0) {
        BLUE_LOG_ERROR(g_logger) << "send failed";
        co_return;
    }

    char buf[4096];
    rt = co_await sock->recv(buf, sizeof(buf) - 1, 0);
    if (rt > 0) {
        buf[rt] = '\0';
        BLUE_LOG_INFO(g_logger) << buf;
    }

    BLUE_LOG_INFO(g_logger) << "bufsize : " << rt;
    co_return;
}

int main()
{
    blue::IOManager iom(4);
    iom.schedule(test());
    iom.wait_all();
    BLUE_LOG_INFO(g_logger) << "结束";
    return 0;
}