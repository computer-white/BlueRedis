#pragma once
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <atomic>
#include "blue/msocket.h"

namespace blue
{
    class MonitorModule
    {
    public:
        MonitorModule() = default;
        ~MonitorModule() = default;

        MonitorModule(const MonitorModule& ) = delete;
        MonitorModule& operator=(const MonitorModule& ) = delete;
    public:
        
        /**
         * @brief 推送消息给monitor_client
         * @param cmd 命令
         * @param sock 被推送的客户端
         * @note 过期的sock会被清理
         */
        void pushToMonitor(const std::string &cmd, MSocket::MSocketPtr sock);

        /**
         * @brief 集中清理过期的monitor_clients
         */
        void removeMonitor();

        /**
         * @brief 获取大小
         */
        size_t size() const;

        /**
         * @brief 添加监控客户端
         */
        void addMonitorClient(MSocket::MSocketPtr sock);

    private:
        mutable std::shared_mutex m_monitor_mutex;           // MONITOR 锁
        std::vector<MSocket::MSocketWPtr> m_monitor_clients; // MONITOR 客户端列表
    };
}