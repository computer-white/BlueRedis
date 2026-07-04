 /**
 * @file slowlog.h
 * @brief redis server 慢查询模块
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.26
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include "blue/spscqueue.h"
#include "blue/msocket.h"


namespace blue
{
    /**
     * @brief 慢查询模块
     * 负责记录一些查询速度慢的redis命令,供给调试
     */
    class SlowLogModule
    {
        struct SlowLogEntry
        {
            int64_t id;                                      // 自增id
            std::chrono::system_clock::time_point timestamp; // 时间戳
            std::chrono::microseconds duration;              // 执行时间(微秒)
            std::string command;                             // 命令字符串
            std::string client_ip;                           // 客户端ip
        };
    public:
        SlowLogModule() = default;
        ~SlowLogModule() = default;

        SlowLogModule(const SlowLogModule& ) = delete;
        SlowLogModule& operator=(const SlowLogModule& ) = delete;
    public:
        /**
         * @brief 将慢查询日志队列内容同步进入m_slow_logs_cache
         */
        void syncSlowLogs();

        /**
         * @brief 记录命令到m_slow_logs里面
         * @param args 命令数组
         * @param sock 客户端socket 
         * @param start 命令开始执行时间
         */
        void pushEntry(const std::string &cmd_str, MSocket::MSocketPtr sock, 
            std::chrono::steady_clock::time_point start, 
            std::chrono::steady_clock::time_point end);

        /**
         * @brief 获取慢查询记录
         * @param count 获取count条记录
         */
        std::vector<RespValue> getSlowLogs(int64_t count);

        /**
         * @brief 获取记录缓存大小
         */
        size_t len() const;

        /**
         * @brief 重置
         */
        void reset();

        /**
         * @brief 获取阈值
         */
        int64_t getSlowLogThan() const { return m_slow_log_slower_than.load(std::memory_order_acquire); }

        /**
         * @brief 获取最大保存条数
         */
        int64_t getSlowMaxLen() const { return m_slow_log_max_len.load(std::memory_order_acquire); }
        /**
        * @brief 设置阈值
         */
        void setSlowLogThan(int64_t val) { return m_slow_log_slower_than.store(val,std::memory_order_release); }

        /**
         * @brief 设置最大保存条数
         */
        void setSlowMaxLen(int64_t val) { return m_slow_log_max_len.store(val,std::memory_order_release); }
    private:
        SPSCQueue<SlowLogEntry, 2048> m_slow_logs;              // 慢查询日志队列
        mutable std::shared_mutex m_slow_logs_cache_mutex;      // 日志锁
        std::vector<SlowLogEntry> m_slow_logs_cache;            // 用于查询的缓存

        std::atomic<int64_t> m_slow_log_id{0};              // 自增ID
        std::atomic<int64_t> m_slow_log_slower_than{10000}; // 阈值（微秒），默认10ms
        std::atomic<size_t> m_slow_log_max_len{128};        // 最大保存条数
    };
}