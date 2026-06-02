#pragma once
#include "spscqueue.h"
#include "logentry.h"
#include <thread>
#include <atomic>
#include <fstream>
#include <functional>
#include <sys/time.h>

namespace blue
{

    // 异步日志器：后台线程 + 无锁队列 + 自动轮转
    class AsyncLogger
    {
    public:
        using Callback = std::function<void(const LogEntry &)>;

        /**
         * @brief 构造函数，启动后台日志线程
         * @param file_pattern  日志文件路径模式（如 "logs/app_%Y%m%d_%H%M%S.log"）
         * @param max_file_size 单个文件最大大小（字节），超过则轮转
         * @param queue_size    无锁队列容量（必须是 2 的幂）
         */
        AsyncLogger(const std::string &file_pattern,
                    size_t max_file_size = 10 * 1024 * 1024,  // 默认 10MB
                    size_t queue_size = 65536);                // 默认 64K
        ~AsyncLogger();

        /**
         * @brief 放入一条日志（无阻塞，生产者线程调用）
         * @param entry 日志条目（移动语义）
         */
        void enqueue(LogEntry &&entry);

        /**
         * @brief 设置额外回调（如同时输出到控制台、网络等）
         * @param cb 回调函数
         */
        void setCallback(Callback cb) { m_callback = std::move(cb); }

        /**
         * @brief 停止日志线程，清空队列后退出
         */
        void stop();

        /**
         * @brief 获取当前毫秒级时间戳
         */
        static uint64_t currentTimeMs();

    private:
        // 后台线程主循环：从队列取日志，格式化输出，检查轮转
        void run();

        // 关闭当前文件，创建新文件
        void rotate();

        // 根据 file_pattern 和时间生成文件名
        std::string generateFileName() const;

        // 写入数据并检查是否需要轮转
        void writeToFile(const std::string &data);

        std::string m_filePattern;       // 文件路径模式
        size_t m_maxFileSize;            // 单个文件最大大小
        size_t m_currentSize = 0;        // 当前文件已写入大小

        SPSCQueue<LogEntry> m_queue;     // 无锁单生产者单消费者队列
        std::thread m_worker;            // 后台日志线程
        std::atomic<bool> m_stop{false}; // 停止标志
        std::ofstream m_file;            // 当前输出文件
        Callback m_callback;             // 额外回调（如控制台输出）
    };

} // namespace blue