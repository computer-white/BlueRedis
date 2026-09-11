 /*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 /**
 * @file AOF.h
 * @brief redis server AOF文件写入模块
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.26
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <fstream>
#include <functional>
#include <string>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include "blue/task.h"
#include "blue/resp_parser.h"
#include "blue/msocket.h"

namespace blue
{
    class AOFModule
    {
    public:
        using TimePoint = std::chrono::steady_clock::time_point;
        using ExecuteFunc = std::function<RespValue(std::vector<RespValue>, MSocket::MSocketPtr, bool)>;
        AOFModule() = default;
        ~AOFModule() = default;
        AOFModule(const AOFModule& ) = delete;
        AOFModule& operator=(const AOFModule& ) = delete;
    private:
        // AOF 异步
        struct AOFBuffer
        {
            std::mutex aof_mutex;                   // 互斥变量
            std::condition_variable aof_cv;         // 条件变量
            std::string aof_buffer;                 // 缓冲区
            std::atomic<size_t> aof_buffer_size{0}; // 缓冲区大小
            bool aof_flush_requested = false;       // 刷新请求
        };
    public:

        /**
         * @brief 设置回调execute
         */
        void setExecutor(ExecuteFunc func) { m_executor = func; }

        /**
         * @brief 供aofFlushThread来检测服务器是否停止
         */
        void stop() { m_stop.store(true, std::memory_order_release); }
        /**
         * @brief 初始化AOF
         */
        void initAOF();

        /**
         * @brief 追加命令到AOF
         * @param cmd 命令
         */
        void appendToAOF(const std::string &cmd);

        /**
         * @brief 加载AOF
         */
        void loadAOF();

        /**
         * @brief 判断是否是写命令
         * @param cmd 命令
         */
        bool isWriteCommand(const std::string &cmd);

        /**
         * @brief 格式化命令为RESP字符转
         * @param args 命令数组
         */
        std::string formatCommand(const std::vector<RespValue> &args);

        /**
         * @brief 按策略循环后台同步AOF文件
         */
        Task<void> aofSyncLoop();

        /**
         * @brief 轮转
         */
        void rotateAOF();

        /**
         * @brief 获取文件名
         */
        std::string getAOFFilename(int index);

        /**
         * @brief 判断文件是否是旧文件并执行删除，若最后文件名可使用返回true
         * @param filename 需要判断的文件名
         *  true 表示可以使用
         */
        bool cleanupOldAOFs(const std::string &filename);

        /**
         * @brief 开启AOF刷新线程
         */
        void startAOFFlushThread();

        /**
         * @brief 停止刷新线程
         */
        void stopAOFFlushThread();

        /**
         * @brief 刷新线程工作函数
         */
        void aofFlushThread();

        /**
         * @brief 取出缓冲区内容,写入文件
         */
        void flushAOFBuffer();
    public:

        /**
         * @brief 获取last_aof_sync
         */
        TimePoint getLastAOFSync() const noexcept { return m_last_aof_sync; }

        /**
         * @brief 更新last_aof_sync
         */
        void setLastAOFSync(TimePoint now) noexcept { m_last_aof_sync = now; }

        /**
         * @brief 获取aof_rotating
         */
        bool getAOFRotating() const { return m_aof_rotating.load(std::memory_order_acquire); }

        /**
         * @brief 获取当前文件名
         */
        const std::string &getCurrentFileName() const { return m_aof_current_filename; }

        /**
         * @brief 获取当前文件编号
         */
        size_t getCurrentFileIdx() const noexcept { return m_aof_file_idx; }

        /**
         * @brief 获取文件大小
         */
        size_t getCurrentFileSize()
        {
            if (m_aof_file.is_open())
            {
                return (size_t)(m_aof_file.tellp());
            }
            return 0;
        }

        /**
         * @brief 刷新并关闭aof流
         */
        void closeAOFWithFlush() { if (m_aof_file.is_open()) { m_aof_file.flush(); m_aof_file.close(); } }

        /**
         * @brief 关闭aof流
         */
        void closeAOF() { if (m_aof_file.is_open()) { m_aof_file.close(); } }

    private:

        // 异步变量
        AOFBuffer m_aof_buffer;                         // 异步aof缓存
        std::thread m_aof_flush_thread;                 // 刷新线程
        std::atomic<bool> m_aof_flush_running{false};   // 刷新线程是否正在运行中

        // 基础变量
        std::shared_mutex m_aof_mutex;           // 互斥变量
        std::string m_aof_current_filename;      // 当前文件名
        std::ofstream m_aof_file;                // 当前打开的文件流
        size_t m_aof_file_idx = 0;               // 当前文件编号
        TimePoint m_last_aof_sync;               // 最新一次写入时间
        std::atomic<bool> m_aof_rotating{false}; // 轮转标志

        // 回调
        ExecuteFunc m_executor;
        std::atomic<bool> m_stop{false};
    };
}