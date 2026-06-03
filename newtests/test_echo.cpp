#include "blue/io_manager.h"
#include "blue/asyncio.h"
#include "blue/await.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
using namespace blue;

// 设置 fd 为非阻塞
void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 处理单个客户端连接
Task<void> handle_client(int client_fd) {
    char buf[1024];
    // std::cout << "[客户端 " << client_fd << "] 已连接" << std::endl;
    BLUE_LOG_INFO(g_logger) << "[客户端 " << client_fd << "] 已连接";
    while (true) {
        ssize_t n = co_await ReadT(client_fd,buf,sizeof(buf),10000);
        if (n <= 0) {
            BLUE_LOG_INFO(g_logger) << "[client " << client_fd << "] 断开 n=" << n;
            break;
        }
        BLUE_LOG_INFO(g_logger) << "[client " << client_fd << "] 收到 " << n << " 字节";
        const char* response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 5\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Hello";
        co_await Write(client_fd, (void*)response, strlen(response));
        break;
    }
    IOManager::GetThis()->delEvent(client_fd, IOManager::READ);
    IOManager::GetThis()->delEvent(client_fd, IOManager::WRITE);
    close(client_fd);
}

// 接受连接
Task<void> accept_loop(int listen_fd) {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        // 先尝试直接 accept
        int client_fd = ::accept(listen_fd, (sockaddr*)&client_addr, &addr_len);
        if (client_fd >= 0) {
            set_nonblock(client_fd);
            // 为每个客户端启动一个协程
            IOManager::GetThis()->schedule(handle_client(client_fd));
            continue;
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 没有新连接，挂起等待
            client_fd = co_await Accept(listen_fd, (sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) continue;  // 错误
            set_nonblock(client_fd);
            IOManager::GetThis()->schedule(handle_client(client_fd));
            continue;
        }
        
        // 错误
        std::cerr << "accept error: " << strerror(errno) << std::endl;
        co_return;
    }
}

Task<void> timer_test() {
    std::cout << "开始等 2 秒..." << std::endl;
    co_await sleepFor(2);
    std::cout << "2 秒到了！" << std::endl;
    co_return;
}

int main() {
    std::cout << "=== Echo Server (C++20 Coroutine) ===" << std::endl;

    // 创建监听 socket
    std::cout << "1. 创建 socket..." << std::endl;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblock(listen_fd);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 128);

    std::cout << "监听端口: 8080" << std::endl;

    // 创建 IOManager
    std::cout << "2. 创建 IOManager..." << std::endl;
    IOManager iom(4);
    
    // 启动 accept 协程
    std::cout << "3. 提交任务..." << std::endl;
    iom.schedule(accept_loop(listen_fd));
    // iom.schedule(timer_test());
    
    // 主线程阻塞
    std::cout << "4. 主线程等待..." << std::endl;
    iom.wait_all();

    return 0;
}
