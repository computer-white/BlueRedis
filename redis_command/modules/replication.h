#pragma once
#include <atomic>
#include <thread>
#include <string>
#include <shared_mutex>
#include <mutex>
#include "blue/msocket.h"

namespace blue
{
    class ReplicationModule
    {
    public:
        /**
         * @brief 停止复制
         */
        void stopReplication();

        /**
         * @brief 开启复制
         */
        void startReplication();

        /**
         * @brief 设置is_master
         */
        void setisMaster(bool val) { m_repl_config.is_master = val; }

        /**
         * @brief 设置master_host
         */
        void setMasterHost(const std::string &val) { m_repl_config.master_host = val; }

        /**
         * @brief 设置master_port
         */
        void setMasterPort(uint16_t val) { m_repl_config.master_port = val; }

        /**
         * @brief 设置repl_offset
         */
        void setReplOffset(int64_t val) { m_repl_config.repl_offset = val; }

        /**
         * @brief 获取is_master
         */
        bool getisMaster() const noexcept { return m_repl_config.is_master; }

        /**
         * @brief 获取master_host
         */
        const std::string &getMasterHost() const noexcept { return m_repl_config.master_host; }

        /**
         * @brief 获取master_port
         */
        uint16_t getMasterPort() const noexcept { return m_repl_config.master_port; }

        /**
         * @brief 添加从节点
         */
        void addSlaves(MSocket::MSocketPtr sock);

        /**
         * @brief 从节点复制循环
         */
        Task<void> replicationLoop();

        /**
         * @brief 写命令广播给从节点
         */
        void broadcastToSlaves(const std::string& cmd);

        /**
         * @brief 生成RDB消息
         */
        void generateRDB();

    private:
        struct ReplicationConfig
        {
            bool is_master = false;      // 是否是主节点
            std::string master_host;     // 主节点地址
            uint16_t master_port = 6666; // 主节点端口
            std::string master_password; // 主节点密码
            int64_t repl_offset;         // 复制偏移量
            std::string repl_id;         // 复制id
        };
        enum RelpState
        {
            REPL_STATE_NONE = 0,    // 未开始
            REPL_STATE_CONNECTING,  // 连接中
            REPL_STATE_HANDSHAKE,   // 握手
            REPL_STATE_TRANSFER,    // 传输中
            REPL_STATE_ONLINE       // 在线
        };

    private:
        ReplicationConfig m_repl_config;

        // 从节点连接
        std::shared_ptr<MSocket> m_repl_sock;
        std::atomic<RelpState> m_repl_state{REPL_STATE_NONE};
        std::thread m_relp_thread;

        // 从节点列表
        std::shared_mutex m_slaves_mutex;
        std::vector<MSocket::MSocketWPtr> m_slaves;
    };
}