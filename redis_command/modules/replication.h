#pragma once
#include <atomic>
#include <thread>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <queue>
#include "blue/msocket.h"

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
        ReplicationModule(const ReplicationModule& ) = delete;
        ReplicationModule& operator=(const ReplicationModule& ) = delete;
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
         * @brief 从节点列表是否为空
         */
        bool slavesEmpty() const noexcept { std::shared_lock<std::shared_mutex> lock(m_slaves_mutex); return m_slaves.empty(); }

        /**
         * @brief 输出slaves信息
         */
        std::string slavesToString() const noexcept;

        /**
         * @brief 获取从节点个数
         */
        size_t slavesCount() const;

        /**
         * @brief 获取当前复制状态
         */
        uint8_t getReplState() const noexcept { return m_repl_state.load(std::memory_order_acquire); }

        /**
         * @brief 获取当前复制偏移量
         */
        int64_t getReplOffset() const noexcept { return m_repl_config.repl_offset; }

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
        void broadcastToSlaves(const std::string& cmd);

        /**
         * @brief 从内存加载 RDB 数据
         */
        bool loadRDBFromMemory(const std::string& data);

    private:
        struct ReplicationConfig
        {
            bool is_master = true;          // 是否是主节点
            std::string master_host;        // 主节点地址
            uint16_t master_port;           // 主节点端口
            std::string master_password;    // 主节点密码
            int64_t repl_offset;            // 复制偏移量
            std::string repl_id;            // 复制id
        };
        enum RelpState : uint8_t
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
        mutable std::shared_mutex m_slaves_mutex;
        std::vector<MSocket::MSocketWPtr> m_slaves;
    private:
        // 服务器停止
        std::atomic<bool> m_server_stop{false};

        // 复制命令队列
        struct ReplCommand
        {
            std::vector<RespValue> args;
        };

        std::mutex m_repl_queue_mutex;
        std::queue<ReplCommand> m_repl_queue;
        std::condition_variable m_repl_queue_cv;
        std::atomic<bool> m_repl_queue_stop{false};
        std::atomic<bool> m_consumer_started{false};

        // 回调
        ExecuteFunc m_executor;
        

    };
}