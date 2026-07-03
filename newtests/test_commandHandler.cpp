#include <csignal>
#include <atomic>
#include "redis_command/command_handler.h"
#include "blue/address.h"
#include "blue/task.h"
#include "blue/log.h"
#include "blue/await.h"

using namespace blue;
static Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

std::atomic<bool> g_running(true);
IOManager* g_iom = nullptr;
void signalHandler(int signum) {
    BLUE_LOG_INFO(g_logger) << "Received signal " << signum << ", shutting down...";
    g_running.store(false);
}

Task<void> test(const std::string host)
{
    // // 注册信号处理
    // signal(SIGINT, signalHandler);
    // signal(SIGTERM, signalHandler);
    auto address = Address::LookupAnyIpAddress(host);
    if (!address)
    {
        BLUE_LOG_ERROR(g_logger) << "address is empty";
        co_return;
    }
    auto comm = std::make_shared<CommandHandler<int>>();
    while (!comm->bind(address))
    {
        co_await sleepFor(2);
    }
    bool ans = co_await comm->start();
    if (ans)
    {
        BLUE_LOG_INFO(g_logger) << "Server started successfully on port 6666";
    }
    // 保持服务器运行，直到收到停止信号
    while (!comm->getIsStop())
    {
        co_await sleepFor(1);
    }
    BLUE_LOG_INFO(g_logger) << "Server stopped";
    IOManager::GetThis()->clear();
    co_return;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        BLUE_LOG_INFO(g_logger) << "need host and port";
        return -1;
    }
    const char* host = argv[1];
    const char* port = argv[2];
    BLUE_LOG_INFO(g_logger) << host << ":" << port;
    std::string host_with_port;
    host_with_port.append(host, strlen(host));
    host_with_port += ":";
    host_with_port.append(port, strlen(port));

    BLUE_LOG_INFO(g_logger) << "main begin";
    
    IOManager iom(2);
    iom.schedule(test(host_with_port));
    
    BLUE_LOG_INFO(g_logger) << "calling wait_all()";
    iom.wait_all();
    BLUE_LOG_INFO(g_logger) << "wait_all() returned";
    
    return 0;
}