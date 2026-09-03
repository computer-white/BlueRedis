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
 * @file loadRedisClientConfig.h
 * @brief 从Yaml文件中加载Reduis AOF配置，并提供类型和string的转化
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

    // 从yaml文件加载出来的Redis Client Configuration
    struct RedisServerConfigDefine
    {
        size_t max_command_size = 1024 * 1024;  // Resp命令解析器缓冲区最大大小(即解析器缓冲区可以接受的最大大小)
        size_t max_batch_size = 256 * 1024;     // 服务器批量响应大小阈值
        size_t max_exec_batch_size = 256;       // 服务器批量执行的命令条数(即客户端单次输入的命令最大个数)
        uint64_t timeout = 0;       // 客户端超时
        uint32_t maxClients = 1000; // 最大客户端数量

        bool operator==(const RedisServerConfigDefine &lhs) const
        {
            return maxClients == lhs.maxClients &&
                   timeout == lhs.timeout;
        }
    };

    // 特化 string -> RedisServerConfig
    template <>
    class LexicalCast<std::string, RedisServerConfigDefine>
    {
    public:
        RedisServerConfigDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            RedisServerConfigDefine res;

            if (!node["max_command_size"].IsDefined())
            {
                std::cerr << "Redis Server configuration error, max_command_size is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.max_command_size
                          << std::endl;
            }
            else
            {
                std::string tmp_max_command_size_val = node["max_command_size"].as<std::string>();
                auto max_command_size_val = util::ConfigParser::ParseSize(tmp_max_command_size_val);
                if (max_command_size_val.has_value())
                {
                    res.max_command_size = *max_command_size_val;
                }
                else
                {
                    std::cerr << "Redis Server configuration error, max_command_size is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.max_command_size
                          << std::endl;
                }
            }

            if (!node["max_batch_size"].IsDefined())
            {
                std::cerr << "Redis Server configuration error, max_batch_size is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.max_batch_size
                          << std::endl;
            }
            else
            {
                std::string tmp_max_batch_size = node["max_batch_size"].as<std::string>();
                auto max_batch_size_val = util::ConfigParser::ParseSize(tmp_max_batch_size);
                if (max_batch_size_val.has_value())
                {
                    res.max_batch_size = *max_batch_size_val;
                }
                else
                {
                    std::cerr << "Redis Server configuration error, max_batch_size is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.max_batch_size
                          << std::endl;
                }
            }

            if (!node["max_exec_batch_size"].IsDefined())
            {
                std::cerr << "Redis Server configuration error, max_exec_batch_size is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.max_exec_batch_size
                          << std::endl;
            }
            else
            {
                res.max_exec_batch_size = node["max_exec_batch_size"].as<size_t>();
            }

            if (!node["maxClients"].IsDefined())
            {
                std::cerr << "Redis Server configuration error, maxClients is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value(Negative numbers indicate no limit): "
                          << res.maxClients
                          << std::endl;
            }
            else
            {
                res.maxClients = node["maxClients"].as<int32_t>();
            }

            if (!node["timeout"].IsDefined())
            {
                std::cerr << "Redis Server configuration error, timeout is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value(Zero means no restrictions): "
                          << res.timeout
                          << std::endl;
            }
            else
            {
                std::string tem_timeout_val = node["timeout"].as<std::string>();
                auto timeout_val = util::ConfigParser::ParseTime(tem_timeout_val);
                if (timeout_val.has_value())
                {
                    res.timeout = (*timeout_val).count();
                }
                else
                {
                    std::cerr << "Redis Server configuration error, timeout is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value(Zero means no restrictions): "
                          << res.timeout
                          << std::endl;
                }
            }
            return res;
        }
    };

    // 特化 RedisServerConfigDefine-> string
    template <>
    class LexicalCast<RedisServerConfigDefine, std::string>
    {
    public:
        std::string operator()(const RedisServerConfigDefine &lhs)
        {
            YAML::Node node;
            node["max_command_size"] = util::ConfigParser::FormatSize(lhs.max_command_size);
            node["max_batch_size"] = util::ConfigParser::FormatSize(lhs.max_batch_size);
            node["max_exec_batch_size"] = lhs.max_exec_batch_size;
            node["maxClients"] = lhs.maxClients;
            node["timeout"] = util::ConfigParser::FormatTime(std::chrono::microseconds(lhs.timeout));
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}