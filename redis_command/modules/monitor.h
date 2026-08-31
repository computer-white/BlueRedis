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
 * @file monitor.h
 * @brief redis srever monitor监控模块
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.26
 * @copyright Copyright (c) 2026年 blue
 */
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