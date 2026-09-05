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
 * @file replication.h
 * @brief redis server 主从复制模块
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.7.3
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <atomic>
#include <thread>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <queue>
#include "blue/msocket.h"
#include "redis_command/generator.h"

namespace blue
{
    class ReplicationModule
    {
    public:
        // (执行回调)函数
        using ExecuteFunc = std::function<RespValue(std::vector<RespValue>, MSocket::MSocketPtr, bool)>;
        // 默认构造
        ReplicationModule() = default;
        ~ReplicationModule() { this->stopReplication(); }

        // 禁止拷贝
        ReplicationModule(const ReplicationModule &) = delete;
        ReplicationModule &operator=(const ReplicationModule &) = delete;

    public:
        /**
         * @brief 设置回调execute
         */
        void setExecutor(ExecuteFunc func) { m_executor = func; }

        /**
         * @brief 设置服务器停止标识
         */
        void setStop() { m_server_stop.store(true, std::memory_order_release); }

        /**
         * @brief 队列消费者协程
         */
        Task<void> processReplQueue();

        /**
         * @brief 停止复制
         */
        void stopReplication();

        /**
         * @brief 开启复制
         */
        void startReplication();

        /**
         * @brief 添加从节点
         */
        void addSlaves(MSocket::MSocketPtr sock);

        /**
         * @brief 从节点列表是否为空
         */
        bool slavesEmpty() const noexcept
        {
            std::shared_lock<std::shared_mutex> lock(m_slaves_mutex);
            return m_slaves.empty();
        }

        /**
         * @brief 输出slaves信息
         */
        std::string slavesToString();

        /**
         * @brief 获取从节点个数
         */
        size_t slavesCount();

        /**
         * @brief 获取当前复制状态
         */
        uint8_t getReplState() const noexcept { return m_repl_state.load(std::memory_order_acquire); }

        /**
         * @brief 返回在线状态
         */
        uint8_t getOnline() const noexcept { return RelpState::REPL_STATE_ONLINE; }

        /**
         * @brief 删除主节点无法成功发送RDB消息给从节点的从节点
         * @param sock 发送sync给主节点的客户端sock
         */
        void remove(MSocket::MSocketPtr sock);

        /**
         * @brief 从节点复制循环
         */
        void replicationLoop();

        /**
         * @brief 写命令广播给从节点
         */
        void broadcastToSlaves(const std::string &cmd);

        /**
         * @brief 从内存加载 RDB 数据
         */
        bool loadRDBFromMemory(std::shared_ptr<MSocket> sock);

    private:
        /**
         * @brief 流式接收RDB数据
         */
        Generator<std::string> recvRDBData(std::shared_ptr<MSocket> sock);

    private:
        enum RelpState : uint8_t
        {
            REPL_STATE_NONE = 0,   // 未开始
            REPL_STATE_CONNECTING, // 连接中
            REPL_STATE_HANDSHAKE,  // 握手
            REPL_STATE_TRANSFER,   // 传输中
            REPL_STATE_ONLINE,     // 在线
            REPL_STATE_RETRY       // 重试
        };

    private:
        // 从节点连接
        std::shared_ptr<MSocket> m_repl_sock;                 // 作为从节点，连接到主节点时，连接成功时的SocketPtr
        std::atomic<RelpState> m_repl_state{REPL_STATE_NONE}; // 主从复制状态
        std::thread m_relp_thread;                            // 主从复制循环线程
        std::atomic<bool> m_repl_stop{true};                  // 主从复制停止表示

        // 从节点列表
        mutable std::shared_mutex m_slaves_mutex;   // 从节点列表读写互斥变量
        std::vector<MSocket::MSocketWPtr> m_slaves; // 从节点列表
    private:
        // 服务器停止
        std::atomic<bool> m_server_stop{false}; // 服务器停止标识

        // 复制命令队列
        struct ReplCommand
        {
            std::vector<RespValue> args;
        };

        std::mutex m_repl_queue_mutex;               // 复制队列互斥变量
        std::queue<ReplCommand> m_repl_queue;        // 主从复制进入在线模式时，接收到主节点的写命令队列
        std::condition_variable m_repl_queue_cv;     // 队列条件变量，搭配unique_lock
        std::atomic<bool> m_repl_queue_stop{false};  // 复制队列停止标识
        std::atomic<bool> m_consumer_started{false}; // 消费者协程开启原子标识

        // 回调
        ExecuteFunc m_executor; // 执行命令的回调函数
    };
}