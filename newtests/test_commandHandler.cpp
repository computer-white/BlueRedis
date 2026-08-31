#include <csignal>
#include <atomic>
#include <getopt.h>
#include "redis_command/command_handler.h"
#include "blue/address.h"
#include "blue/task.h"
#include "blue/log.h"
#include "blue/await.h"

using namespace blue;
static Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

Task<void> runServer(const std::string host)
{
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

void print_usage(const char *prog_name)
{
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -b, --bind <ip>     Bind address (default: 127.0.0.1)\n"
              << "  -p, --port <port>   Listen port (default: 6666)\n"
              << "  -h, --help          Show this help message\n"
              << "\nExample:\n"
              << "  " << prog_name << "                    # Start with defaults\n"
              << "  " << prog_name << " -b 0.0.0.0 -p 6379 # Bind all interfaces\n"
              << "  " << prog_name << " --help             # Show help\n";
}

int main(int argc, char *argv[])
{
    // 默认配置
    std::string bind_host = "127.0.0.1";
    std::string port = "6666";

    // 定义长参数选项
    static struct option long_options[] = {
        {"bind", required_argument, 0, 'b'},
        {"port", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "b:p:h", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'b':
            bind_host = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    // 构造地址字符串
    std::string address_str = bind_host + ":" + port;
    BLUE_LOG_INFO(g_logger) << "Starting BlueRedis with address: " << address_str;

    // 启动调度器和服务器协程
    IOManager iom(2);
    iom.schedule(runServer(address_str));

    BLUE_LOG_INFO(g_logger) << "Event loop started, waiting for tasks...";
    iom.wait_all();
    BLUE_LOG_INFO(g_logger) << "Event loop finished";

    return 0;
}