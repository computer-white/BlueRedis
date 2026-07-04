/**
 * @file logentry.h
 * @brief 日志信息条目，搭配async_logger使用
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.24
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace blue
{

    // 日志级别
    enum class LogLevel
    {
        DEBUG,  // 调试信息
        INFO,   // 一般信息
        WARN,   // 警告
        ERROR,  // 错误
        FATAL   // 致命错误
    };

    // 日志条目（生产者构造，消费者格式化输出）
    struct LogEntry
    {
        uint64_t timestamp_ms;   // 日志产生时间（毫秒级时间戳）
        LogLevel level;          // 日志级别
        std::string logger_name; // 日志器名称（如 "system"、"http"、"db"）
        std::string message;     // 日志内容
        std::string file;        // 源文件名（__FILE__）
        int line;                // 源文件行号（__LINE__）

        // 格式化成最终输出字符串（消费者线程调用，不阻塞生产者）
        std::string format() const
        {
            auto tp = std::chrono::system_clock::from_time_t(timestamp_ms / 1000);
            auto ms = timestamp_ms % 1000;
            std::time_t t = std::chrono::system_clock::to_time_t(tp);
            std::tm tm;
            localtime_r(&t, &tm);

            std::ostringstream ss;
            // 时间戳格式：2026-06-02 18:30:45.123
            ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
               << '.' << std::setfill('0') << std::setw(3) << ms;
            // 级别和日志器
            ss << " [" << levelToString(level) << "] ";
            ss << "[" << logger_name << "] ";
            // 日志内容
            ss << message;
            // 可选：源码位置
            if (!file.empty())
            {
                ss << " (" << file << ":" << line << ")";
            }
            ss << "\n";
            return ss.str();
        }

        // 日志级别转字符串
        static const char *levelToString(blue::LogLevel level)
        {
            switch (level)
            {
            case blue::LogLevel::DEBUG: return "DEBUG";
            case blue::LogLevel::INFO:  return "INFO ";
            case blue::LogLevel::WARN:  return "WARN ";
            case blue::LogLevel::ERROR: return "ERROR";
            case blue::LogLevel::FATAL: return "FATAL";
            }
            return "UNKNOW";
        }
    };

} // namespace blue