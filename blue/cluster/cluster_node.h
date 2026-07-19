/**
 * @file cluster_node.h
 * @brief 集群节点管理
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.7.17
 * @copyright Copyright(c) 2026年 blue
 */
#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace blue
{
    namespace cluster
    {
        enum class NodeState
        {
            OFFLINE = 0, // 离线
            ONLINE,      // 在线
            FATL,        // 故障
            HANDSHAKE,   // 握手中
            SYNCING,     // 同步中
            LEAVING,     // 离开中
        };

        std::string nodeStateToString(NodeState state);

        struct ClusterNode
        {
            using TimePoint = std::chrono::steady_clock::time_point;

        public:
            // 默认构造函数
            ClusterNode()
                : id(0), port(6666), cluster_port(16379),
                  state(std::make_unique<std::atomic<NodeState>>(NodeState::OFFLINE)),
                  is_master(true), master_id(0),
                  config_epoch(0), current_epoch(0),
                  cmd_count(std::make_unique<std::atomic<uint64_t>>(0)),
                  bytes_received(std::make_unique<std::atomic<uint64_t>>(0)),
                  bytes_sent(std::make_unique<std::atomic<uint64_t>>(0)),
                  ping_count(std::make_unique<std::atomic<uint64_t>>(0)),
                  pong_count(std::make_unique<std::atomic<uint64_t>>(0)) {}

            // 带参数的构造函数
            ClusterNode(uint32_t id, const std::string &ip, uint16_t port, uint16_t cluster_port)
                : id(id), ip(ip), port(port), cluster_port(cluster_port),
                  state(std::make_unique<std::atomic<NodeState>>(NodeState::OFFLINE)),
                  is_master(true), master_id(0),
                  config_epoch(0), current_epoch(0),
                  cmd_count(std::make_unique<std::atomic<uint64_t>>(0)),
                  bytes_received(std::make_unique<std::atomic<uint64_t>>(0)),
                  bytes_sent(std::make_unique<std::atomic<uint64_t>>(0)),
                  ping_count(std::make_unique<std::atomic<uint64_t>>(0)),
                  pong_count(std::make_unique<std::atomic<uint64_t>>(0)) {}
            
            // 拷贝构造函数（深拷贝 atomic 的值）
            ClusterNode(const ClusterNode& other)
                : id(other.id), ip(other.ip), port(other.port), cluster_port(other.cluster_port),
                  state(std::make_unique<std::atomic<NodeState>>(other.state->load(std::memory_order_acquire))),
                  is_master(other.is_master), master_id(other.master_id),
                  slaves(other.slaves), slots(other.slots),
                  last_ping(other.last_ping), last_pong(other.last_pong), last_seen(other.last_seen),
                  cmd_count(std::make_unique<std::atomic<uint64_t>>(other.cmd_count->load(std::memory_order_acquire))),
                  bytes_received(std::make_unique<std::atomic<uint64_t>>(other.bytes_received->load(std::memory_order_acquire))),
                  bytes_sent(std::make_unique<std::atomic<uint64_t>>(other.bytes_sent->load(std::memory_order_acquire))),
                  ping_count(std::make_unique<std::atomic<uint64_t>>(other.ping_count->load(std::memory_order_acquire))),
                  pong_count(std::make_unique<std::atomic<uint64_t>>(other.pong_count->load(std::memory_order_acquire))),
                  config_epoch(other.config_epoch), current_epoch(other.current_epoch) {}
            
            // 拷贝赋值运算符
            ClusterNode& operator=(const ClusterNode& other)
            {
                if (this != &other)
                {
                    id = other.id;
                    ip = other.ip;
                    port = other.port;
                    cluster_port = other.cluster_port;
                    state->store(other.state->load(std::memory_order_acquire), std::memory_order_release);
                    is_master = other.is_master;
                    master_id = other.master_id;
                    slaves = other.slaves;
                    slots = other.slots;
                    last_ping = other.last_ping;
                    last_pong = other.last_pong;
                    last_seen = other.last_seen;
                    cmd_count->store(other.cmd_count->load(std::memory_order_acquire), std::memory_order_release);
                    bytes_received->store(other.bytes_received->load(std::memory_order_acquire), std::memory_order_release);
                    bytes_sent->store(other.bytes_sent->load(std::memory_order_acquire), std::memory_order_release);
                    ping_count->store(other.ping_count->load(std::memory_order_acquire), std::memory_order_release);
                    pong_count->store(other.pong_count->load(std::memory_order_acquire), std::memory_order_release);
                    config_epoch = other.config_epoch;
                    current_epoch = other.current_epoch;
                }
                return *this;
            }

            // 移动构造函数
            ClusterNode(ClusterNode&& other) noexcept = default;
            // 移动赋值运算符
            ClusterNode& operator=(ClusterNode&& other) noexcept = default;

            /**
             * @brief 判断节点是否存活
             */
            bool isAlive() const noexcept
            {
                return state->load(std::memory_order_acquire) == NodeState::ONLINE;
            }

            /**
             * @brief 判断节点是否故障
             */
            bool isFailed() const noexcept
            {
                return state->load(std::memory_order_acquire) == NodeState::FATL;
            }

            // 更新心跳
            void updatePing()
            {
                last_ping = std::chrono::steady_clock::now();
                ping_count->fetch_add(1, std::memory_order_acq_rel);
            }

            void updatePong()
            {
                last_pong = std::chrono::steady_clock::now();
                last_seen = last_pong;
                pong_count->fetch_add(1, std::memory_order_acq_rel);
            }

            /**
             * @brief 检查是否超时
             * @param timeout_seconds 超时时长(s),默认5s
             */
            bool isTimeout(int timeout_seconds = 5) const
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_seen);
                return elapsed.count() > timeout_seconds;
            }

        public:
            uint32_t id;           // 节点ID
            std::string ip;        // IP地址
            uint16_t port;         // 数据端口
            uint16_t cluster_port; // 集群通信端口

            std::unique_ptr<std::atomic<NodeState>> state; // 节点状态
            bool is_master;                                // 是否为主节点
            uint32_t master_id;                            // 主节点ID
            std::vector<uint32_t> slaves;                  // 从节点列表

            // 槽位信息
            std::vector<uint16_t> slots; // 该节点负责的槽位

            // 时间信息
            TimePoint last_ping; // 最新ping的时间
            TimePoint last_pong; // 最新pong的时间
            TimePoint last_seen; // 最新seen的时间

            // 统计信息
            std::unique_ptr<std::atomic<uint64_t>> cmd_count;      // 命令个数
            std::unique_ptr<std::atomic<uint64_t>> bytes_received; // 已经接受的
            std::unique_ptr<std::atomic<uint64_t>> bytes_sent;     // 已经发送出去的
            std::unique_ptr<std::atomic<uint64_t>> ping_count;     // ping次数
            std::unique_ptr<std::atomic<uint64_t>> pong_count;     // pong次数

            // 版本信息
            uint64_t config_epoch;  // 配置纪元
            uint64_t current_epoch; // 当前纪元
        };

        class ClusterNodeManager
        {
        public:
            ClusterNodeManager() : m_self_id(0) {}
            ~ClusterNodeManager() = default;

            /**
             * @brief 设置本节点id
             */
            void setSelfId(uint32_t id) noexcept { m_self_id = id; }

            /**
             * @brief 获取本节点id
             */
            uint32_t getSelfId() const noexcept { return m_self_id; }

            /**
             * @brief 添加节点
             * @param node 集群节点
             */
            void addNode(const ClusterNode &node);

            /**
             * @brief 移除节点
             */
            bool removeNode(uint32_t id);

            // 获取节点
            ClusterNode *getNode(uint32_t node_id);
            const ClusterNode *getNode(uint32_t node_id) const;

            // 获取所有节点
            std::vector<ClusterNode> getAllNodes() const;
            std::vector<ClusterNode> getAliveNodes() const;
            std::vector<ClusterNode> getMasterNodes() const;
            std::vector<ClusterNode> getSlaveNodes() const;

            /**
             * @brief 更新节点状态
             * @param node_id 节点id
             * @param state 状态
             */
            void updateNodeState(uint32_t node_id, NodeState state);

            /**
             * @brief 更新节点槽位
             * @param node_id 节点id
             * @param slots 槽位
             */
            void updateNodeSlots(uint32_t node_id, const std::vector<uint16_t> &slots);

            // 获取节点数量
            size_t getNodeCount() const;
            size_t getAliveNodeCount() const;

            /**
             * @brief 生成节点ID
             */
            static uint32_t generateNodeId();

            /**
             * @brief 清理超时节点
             * @param timeout_seconds 超时时间(s),默认10s
             */
            void cleanupTimeoutNodes(int timeout_seconds = 10);

        private:
            uint32_t m_self_id;
            static std::atomic<uint32_t> m_next_node_id;
            mutable std::shared_mutex m_mutex;
            std::unordered_map<uint32_t, ClusterNode> m_nodes;
        };

    } // namesapce cluster
}