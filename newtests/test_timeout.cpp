#include <iostream>
#include <thread>
#include <chrono>
#include "blue/io_manager.h"
#include "blue/msocket.h"
#include "blue/address.h"
#include "blue/await.h"
#include "blue/task.h"
#include "blue/log.h"

using namespace blue;
static Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

// 测试 1: 简单的 recv 超时测试（没有数据时超时）
Task<void> testRecvTimeout(MSocket::MSocketPtr sock, uint64_t timeout_ms) {
    char buf[1024];
    
    BLUE_LOG_INFO(g_logger) << "Testing recvT with " << timeout_ms << "ms timeout";
    
    auto start = std::chrono::steady_clock::now();
    ssize_t ret = co_await sock->recvT(buf, sizeof(buf), 0, timeout_ms);
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    if (ret == -1 && errno == ETIMEDOUT) {
        BLUE_LOG_INFO(g_logger) << "✅ recvT timed out correctly after " << elapsed << "ms";
    } else if (ret > 0) {
        BLUE_LOG_INFO(g_logger) << "recvT received " << ret << " bytes (unexpected)";
    } else {
        BLUE_LOG_INFO(g_logger) << "recvT returned " << ret << ", errno=" << errno << " (" << strerror(errno) << ")";
    }
    
    co_return;
}

// 测试 2: 先有数据后超时
Task<void> testRecvWithData(MSocket::MSocketPtr sock, uint64_t timeout_ms) {
    char buf[1024];
    
    BLUE_LOG_INFO(g_logger) << "Testing recvT with data then timeout";
    
    // 第一次 recv: 等待数据
    auto start = std::chrono::steady_clock::now();
    ssize_t ret = co_await sock->recvT(buf, sizeof(buf), 0, timeout_ms);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    if (ret > 0) {
        BLUE_LOG_INFO(g_logger) << "✅ Received " << ret << " bytes after " << elapsed << "ms";
    } else {
        BLUE_LOG_INFO(g_logger) << "❌ recvT returned " << ret << ", errno=" << errno;
    }
    
    // 第二次 recv: 应该超时（没有更多数据）
    start = std::chrono::steady_clock::now();
    ret = co_await sock->recvT(buf, sizeof(buf), 0, timeout_ms);
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    if (ret == -1 && errno == ETIMEDOUT) {
        BLUE_LOG_INFO(g_logger) << "✅ Second recvT timed out correctly after " << elapsed << "ms";
    } else {
        BLUE_LOG_INFO(g_logger) << "Second recvT returned " << ret << ", errno=" << errno;
    }
    
    co_return;
}

// 测试 3: 多个并发的超时测试
Task<void> testMultipleTimeouts(MSocket::MSocketPtr sock) {
    std::vector<uint64_t> timeouts = {100, 500, 1000, 2000};
    char buf[1024];
    
    for (auto ms : timeouts) {
        BLUE_LOG_INFO(g_logger) << "Testing " << ms << "ms timeout";
        auto start = std::chrono::steady_clock::now();
        ssize_t ret = co_await sock->recvT(buf, sizeof(buf), 0, ms);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        
        if (ret == -1 && errno == ETIMEDOUT) {
            BLUE_LOG_INFO(g_logger) << "  ✅ " << ms << "ms: timed out after " << elapsed << "ms";
        } else {
            BLUE_LOG_INFO(g_logger) << "  ❌ " << ms << "ms: returned " << ret << ", errno=" << errno;
        }
    }
    
    co_return;
}

// 服务器端：接受连接并发送数据
Task<void> serverHandler(MSocket::MSocketPtr server_sock) {
    BLUE_LOG_INFO(g_logger) << "Server waiting for connection...";
    
    auto client = co_await server_sock->accept();
    if (!client) {
        BLUE_LOG_ERROR(g_logger) << "Accept failed";
        co_return;
    }
    
    BLUE_LOG_INFO(g_logger) << "Client connected, fd=" << client->getSocketfd();
    client->setNoBlocking();
    
    // 发送一些数据
    const char* msg = "Hello from server!\n";
    co_await client->send(msg, strlen(msg));
    
    BLUE_LOG_INFO(g_logger) << "Data sent, waiting 5 seconds...";
    co_await sleepFor(5);
    
    // 发送更多数据
    const char* msg2 = "Second message\n";
    co_await client->send(msg2, strlen(msg2));
    
    BLUE_LOG_INFO(g_logger) << "Second message sent";
    
    co_await sleepFor(2);
    client->close();
    BLUE_LOG_INFO(g_logger) << "Server done";
}

// 客户端：测试 recvT
Task<void> clientTask(MSocket::MSocketPtr sock) {
    BLUE_LOG_INFO(g_logger) << "Client connected, fd=" << sock->getSocketfd();
    sock->setNoBlocking();
    
    // 测试1: 带数据的情况
    co_await testRecvWithData(sock, 3000);
    
    // 等待一下
    co_await sleepFor(1);
    
    // 测试2: 纯超时（数据已读完）
    co_await testRecvTimeout(sock, 1500);
    
    co_return;
}

int main() {
    // 初始化日志
    auto logger = BLUE_LOG_NAME("system");
    BLUE_LOG_INFO(logger) << "=== Timeout Test Started ===";
    
    IOManager iom(2);
    
    // 创建 TCP 服务器
    auto server_sock = MSocket::CreateTcpSocket();
    auto addr = IPv4Address::Create("127.0.0.1", 8888);

    if (!server_sock->bind(addr)) {
        BLUE_LOG_ERROR(logger) << "Bind failed";
        return 1;
    }
    
    if (!server_sock->listen()) {
        BLUE_LOG_ERROR(logger) << "Listen failed";
        return 1;
    }
    
    BLUE_LOG_INFO(logger) << "Server listening on port 8888";
    
    // 启动服务器协程
    iom.schedule(serverHandler(server_sock));
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // // 客户端连接
    // auto client_sock = MSocket::CreateTcpSocket();
    // BLUE_LOG_INFO(logger) << "Client connecting...";
    
    // bool connected = false;
    // // 简单同步连接（为了测试，使用阻塞方式）
    // for (int i = 0; i < 10; ++i) {
    //     if (client_sock->connect(addr, 3000).get()) {
    //         connected = true;
    //         break;
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
    
    // if (!connected) {
    //     BLUE_LOG_ERROR(logger) << "Client connection failed";
    //     return 1;
    // }
    
    // BLUE_LOG_INFO(logger) << "Client connected";
    // client_sock->setNoBlocking();
    
    // // 启动客户端协程
    // iom.schedule(clientTask(client_sock));
    iom.wait_all();
    BLUE_LOG_INFO(logger) << "=== Test Finished ===";
    return 0;
}