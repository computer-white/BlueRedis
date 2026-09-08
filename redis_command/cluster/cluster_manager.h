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
 * @file cluster_manager.h
 * @brief 负责集群节点的管理，初始化管理器时根据用户需求启动若干个服务器节点，并启动后台协程来负责从节点连接上主节点的逻辑
 * @author blue
 */
#pragma once
#include "blue/task.h"
#include "blue/address.h"
#include "blue/resp_parser.h"
#include "redis_command/command_handler.h" // Redis 服务器

namespace blue
{
    namespace cluster
    {
        template <typename T>
        class ClusterManager
        {
        public:
            using ClusterManagerPtr = std::shared_ptr<ClusterManager>;

        public:
            ClusterManager() = default;
            ClusterManager(const ClusterManager &) = delete;
            ClusterManager& operator=(const ClusterManager &) = delete;

            /**
             * @brief 节点bind地址,成功后启动节点
             * @param slave_hosts slaves主机(IP + Port)
             */
            Task<bool> bindSlave(std::vector<std::string> slave_hosts);

            /**
             * @brief bind master节点
             */
            Task<bool> bindMaster(std::string master_addr, uint16_t master_port);

            /**
             * @brief 加入节点进入集群
             */
            Task<bool> addNodeToCluster(std::string ip, uint16_t port);

            /**
             * @brief 加入节点进入集群
             */
            Task<bool> addNodeToCluster(blue::Address::AddressPtr addr);

            /**
             * @brief 停止集群
             */
            Task<bool> stop();

        private:

            /**
             * @brief 生成主从复制RESP格式消息，发给每个从节点执行
             */
            std::string buildReplicaofCmd() const noexcept;

            /**
             * @brief 建立主从复制(从节点执行replicaof 主节点地址 主节点端口命令)
             */
            Task<bool> buildReplication(blue::Address::AddressPtr addr, std::string cmd);

        private:
            static size_t m_node_id;
            size_t m_master_id = 1;
            blue::Address::AddressPtr m_master_addr;
            std::shared_ptr<blue::CommandHandler<int>> m_master_node;
            std::shared_mutex m_mutex;
            std::vector<std::pair<size_t, std::shared_ptr<blue::CommandHandler<int>>>> m_slaves_nodes;
        };

        template <typename T>
        size_t ClusterManager<T>::m_node_id = 2;

        template <typename T>
        Task<bool> ClusterManager<T>::bindMaster(std::string master_addr, uint16_t master_port)
        {
            std::string master_host = master_addr + ":" + std::to_string(master_port);
            auto addr = blue::Address::LookupAnyIpAddress(master_host);
            if (!addr)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "master address is empty, host: " << master_host;
                co_return false;
            }
            auto comm = std::make_shared<blue::CommandHandler<int>>();
            while (!comm->bind(addr))
            {
                co_await sleepFor(2);
            }
            bool ans = co_await comm->start();
            if (!ans)
            {
                co_return false;
            }
            BLUE_LOG_INFO(xx::g_logger) << "Master Node started successfully on " << master_host;
            m_master_addr = addr;
            m_master_node = comm;
            co_return true;
        }

        template <typename T>
        Task<bool> ClusterManager<T>::bindSlave(std::vector<std::string> slave_hosts)
        {
            size_t count = 0;
            for (auto &host : slave_hosts)
            {
                auto addr = blue::Address::LookupAnyIpAddress(host);
                if (!addr)
                {
                    BLUE_LOG_ERROR(xx::g_logger) << "slave address is empty, host: " << host;
                    continue;
                }
                bool tmp = co_await addNodeToCluster(addr);
                if (tmp)
                {
                    count++;
                }
            }
            co_return (count != 0);
        }

        template <typename T>
        Task<bool> ClusterManager<T>::stop()
        {
            if (m_master_node)
            {
                m_master_node->ShutDownServer();
            }

            for (auto [_, node] : m_slaves_nodes)
            {
                if (node)
                {
                    node->ShutDownServer();
                }
            }
            co_return true;
        }

        template <typename T>
        Task<bool> ClusterManager<T>::buildReplication(blue::Address::AddressPtr addr, std::string cmd)
        {
            BLUE_LOG_INFO(xx::g_logger) << "buildReplication, cmd: " << cmd;
            auto sock = MSocket::CreateTcp(addr);
            if (!sock)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Slave Node Sock is invalid";
                co_return false;
            }
            BLUE_LOG_INFO(xx::g_logger) << "sock is successful";
            bool ans = co_await sock->connect(addr);
            if (!ans)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Slave Node Sock can't connect to " << addr->toString();
                sock->close();
                co_return false;
            }
            BLUE_LOG_INFO(xx::g_logger) << "sock connect to " << addr->toString();
            sock->setNoBlocking();
            ssize_t ret = co_await sock->send(cmd.data(), cmd.size(), MSG_NOSIGNAL);
            if (ret <= 0)
            {
                BLUE_LOG_INFO(xx::g_logger) << "sock send failed";
                sock->close();
                co_return false;
            }
            BLUE_LOG_INFO(xx::g_logger) << "replicaof cmd send successful! ret: " << ret;
            // sock->close();
            // +OK\r\n
            char buf[10]; // 两个+OK\r\n
            ret = co_await sock->recv(buf, sizeof(buf), MSG_NOSIGNAL);
            if (ret <= 0)
            {
                sock->close();
                BLUE_LOG_INFO(xx::g_logger) << "Slave Node " << addr->toString() << " close this connection! ";
            }
            else
            {
                std::string data(buf, ret);
                if (data == "+OK\r\n+OK\r\n")
                {
                    BLUE_LOG_INFO(xx::g_logger) << "recv AUTH and REPLICAOF response";
                }
            }
            sock->close();

            co_return true;
        }

        template <typename T>
        std::string ClusterManager<T>::buildReplicaofCmd() const noexcept
        {
            auto m_addr = std::dynamic_pointer_cast<blue::IPAddress>(m_master_addr);
            // 构建AUTH消息
            std::vector<RespValue> auth_args;
            auth_args.push_back(*RespValue::bulk_string("AUTH"));
            auth_args.push_back(*RespValue::bulk_string("client123"));
            std::string auth_cmd = RespValue::encode(*RespValue::array(std::move(auth_args)));

            // 构建REPLICAOF消息
            std::vector<RespValue> repl_args;
            repl_args.push_back(*RespValue::bulk_string("REPLICAOF"));
            repl_args.push_back(*RespValue::bulk_string(m_addr->getIp()));
            repl_args.push_back(*RespValue::bulk_string(std::to_string(m_addr->getPort())));
            std::string repl_cmd = RespValue::encode(*RespValue::array(std::move(repl_args)));

            std::string cmd = auth_cmd + repl_cmd;
            return cmd;
        }

        template <typename T>
        Task<bool> ClusterManager<T>::addNodeToCluster(std::string ip, uint16_t port)
        {
            std::string host = ip + std::to_string(port);
            auto addr = blue::Address::LookupAnyIpAddress(host);
            if (!addr)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "slave address is empty, host: " << host;
                co_return false;
            }
            co_return addNodeToCluster(addr);
        }

        template <typename T>
        Task<bool> ClusterManager<T>::addNodeToCluster(blue::Address::AddressPtr addr)
        {
            auto comm = std::make_shared<blue::CommandHandler<int>>();
            while (!comm->bind(addr))
            {
                co_await sleepFor(2);
            }
            bool ans = co_await comm->start();
            if (!ans)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Slave Node can't start " << addr->toString();
                co_return false;
            }
            BLUE_LOG_INFO(xx::g_logger) << "Slave Node started successfully on " << addr->toString();
            std::string repl_cmd = buildReplicaofCmd();
            BLUE_LOG_INFO(xx::g_logger) << "builfReplicaofCmd: " << repl_cmd << "will exec buildReplication";
            ans = co_await buildReplication(addr, std::move(repl_cmd));
            if (!ans)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Slave Node can't build Replication " << addr->toString();
                co_return false;
            }
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_slaves_nodes.emplace_back(m_node_id++, comm);
            co_return true;
        }

    } // namespace cluster
} // namespace blue