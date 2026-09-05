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
 * @file loadRedisSlowLogConfig.h
 * @brief 从Yaml文件中加载Reduis SlowLog配置，并提供类型和string的转化
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
    // 从yaml文件中加载出来的SlowLogConfig
    struct SlowLogConfigDefine
    {
        int64_t slow_log_slower_than = 10'000; // 慢查询阈值
        size_t slow_log_max_len = 128;         // 慢查询日志缓存的最大条数

        bool operator==(const SlowLogConfigDefine &lhs) const
        {
            return slow_log_slower_than == lhs.slow_log_slower_than &&
                   slow_log_max_len == lhs.slow_log_max_len;
        }
    };

    // 特化 string -> SlowLogConfigDefine
    template <>
    class LexicalCast<std::string, SlowLogConfigDefine>
    {
    public:
        SlowLogConfigDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            SlowLogConfigDefine res;

            if (!node["slow_log_slower_than"].IsDefined())
            {
                std::cerr << "SlowLog configuration error, slow_log_slower_than is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << util::ConfigParser::FormatTime(std::chrono::microseconds(res.slow_log_slower_than))
                          << std::endl;
            }
            else
            {
                std::string tmp_slower_than = node["slow_log_slower_than"].as<std::string>();
                auto slower_than = util::ConfigParser::ParseTime(tmp_slower_than);
                if (slower_than.has_value())
                {
                    res.slow_log_slower_than = (*slower_than).count();
                }
                else
                {
                    std::cerr << "SlowLog configuration error, slow_log_slower_than is invalid "
                              << __FILE__ << " " << __LINE__ << "\nnode:\n"
                              << node << "\n"
                              << "I will use default value: "
                              << util::ConfigParser::FormatTime(std::chrono::microseconds(res.slow_log_slower_than))
                              << std::endl;
                }
            }

            if (!node["slow_log_max_len"].IsDefined())
            {
                std::cerr << "SlowLog configuration error, slow_log_max_len is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.slow_log_max_len
                          << std::endl;
            }
            else
            {
                res.slow_log_max_len = node["slow_log_max_len"].as<size_t>();
            }
            return res;
        }
    };

    // 特化 SlowLogConfigDefine -> string
    template <>
    class LexicalCast<SlowLogConfigDefine, std::string>
    {
    public:
        std::string operator()(const SlowLogConfigDefine &lhs)
        {
            YAML::Node node;
            node["slow_log_slower_than"] = util::ConfigParser::FormatTime(std::chrono::microseconds(lhs.slow_log_slower_than));
            node["slow_log_max_len"] = lhs.slow_log_max_len;

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}