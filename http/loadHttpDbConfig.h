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
 * @file loadHttpDbConfig.h
 * @brief 加载http中用到的数据库信息
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
    struct HttpDbDefine
    {
        std::string db_host = "127.0.0.1";
        std::string db_user = "blue";
        std::string db_database = "blue_proxy";
        std::string db_password = "";
        uint16_t db_port = 3306;

        bool operator==(const HttpDbDefine &lhs) const
        {
            return db_host == lhs.db_host &&
                   db_user == lhs.db_user &&
                   db_password == lhs.db_password &&
                   db_database == lhs.db_database &&
                   db_port == lhs.db_port;
        }
    };

    // 特化 string -> HttpDbDefine
    template <>
    class LexicalCast<std::string, HttpDbDefine>
    {
    public:
        HttpDbDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            HttpDbDefine res;

            if (!node["db_host"].IsDefined())
            {
                std::cerr << "Http Db configuration error, db_host is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << res.db_host
                        << std::endl;
            }
            else
            {
                res.db_host = node["db_host"].as<std::string>();
            }

            if (!node["db_port"].IsDefined())
            {
                std::cerr << "Http Db configuration error, db_port is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << res.db_port
                        << std::endl;
            }
            else
            {
                res.db_port = node["db_port"].as<uint16_t>();
            }

            if (!node["db_database"].IsDefined())
            {
                std::cerr << "Http Db configuration error, db_database is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << res.db_database
                        << std::endl;
            }
            else
            {
                res.db_database = node["db_database"].as<std::string>();
            }

            if (!node["db_user"].IsDefined())
            {
                std::cerr << "Http Db configuration error, db_user is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << "I will use default value: "
                        << res.db_user
                        << std::endl;
            }
            else
            {
                res.db_user = node["db_user"].as<std::string>();
            }

            if (!node["db_password"].IsDefined())
            {
                std::cerr << "Http Db configuration error, db_password is null "
                        << __FILE__ << " " << __LINE__ << "\nnode:\n"
                        << node << "\n"
                        << std::endl;
            }
            else
            {
                res.db_password = node["db_password"].as<std::string>();
            }
            return res;
        }
    };

    // 特化 HttpDbDefine -> string
    template <>
    class LexicalCast<HttpDbDefine, std::string>
    {
    public:
        std::string operator()(const HttpDbDefine &val)
        {
            YAML::Node node;
            node["db_host"] = val.db_host;
            node["db_port"] = val.db_port;
            node["db_password"] = val.db_password;
            node["db_database"] = val.db_database;
            node["db_user"] = val.db_user;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}