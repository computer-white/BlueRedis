/**
 * @file cluster_peer.h
 * @brief 集群节点之间通信连接
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.7.17
 * @copyright Copyright(c) 2026年 blue
 */
#pragma once
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

#include "cluster_message.h"

namespace blue
{
    namespace cluster
    {
        enum class PeerState
        {
            DISCONNECTED = 0, // 未连接
            CONNECTING,       // 连接中
            CONNECTED,        // 已连接
            HANDSHAKING,      // 握手中
            READY,            // 就绪
            ERROR,            // 错误
            CLOSED            // 已关闭
        };

        std::string peerStateToString(PeerState state);

        // 连接配置
        struct PeerConfig
        {
            int socket_timeout_ms = 5000;     // socket超时(ms)
            int reconnect_interval_ms = 1000; // 重连间隔
            int max_reconnect_attempts = 10;  // 最大重连次数
            int send_buffer_size = 65536;     // 发送缓冲区大小
            int recv_buffer_size = 65536;     // 接受缓冲区大小
            bool keep_alive = true;           // 是否启用 TCP KeepAlive
            int keep_alive_idle = 30;         // KeepAlive 空闲时间（秒）
            int keep_alive_interval = 10;     // KeepAlive 间隔（秒）
            int keep_alive_count = 3;         // KeepAlive 探测次数
        };

        // 集群节点连接
        class ClusterPeer
        {
        public:
            // 消息回调类型
            using MessageCallBack = std::function<void(const Message &)>;
            // 状态变化回调
            using StateCallBack = std::function<void(PeerState old_state, PeerState new_state)>;
            // 错误回调
            using ErrorCallBack = std::function<void(const std::string& error)>;

        public:
            ClusterPeer(uint32_t node_id, const std::string &ip, uint16_t port);
            ~ClusterPeer();

            /**
             * @brief 主动连接
             */
            bool connect();

            /**
             * @brief 断开连接
             */
            void disconnect();

            /**
             * @brief 是否已经连接
             */
            bool isConnected() const noexcept { return m_state.load(std::memory_order_acquire) == PeerState::CONNECTED ||
                                                m_state.load(std::memory_order_acquire) == PeerState::READY; }

            /**
             * @brief 是否就绪
             */
            bool isReady() const noexcept { return m_state.load(std::memory_order_acquire) == PeerState::READY; }
            
            /**
             * @brief 获取状态
             */
            PeerState getState() const noexcept { return m_state.load(std::memory_order_acquire); }

            /**
             * @brief 发送消息(放到队列中)
             * @param msg 消息
             */
            bool sendMessage(const Message &msg);

            /**
             * @brief 发送消息(同步)
             * @param msg 消息
             */
            bool sendMessageSync(const Message &msg);

            /**
             * @brief 获取发送队列大小
             */
            size_t getSendQueueSize() const;

            // 回调注册
            void onMessage(MessageCallBack msgcb);


        private:
            /**
             * @brief 接受循环
             */
            void receiveLoop();

            /**
             * @brief 发送循环
             */
            void sendLoop();

            /**
             * @brief 解析消息
             */
            bool readMessage(Message &msg);

            /**
             * @brief 写消息
             */
            bool writeMessage(const Message &msg);

            // Socket
            std::atomic<int> m_socket_fd;
            std::atomic<PeerState> m_state;

            // 配置
            PeerConfig m_config;

            // 消息队列
            std::condition_variable m_queue_cv;
            mutable std::mutex m_queue_mutex;
            std::queue<Message> m_send_queue;

            // 线程控制
            std::atomic<bool> m_running;
            std::unique_ptr<std::thread> m_receive_thread;
            std::unique_ptr<std::thread> m_send_thread;

            // 回调
            MessageCallBack m_message_callback;
            StateCallBack m_state_callback;
            ErrorCallBack m_error_callback;
        };
    } // namespace cluster
}
