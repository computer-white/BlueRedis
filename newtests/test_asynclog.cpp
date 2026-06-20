#include "blue/async_logger.h"
#include <iostream>
#include <thread>
#include <chrono>

static blue::AsyncLogger g_logger("logs/test_%Y%m%d_%H%M%S.log", 1024);

int main() {
    std::cout << "=== 异步日志测试（无宏） ===" << std::endl;

    // 直接构造 LogEntry 并提交
    for (int i = 0; i < 10; i++) {
        blue::LogEntry entry;
        entry.timestamp_ms = blue::AsyncLogger::currentTimeMs();
        entry.level = blue::LogLevel::INFO;
        entry.logger_name = "test";
        entry.message = "测试日志 " + std::to_string(i);
        g_logger.enqueue(std::move(entry));
    }

    // 多线程测试
    std::thread t1([&] {
        for (int i = 0; i < 5; i++) {
            blue::LogEntry entry;
            entry.timestamp_ms = blue::AsyncLogger::currentTimeMs();
            entry.level = blue::LogLevel::WARN;
            entry.logger_name = "thread1";
            entry.message = "线程1 日志 " + std::to_string(i);
            g_logger.enqueue(std::move(entry));
        }
    });

    std::thread t2([&] {
        for (int i = 0; i < 5; i++) {
            blue::LogEntry entry;
            entry.timestamp_ms = blue::AsyncLogger::currentTimeMs();
            entry.level = blue::LogLevel::ERROR;
            entry.logger_name = "thread2";
            entry.message = "线程2 日志 " + std::to_string(i);
            g_logger.enqueue(std::move(entry));
        }
    });

    t1.join();
    t2.join();

    std::cout << "等待日志线程清空队列..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    g_logger.stop();

    std::cout << "=== 测试完成，检查 logs/ 目录 ===" << std::endl;
    return 0;
}