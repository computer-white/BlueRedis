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
 * @file loadRedisReplicationConfig.h
 * @brief 从Yaml文件中加载Reduis Replication配置，并提供类型和string的转化
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.9.1
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <iostream>
#include "blue/config.h"
#include "blue/config_parser.h"

namespace blue
{
    struct ReplicationConfigDefine
    {
        bool is_master = true;                    // 是否是主节点
        std::string master_addr{"127.0.0.1"};     // 主节点地址
        uint16_t master_port = 6667;              // 主节点端口
        std::string master_password{"client123"}; // 主节点密码
        int64_t repl_offset = 0;                  // 从节点主从复制进入在线模式后，从节点同步主节点的写命令数量
        int retry_count = 5;                      // 从节点连接主节点，尝试连接次数，超过就退出复制循环(即复制线程结束)
        bool operator==(const ReplicationConfigDefine &lhs) const
        {
            return is_master == lhs.is_master &&
                   master_addr == lhs.master_addr &&
                   master_port == lhs.master_port &&
                   master_password == lhs.master_password &&
                   repl_offset == lhs.repl_offset;
        }
    };

    // 特化 string -> ReplicationConfigDefine
    template <>
    class LexicalCast<std::string, ReplicationConfigDefine>
    {
    public:
        ReplicationConfigDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            ReplicationConfigDefine res;

            if (!node["is_master"].IsDefined())
            {
                std::cerr << "Replication configuration error, is_master is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: true"
                          << std::endl;
            }
            else
            {
                std::string tmp_is_master = node["is_master"].as<std::string>();
                res.is_master = util::ConfigParser::ParseBool(tmp_is_master);
            }

            if (!node["master_addr"].IsDefined())
            {
                std::cerr << "Replication configuration error, master_addr is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.master_addr
                          << std::endl;
            }
            else
            {
                res.master_addr = node["master_addr"].as<std::string>();
            }

            if (!node["master_port"].IsDefined())
            {
                std::cerr << "Replication configuration error, master_port is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.master_port
                          << std::endl;
            }
            else
            {
                res.master_port = node["master_port"].as<uint16_t>();
            }

            if (!node["master_password"].IsDefined())
            {
                std::cerr << "Replication configuration error, master_password is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.master_password
                          << std::endl;
            }
            else
            {
                res.master_password = node["master_password"].as<std::string>();
            }

            if (!node["repl_offset"].IsDefined())
            {
                std::cerr << "Replication configuration error, repl_offset is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.repl_offset
                          << std::endl;
            }
            else
            {
                res.repl_offset = node["repl_offset"].as<int64_t>();
            }

            if (!node["retry_count"].IsDefined())
            {
                std::cerr << "Replication configuration error, retry_count is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.retry_count
                          << std::endl;
            }
            else
            {
                res.retry_count = node["retry_count"].as<int>();
            }
            return res;
        }
    };

    // 特化 ReplicationConfigDefine -> string
    template <>
    class LexicalCast<ReplicationConfigDefine, std::string>
    {
    public:
        std::string operator()(const ReplicationConfigDefine &val)
        {
            YAML::Node node;
            node["is_master"] = (val.is_master ? "true" : "false");
            node["master_addr"] = val.master_addr;
            node["master_port"] = val.master_port;
            node["master_password"] = val.master_password;
            node["repl_offset"] = val.repl_offset;
            node["retry_count"] = val.retry_count;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}
