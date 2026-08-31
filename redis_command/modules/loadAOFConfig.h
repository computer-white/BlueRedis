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
 * @file loadaofconfig.h
 * @brief 从Yaml文件中加载Reduis AOF配置，并提供类型和string的转化
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.8.30
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <iostream>
#include "blue/config.h"
#include "blue/config_parser.h"

namespace blue
{
    namespace redisServerAOFConfig
    {
        static std::string aof_name = "appendonly.aof"; // 作为一个锚点，让std::atomic<const char*>内部使用的值的内存指向aof_name
        enum class AOFSyncStrategy : uint8_t
        {
            ALWAYS = 0,
            EVERYSEC = 1,
            NO = 2
        };

        // 辅助转换函数
        inline AOFSyncStrategy stringToSyncStrategy(const std::string &str)
        {
            if (str == "always")
            {
                return AOFSyncStrategy::ALWAYS;
            }
            if (str == "no")
            {
                return AOFSyncStrategy::NO;
            }
            return AOFSyncStrategy::EVERYSEC; // 默认
        }

        inline std::string syncStrategyToString(AOFSyncStrategy strategy)
        {
            switch (strategy)
            {
            case AOFSyncStrategy::ALWAYS:
                return "always";
            case AOFSyncStrategy::NO:
                return "no";
            default:
                return "everysec";
            }
        }
    }

    // 从yaml文件加载出来的Redis AOF
    struct AOFConfigDefine
    {
        // AOF
        bool aof_enabled = false;                    // 是否开启aof
        size_t aof_max_buffer_size = 1024 * 1024;    // aof异步写入文件的最大缓冲区大小
        size_t aof_max_file_size = 1024;             // 每个文件最大大小
        size_t aof_max_file_number = 5;              // 保留5个aof文件
        std::string aof_filename = "appendonly.aof"; // 文件模板名
        redisServerAOFConfig::AOFSyncStrategy aof_sync =
            redisServerAOFConfig::AOFSyncStrategy::EVERYSEC; // 保存策略,always(0), everysec(1), no(2)

        bool operator==(const AOFConfigDefine &rhs) const
        {
            return aof_enabled == rhs.aof_enabled &&
                   aof_filename == rhs.aof_filename &&
                   aof_max_file_number == rhs.aof_max_file_number &&
                   aof_max_file_size == rhs.aof_max_file_size &&
                   aof_sync == rhs.aof_sync;
        }

        friend std::ostream &operator<<(std::ostream &os, const AOFConfigDefine &lhs)
        {
            os << "aof_enabled: " << lhs.aof_enabled
               << "aof_filename: " << lhs.aof_filename << "\n"
               << "aof_max_file_size: " << lhs.aof_max_file_size << "\n"
               << "aof_max_file_number: " << lhs.aof_max_file_number << "\n"
               << "aof_sync: " << redisServerAOFConfig::syncStrategyToString(lhs.aof_sync);
            return os;
        }
    };

    // 特化string -> AOFConfigDefine
    template <>
    class LexicalCast<std::string, AOFConfigDefine>
    {
    public:
        AOFConfigDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            AOFConfigDefine res;
            if (!node["aof_enabled"].IsDefined())
            {
                std::cerr << "AOF configuration error, aof_enabled is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: false"
                          << std::endl;
            }
            else
            {
                res.aof_enabled = node["aof_enabled"].as<bool>();
            }

            if (!node["aof_filename"].IsDefined())
            {
                std::cerr << "AOF configuration error, aof_filename is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: appendonly.aof"
                          << std::endl;
            }
            else
            {
                res.aof_filename = node["aof_filename"].as<std::string>();
            }

            if (!node["aof_max_buffer_size"].IsDefined())
            {
                std::cerr << "AOF configuration error, aof_max_buffer_size is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default AOFConfiguration\n"
                          << AOFConfigDefine()
                          << std::endl;
                return AOFConfigDefine();
            }
            std::string tem_buffer_size = node["aof_max_buffer_size"].as<std::string>();
            auto buffer_size = util::ConfigParser::ParseSize(tem_buffer_size);
            if (buffer_size.has_value())
            {
                res.aof_max_buffer_size = *buffer_size;
            }
            else
            {
                std::cerr << "AOF configuration error, aof_max_buffer_size is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default aof_max_buffer_size: "
                          << util::ConfigParser::FormatSize(res.aof_max_file_size)
                          << std::endl;
            }

            if (!node["aof_max_file_size"].IsDefined())
            {
                std::cerr << "AOF configuration error, aof_max_file_size is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default AOFConfiguration\n"
                          << AOFConfigDefine()
                          << std::endl;
                return AOFConfigDefine();
            }
            std::string tmp_max_file_size = node["aof_max_file_size"].as<std::string>();
            auto max_file_size = util::ConfigParser::ParseSize(tmp_max_file_size);
            if (max_file_size.has_value())
            {
                res.aof_max_file_size = *max_file_size;
            }
            else
            {
                std::cerr << "AOF configuration error, aof_max_file_size is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default aof_max_file_size: "
                          << util::ConfigParser::FormatSize(res.aof_max_file_size)
                          << std::endl;
            }

            if (!node["aof_max_file_number"].IsDefined())
            {
                std::cerr << "AOF configuration error, aof_max_file_number is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default AOFConfiguration\n"
                          << AOFConfigDefine()
                          << std::endl;
                return AOFConfigDefine();
            }
            res.aof_max_file_number = node["aof_max_file_number"].as<size_t>();

            if (!node["aof_sync"].IsDefined())
            {
                std::cerr << "AOF configuration error, aof_sync is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default aof_sync: "
                          << redisServerAOFConfig::syncStrategyToString(res.aof_sync)
                          << std::endl;
            }
            else
            {
                std::string tem_sync = node["aof_sync"].as<std::string>();
                res.aof_sync = redisServerAOFConfig::stringToSyncStrategy(tem_sync);
            }
            return res;
        }
    };

    // 特化AOFConfigDefine -> string
    template <>
    class LexicalCast<AOFConfigDefine, std::string>
    {
    public:
        std::string operator()(const AOFConfigDefine &val)
        {
            YAML::Node node;
            node["aof_enabled"] = val.aof_enabled;
            node["aof_filename"] = val.aof_filename;
            node["aof_max_buffer_size"] = util::ConfigParser::FormatSize(val.aof_max_buffer_size);
            node["aof_max_file_size"] = util::ConfigParser::FormatSize(val.aof_max_file_size);
            node["aof_max_file_number"] = val.aof_max_file_number;
            node["aof_sync"] = redisServerAOFConfig::syncStrategyToString(val.aof_sync);
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}