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
 * @file loadHttpParserConfig.h
 * @brief 从yaml文件加载Http相关配置
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.9.7
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <iostream>
#include "blue/config.h"
#include "blue/config_parser.h"

namespace blue
{
    struct HttpParserConfigDefine
    {
        size_t buffer_size = 1024 * 1024; // 缓冲区大小
        size_t max_body_size = 5 * 1024;  // body大小

        bool operator==(const HttpParserConfigDefine &lhs) const
        {
            return buffer_size == lhs.buffer_size &&
                    max_body_size == lhs.max_body_size;
        }
    };

    // 特化 string -> HttpParserConfigDefine
    template <>
    class LexicalCast<std::string, HttpParserConfigDefine>
    {
    public:
        HttpParserConfigDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            HttpParserConfigDefine res;
            
            if (!node["buffer_size"].IsDefined())
            {
                std::cerr << "HttpParser configuration error, buffer_size is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << util::ConfigParser::FormatSize(res.buffer_size)
                        << std::endl;
            }
            else
            {
                std::string tmp_buffer_size_val = node["buffer_size"].as<std::string>();
                auto buffer_size_val = util::ConfigParser::ParseSize(tmp_buffer_size_val);
                if (buffer_size_val.has_value())
                {
                    res.buffer_size = *buffer_size_val;
                }
                else
                {
                    std::cerr << "HttpParser configuration error, buffer_size is invalid "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << util::ConfigParser::FormatSize(res.buffer_size)
                        << std::endl;
                }
            }

            if (!node["max_body_size"].IsDefined())
            {
                std::cerr << "HttpParser configuration error, max_body_size is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << util::ConfigParser::FormatSize(res.max_body_size)
                        << std::endl;
            }
            else
            {
                std::string tmp_max_body_size_val = node["max_body_size"].as<std::string>();
                auto max_body_size_val = util::ConfigParser::ParseSize(tmp_max_body_size_val);
                if (max_body_size_val.has_value())
                {
                    res.max_body_size = *max_body_size_val;
                }
                else
                {
                    std::cerr << "HttpParser configuration error, max_body_size is invalid "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << util::ConfigParser::FormatSize(res.max_body_size)
                        << std::endl;
                }
            }
            return res;
        }
    };

    // 特化 HttpParserConfigDefine -> string
    template <>
    class LexicalCast<HttpParserConfigDefine, std::string>
    {
    public:
        std::string operator()(const HttpParserConfigDefine &val)
        {
            YAML::Node node;
            node["buffer_size"] = util::ConfigParser::FormatSize(val.buffer_size);
            node["max_body_size"] = util::ConfigParser::FormatSize(val.max_body_size);
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}