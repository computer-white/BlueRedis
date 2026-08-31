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
 * @file loadConfig.h
 * @brief 从Yaml文件中加载日志系统相关配置，并提供类型和string的转化
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
    // 从yaml文件加载出来的Logrotate内容的类型
    struct LogRotateDefine
    {
        std::string rotate_filename = "blue.log"; // 轮转文件名
        uint64_t rotate_file_size = 1024;         // 轮转文件大小
        uint32_t rotate_file_num = 5;             // 轮转文件数量

        LogRotateDefine(const std::string &filename = "blue.log", uint64_t file_size = 1024, uint32_t file_num = 5)
            : rotate_filename(std::move(filename)), rotate_file_size(file_size), rotate_file_num(file_num)
        {
        }

        bool operator==(const LogRotateDefine &rhs) const
        {
            return rotate_filename == rhs.rotate_filename &&
                   rotate_file_num == rhs.rotate_file_num &&
                   rotate_file_size == rhs.rotate_file_size;
        }

        friend std::ostream &operator<<(std::ostream &os, const LogRotateDefine &lhs)
        {
            os << "rotate_filename: " << lhs.rotate_filename << "\n"
               << "rotate_file_size: " << lhs.rotate_file_size << "\n"
               << "rotate_file_num: " << lhs.rotate_file_num;
            return os;
        }
    };

    // 从string -> LogRotateDefine
    template <>
    class LexicalCast<std::string, LogRotateDefine>
    {
    public:
        LogRotateDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            LogRotateDefine res;
            if (!node["rotate_filename"].IsDefined())
            {
                std::cerr << "LogRotateDefine.rotate_filename is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default filename: "
                          << res.rotate_filename
                          << std::endl;
            }
            else
            {
                res.rotate_filename = node["rotate_filename"].as<std::string>();
            }
            if (!node["rotate_file_num"].IsDefined())
            {
                std::cerr << "log config is error, LogRotateDefine.rotate_file_num is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default LogRotateDefine value, "
                          << "this yaml configuration file will not be used"
                          << LogRotateDefine()
                          << std::endl;
                return LogRotateDefine();
            }
            res.rotate_file_num = node["rotate_file_num"].as<uint32_t>();
            if (!node["rotate_file_size"].IsDefined())
            {
                std::cerr << "log config is error, LogRotateDefine.rotate_file_size is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default LogRotateDefine value, "
                          << "this yaml configuration file will not be used"
                          << LogRotateDefine()
                          << std::endl;
                return LogRotateDefine();
            }
            std::string tem_val = node["rotate_file_size"].as<std::string>();
            auto size_val = blue::util::ConfigParser::ParseSize(tem_val);
            if (size_val.has_value())
            {
                res.rotate_file_size = *size_val;
            }
            else
            {
                std::cerr << "log config is error, LogRotateDefine.rotate_file_size is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default rotate_file_size: 1K"
                          << std::endl;
                res.rotate_file_size = 1024;
            }
            return res;
        }
    };

    // 从LogRotateDefine -> string
    template <>
    class LexicalCast<LogRotateDefine, std::string>
    {
    public:
        std::string operator()(const LogRotateDefine &val)
        {
            YAML::Node node;
            node["rotate_filename"] = val.rotate_filename;
            node["rotate_file_num"] = val.rotate_file_num;
            node["rotate_file_size"] = util::ConfigParser::FormatSize(val.rotate_file_size);

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 从yaml文件加载出来的LogAppender内容类型
    struct LogAppenderDefine
    {
        int type = 0;                            // type = 1(file),type = 2(std)
        blue::Level level = blue::Level::NOKNOW; // lever
        std::string formatter;                   // formatter
        std::string file;                        // 文件名
        std::string name;                        // 输出目的地名称
        LogAppenderDefine() = default;

        // 重载比较运算符
        bool operator==(const LogAppenderDefine &rhs) const
        {
            return type == rhs.type &&
                   level == rhs.level &&
                   formatter == rhs.formatter &&
                   file == rhs.file &&
                   name == rhs.name;
        }
    };

    // 从yaml文件加载出来的Log内容类型
    struct LogDefine
    {
        std::string name;                         // log的name
        blue::Level level = blue::Level::NOKNOW;  // lever
        std::string formatter;                    // formatter
        std::vector<LogAppenderDefine> appenders; // appenders
        LogDefine() = default;

        // 重载比较运算符
        bool operator==(const LogDefine &rhs) const
        {
            return name == rhs.name &&
                   level == rhs.level &&
                   formatter == rhs.formatter &&
                   appenders == rhs.appenders;
        }

        // 重载比较运算符
        bool operator<(const LogDefine &rhs) const
        {
            return name < rhs.name;
        }
    };

    // 特化string -> LogAppenderDefine
    template <>
    class LexicalCast<std::string, LogAppenderDefine>
    {
    public:
        LogAppenderDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            LogAppenderDefine p;
            if (!node["name"].IsDefined())
            {
                std::cerr << "log config error LogAppenderDefine.name is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default LogAppenderDefine value, "
                          << "this yaml configuration file will not be used"
                          << std::endl;
                return LogAppenderDefine();
            }

            p.name = node["name"].as<std::string>();

            if (!node["type"].IsDefined())
            {
                std::cerr << "log config error LogAppenderDefine.type is null"
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default LogAppenderDefine value, "
                          << "this yaml configuration file will not be used"
                          << std::endl;
                return LogAppenderDefine();
            }

            std::string type = node["type"].as<std::string>();
            if (type == "FileoutLogAppender")
            {
                p.type = 1;
                if (!node["file"].IsDefined())
                {
                    std::cerr << "log config error LogAppenderDefine.file is null "
                              << __FILE__ << " " << __LINE__ << "\nnode:\n"
                              << node << "\n"
                              << "I will use default LogAppenderDefine value, "
                              << "this yaml configuration file will not be used"
                              << std::endl;
                    return LogAppenderDefine();
                }
                p.file = node["file"].as<std::string>();
            }
            else if (type == "StdoutLogAppender")
            {
                p.type = 2;
            }
            else
            {
                std::cerr << "log config error LogAppenderDefine.type is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default LogAppenderDefine value, "
                          << "this yaml configuration file will not be used"
                          << std::endl;
                return LogAppenderDefine();
            }

            p.level = EnumTraits<blue::Level>::Getstringlevel(
                node["level"].IsDefined() ? node["level"].as<std::string>() : std::string());

            p.formatter = node["formatter"].IsDefined() ? node["formatter"].as<std::string>() : std::string();

            return p;
        }
    };

    // 特化LogAppenderDefine -> string
    template <>
    class LexicalCast<LogAppenderDefine, std::string>
    {
    public:
        std::string operator()(const LogAppenderDefine &val)
        {
            YAML::Node node;
            node["name"] = val.name;

            if (val.type == 1) // file
            {
                node["type"] = "FileoutLogAppender";

                node["file"] = val.file;
            }
            else if (val.type == 2) // std
            {
                node["type"] = "StdoutLogAppender";
            }

            node["level"] = EnumTraits<blue::Level>::Getlevelstring(val.level);

            if (!val.formatter.empty())
            {
                node["formatter"] = val.formatter;
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化string -> LogDefine
    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            LogDefine p;

            if (!node["name"].IsDefined())
            {
                std::cerr << " log error val.LogDefine.name is null "
                          << __FILE__ << " " << __LINE__ << "\nnode\n"
                          << node << "\n"
                          << "I will use default LogDefine value, "
                          << "this yaml configuration file will not be used"
                          << std::endl;
                return LogDefine();
            }

            p.name = node["name"].as<std::string>();

            p.level = EnumTraits<blue::Level>::Getstringlevel(
                node["level"].IsDefined() ? node["level"].as<std::string>() : std::string());

            if (node["formatter"].IsDefined())
            {
                p.formatter = node["formatter"].as<std::string>();
            }

            // 从 YAML 节点中读取 appenders
            if (node["appenders"].IsDefined() && node["appenders"].IsSequence())
            {
                for (const auto &appender_node : node["appenders"])
                {
                    LogAppenderDefine appender;
                    appender.name = appender_node["name"].as<std::string>();

                    if (!appender_node["type"].IsDefined())
                    {
                        std::cerr << "log config error val.LogAppenderDefine.type is null "
                                  << __FILE__ << " " << __LINE__ << "\nnode:\n"
                                  << appender_node << "\n"
                                  << std::endl;
                        continue;
                    }

                    std::string type = appender_node["type"].as<std::string>();
                    if (type == "FileoutLogAppender")
                    {
                        appender.type = 1;
                        if (!appender_node["file"].IsDefined())
                        {
                            std::cerr << "log config error val.LogAppenderDefine.file is null "
                                      << __FILE__ << " " << __LINE__ << "\nnode:\n"
                                      << appender_node << "\n"
                                      << std::endl;
                            continue;
                        }
                        appender.file = appender_node["file"].as<std::string>();
                    }
                    else if (type == "StdoutLogAppender")
                    {
                        appender.type = 2;
                    }
                    else
                    {
                        std::cerr << "log config error val.LogAppenderDefine.type is invalid "
                                  << __FILE__ << " " << __LINE__ << "\nnode:\n"
                                  << appender_node << "\n"
                                  << std::endl;
                        continue;
                    }

                    appender.level = EnumTraits<blue::Level>::Getstringlevel(
                        appender_node["level"].IsDefined() ? appender_node["level"].as<std::string>() : std::string());

                    appender.formatter =
                        appender_node["formatter"].IsDefined() ? appender_node["formatter"].as<std::string>() : std::string();

                    p.appenders.push_back(appender);
                }
            }
            return p;
        }
    };

    // 特化LogDefine -> string
    template <>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &val)
        {
            YAML::Node node;
            node["name"] = val.name;

            node["level"] = EnumTraits<blue::Level>::Getlevelstring(val.level);

            if (!val.formatter.empty())
            {
                node["formatter"] = val.formatter;
            }

            // 序列化 val.appenders
            if (!val.appenders.empty())
            {
                // 用于序列化val.appenders
                YAML::Node appenders_node = YAML::Node(YAML::NodeType::Sequence);
                for (const auto &appender : val.appenders)
                {
                    YAML::Node appender_node; // appenders_node中的节点,也就是序列化LogAppenderDefine的结果

                    appender_node["name"] = appender.name;

                    if (appender.type == 1)
                    {
                        appender_node["type"] = "FileoutLogAppender";

                        appender_node["file"] = appender.file;
                    }
                    else if (appender.type == 2)
                    {
                        appender_node["type"] = "StdoutLogAppender";
                    }

                    appender_node["level"] = EnumTraits<blue::Level>::Getlevelstring(appender.level);

                    if (!appender.formatter.empty())
                    {
                        appender_node["formatter"] = appender.formatter;
                    }

                    appenders_node.push_back(appender_node);
                }
                node["appenders"] = appenders_node;
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}