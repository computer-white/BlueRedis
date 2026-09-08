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
 * @file loadHttpRedisConfig.h
 * @brief 加载http中用到的Redis数据库信息
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
    struct HttpRedisDefine
    {
        std::string redis_host = "127.0.0.1";          // redis 主机
        std::string redis_password = "";               // redis 密码
        uint64_t rate_limit = 100;                     // 限制访问上线次数
        uint64_t rate_limit_expire = 60 * 1000 * 1000; // 限制的窗口时间(60s)即60s内超过rate_limit的请求会被禁止
        uint64_t cache_expire = 60 * 1000 * 1000;      // 缓存过期时间(60s)
        uint16_t redis_port = 6379;                    // redis 端口

        bool operator==(const HttpRedisDefine &lhs) const
        {
            return redis_host == lhs.redis_host &&
                   redis_password == lhs.redis_password &&
                   rate_limit == lhs.rate_limit &&
                   rate_limit_expire == lhs.rate_limit_expire &&
                   cache_expire == lhs.cache_expire &&
                   redis_port == lhs.redis_port;
        }
    };

    // 特化 string -> HttpRedisDefine
    template <>
    class LexicalCast<std::string, HttpRedisDefine>
    {
    public:
        HttpRedisDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            HttpRedisDefine res;

            if (!node["redis_host"].IsDefined())
            {
                std::cerr << "Http Redis configuration error, redis_host is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.redis_host
                          << std::endl;
            }
            else
            {
                res.redis_host = node["redis_host"].as<std::string>();
            }

            if (!node["redis_port"].IsDefined())
            {
                std::cerr << "Http Redis configuration error, redis_port is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.redis_port
                          << std::endl;
            }
            else
            {
                res.redis_port = node["redis_port"].as<uint16_t>();
            }

            if (!node["redis_password"].IsDefined())
            {
                std::cerr << "Http Redis configuration error, redis_password is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << std::endl;
            }
            else
            {
                res.redis_password = node["redis_password"].as<std::string>();
            }

            if (!node["rate_limit"].IsDefined())
            {
                std::cerr << "Http Redis configuration error, rate_limit is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << res.rate_limit
                          << std::endl;
            }
            else
            {
                res.rate_limit = node["rate_limit"].as<uint64_t>();
            }

            if (!node["rate_limit_expire"].IsDefined())
            {
                std::cerr << "Http Redis configuration error, rate_limit_expire is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << util::ConfigParser::FormatTime(std::chrono::microseconds(res.rate_limit_expire))
                          << std::endl;
            }
            else
            {
                std::string tmp_rate_limit_expire_val = node["rate_limit_expire"].as<std::string>();
                auto rate_limit_expire_val = util::ConfigParser::ParseTime(tmp_rate_limit_expire_val);
                if (rate_limit_expire_val.has_value())
                {
                    res.rate_limit_expire = (*rate_limit_expire_val).count();
                }
                else
                {
                    std::cerr << "Http Redis configuration error, rate_limit_expire is invalid "
                              << __FILE__ << " " << __LINE__ << "\nnode:\n"
                              << node << "\n"
                              << "I will use default value: "
                              << util::ConfigParser::FormatTime(std::chrono::microseconds(res.rate_limit_expire))
                              << std::endl;
                }
            }

            if (!node["cache_expire"].IsDefined())
            {
                std::cerr << "Http Redis configuration error, cache_expire is null "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << util::ConfigParser::FormatTime(std::chrono::microseconds(res.cache_expire))
                          << std::endl;
            }
            else
            {
                std::string tmp_cache_expire_val = node["cache_expire"].as<std::string>();
                auto cache_expire_val = util::ConfigParser::ParseTime(tmp_cache_expire_val);
                if (cache_expire_val.has_value())
                {
                    res.cache_expire = (*cache_expire_val).count();
                }
                else
                {
                    std::cerr << "Http Redis configuration error, cache_expire is invalid "
                          << __FILE__ << " " << __LINE__ << "\nnode:\n"
                          << node << "\n"
                          << "I will use default value: "
                          << util::ConfigParser::FormatTime(std::chrono::microseconds(res.cache_expire))
                          << std::endl;
                }
            }
            return res;
        }
    };

    // 特化 HttpRedisDefine -> string
    template <>
    class LexicalCast<HttpRedisDefine, std::string>
    {
    public:
        std::string operator()(const HttpRedisDefine &val)
        {
            YAML::Node node;
            node["redis_host"] = val.redis_host;
            node["redis_port"] = val.redis_port;
            node["redis_password"] = val.redis_password;
            node["rate_limit"] = val.rate_limit;
            node["rate_limit_expire"] = util::ConfigParser::FormatTime(std::chrono::microseconds(val.rate_limit_expire));
            node["cache_expire"] = util::ConfigParser::FormatTime(std::chrono::microseconds(val.cache_expire));

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}