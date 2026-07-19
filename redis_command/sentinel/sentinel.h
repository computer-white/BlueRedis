#pragma once
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <functional>
#include <unordered_map>
#include "blue/task.h"
#include "blue/msocket.h"
#include "blue/resp_parser.h"

namespace blue
{
    /**
     * @brief 哨兵配置
     */
    struct SentinelConfig
    {
        std::string master_name = "mymaster";
        std::string master_host = "127.0.0.1";
        uint16_t master_port = 6666;
        int quorum = 2; // 法定票数
        int down_after_milliseconds = 30'000;
        int failover_timeout = 180'000;
        int parallel_syncs = 1; // 同时同步的节点数

        // Sentinel 自身信息
        std::string sentinel_id;
        std::vector<std::string> sentinel_peers; // 其他 Sentinel 地址
    };

    /**
     * @brief 节点信息
     */
    struct NodeInFo
    {
        std::string host;
        uint16_t port;
        std::string replid;
        uint64_t offset = 0;
        std::chrono::steady_clock::time_point last_ping;
        bool is_down;
        bool is_master;
        bool is_slave;
        std::string role; // "master"/"slave"

        // 从节点持有
        std::string master_host;
        uint16_t master_port;
        uint64_t master_offset = 0;

        // 主节点持有
        std::vector<NodeInFo> slaves;
    };

    /**
     * @brief 哨兵类
     */
    class Sentinel
    {
    public:
        using ExecuteFunc = std::function<RespValue(std::vector<RespValue>, MSocket::MSocketPtr, bool)>;

    public:
        Sentinel(const SentinelConfig &config);
        ~Sentinel();

        // 禁止拷贝
        Sentinel(const Sentinel &) = delete;
        Sentinel &operator=(const Sentinel &) = delete;

        /**
         * @brief 启动哨兵
         */
        void start();

        /**
         * @brief 停止哨兵
         */
        void stop();

        /**
         * @brief 执行 SENTINEL 命令
         */
        AutoRespValue handleCommand(std::vector<RespValue> &args, MSocket::MSocketPtr sock);

        /**
         * @brief 获取当前主节点地址
         */
        std::pair<std::string, int> getMasterAddr() const;

        /**
         * @brief 获取从节点列表
         */
        std::vector<NodeInFo> getSlaves() const;

        /**
         * @brief 手动故障转移
         */
        bool failOver();

    private:
        /**
         * @brief 监控主循环
         */
        Task<void> monitorLoop();

        /**
         * @brief 检查主节点健康
         */
        Task<void> checkMasterHealth();

        /**
         * @brief 检查从节点健康
         */
        Task<void> checkSlavesHeadlth();

        /**
         * @brief 发现从节点
         */
        Task<void> discoverSlaves();

        /**
         * @brief 启动故障转移
         */
        Task<void> startFailOver();

        /**
         * @brief 选举新的主节点
         */
        NodeInFo *electNewMaster();

        /**
         * @brief 提升从节点为主节点
         */
        Task<bool> promoteToMaster(NodeInFo slave);

        /**
         * @brief 更新其他从节点的复制目标
         */
        Task<void> updateReplicas(NodeInFo newMaster);

        /**
         * @brief 向节点发送命令
         */
        Task<std::string> sendCommand(std::string host, uint16_t port, std::string cmd);

        /**
         * @brief 向节点发送命令（带密码）
         */
        Task<std::string> sendCommandWithPassWord(std::string host, uint16_t port, 
                                                std::string cmd, std::string password = "client123");

        /**
         * @brief PING 节点
         */
        Task<bool> pingNode(std::string host, uint16_t port);

        /**
         * @brief 获取节点 INFO 信息
         */
        Task<std::string> getNodeInfo(std::string host, uint16_t port);

        /**
         * @brief 解析 INFO 响应
         */
        NodeInFo parseInfo(const std::string &info);

        /**
         * @brief 解析从节点 INFO
         */
        std::vector<NodeInFo> parseSlaves(const std::string &info);

        /**
         * @brief 获取 Sentinel 之间通信端口
         */
        int getSentinelPort() const { return m_config.master_port + 10000; }

        /**
         * @brief 生成 Sentinel ID
         */
        std::string generateSentinelId();

        /**
         * @brief 处理 SENTINEL MASTER 命令
         */
        AutoRespValue handleSentinelMaster();

        /**
         * @brief 处理 SENTINEL SLAVES 命令
         */
        AutoRespValue handleSentinelSlaves(const std::string &master_name);

        /**
         * @brief 处理 SENTINEL GET-MASTER-ADDR-BY-NAME 命令
         */
        AutoRespValue handleSentinelGetMasterAddr(const std::string &master_name);

        /**
         * @brief 处理 SENTINEL RESET 命令
         */
        AutoRespValue handleSentinelReset(const std::string &pattern);

        /**
         * @brief 处理 SENTINEL FAILOVER 命令
         */
        AutoRespValue handleSentinelFailOver(const std::string &master_name);

        /**
         * @brief 处理 SENTINEL MONITOR 命令
         */
        AutoRespValue handleSentinelMonitor(const std::string &master_name,
                                            const std::string &host, uint16_t port,
                                            int quorum);

        /**
         * @brief 处理 SENTINEL REMOVE 命令
         */
        AutoRespValue handleSentinelRemove(const std::string &master_name);

        /**
         * @brief Sentinel 间通信服务器
         */
        void startSentinelServer();

        /**
         * @brief 处理 Sentinel 间消息
         */
        void handleSentinelMessage(const std::string &msg);

    private:
        SentinelConfig m_config;
        NodeInFo m_master_info;
    
        mutable std::shared_mutex m_mutex;
        std::vector<NodeInFo> m_slaves_info;
        std::vector<std::string> m_know_sentinel;

        std::atomic<bool> m_running{false};
        std::atomic<bool> m_failover_in_progress{false};
        std::atomic<uint64_t> m_config_epoch{0};

        std::thread m_server_thread;

        // 命令执行器
        ExecuteFunc m_executor;
    };
}