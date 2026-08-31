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
 * @file configinit.h
 * @brief 一些数据库和redis配置信息
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.10
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_CONFIGINIT_H
#define BLUE_CONFIGINIT_H
#include <string>
#include "blue/dbmanager.h"
#include "blue/redismanager.h"
#include "blue/loadLogConfig.h"
#include "redis_command/modules/loadAOFConfig.h"
#include "redis_command/modules/loadSlowLogConfig.h"

// 数据库和redis配置
namespace blue
{
    // log system config
    namespace logSystemConfig
    {
        static blue::ConfigVar<LogRotateDefine>::ConfigVarPtr
            g_logRotateDefine_config_ptr = blue::Config::Lookup<LogRotateDefine>("logrotate",
                                                                                 LogRotateDefine(),
                                                                                 "logrotate configuration");

        static blue::ConfigVar<std::set<LogDefine>>::ConfigVarPtr
            g_logDefine_config_ptr = blue::Config::Lookup<std::set<LogDefine>>("logs",
                                                                               std::set<LogDefine>(),
                                                                               "logs LogDefine");
    }

    // redis server AOF config
    namespace redisServerAOFConfig
    {

        static blue::ConfigVar<AOFConfigDefine>::ConfigVarPtr
            g_AOFDefine_config_ptr = blue::Config::Lookup<AOFConfigDefine>("redis.aof",
                                                                           AOFConfigDefine(),
                                                                           "redis AOF configurations");
        
        static blue::ConfigVar<SlowLogConfigDefine>::ConfigVarPtr
            g_SlowLogDefine_config_ptr = blue::Config::Lookup<SlowLogConfigDefine>("redis.slowlog",
                                                                                    SlowLogConfigDefine(),
                                                                                    "redis SlowLog search configurations");
    }

    // 这里是对于http模块中使用到的数据库和redis的配置
    // 相关数据库配置
    static blue::ConfigVar<std::string>::ConfigVarPtr g_db_host =
        blue::Config::Lookup<std::string>("db.host", "localhost", "db host");

    static blue::ConfigVar<std::string>::ConfigVarPtr g_db_user =
        blue::Config::Lookup<std::string>("db.user", "blue", "db user");

    static blue::ConfigVar<std::string>::ConfigVarPtr g_db_password =
        blue::Config::Lookup<std::string>("db.password", "", "db password");

    static blue::ConfigVar<std::string>::ConfigVarPtr g_db_database =
        blue::Config::Lookup<std::string>("db.database", "blue_proxy", "db database");

    static blue::ConfigVar<uint16_t>::ConfigVarPtr g_db_port =
        blue::Config::Lookup<uint16_t>("db.port", 3306, "db port");

    // 相关redis配置
    static blue::ConfigVar<std::string>::ConfigVarPtr g_redis_host =
        blue::Config::Lookup<std::string>("redis.host", "127.0.0.1", "redis host");

    static blue::ConfigVar<uint16_t>::ConfigVarPtr g_redis_port =
        blue::Config::Lookup<uint16_t>("redis.port", 6379, "redis port");

    static blue::ConfigVar<std::string>::ConfigVarPtr g_redis_password =
        blue::Config::Lookup<std::string>("redis.password", "", "redis password");

    static blue::ConfigVar<uint64_t>::ConfigVarPtr g_rate_limit =
        blue::Config::Lookup<uint64_t>("redis.rate_limit", 100, "redis rate limit");

    static blue::ConfigVar<uint64_t>::ConfigVarPtr g_rate_limit_expire =
        blue::Config::Lookup<uint64_t>("redis.rate_limit_expire", 60, "redis rate limit expire");

    static blue::ConfigVar<uint64_t>::ConfigVarPtr g_cache_expire =
        blue::Config::Lookup<uint64_t>("redis.cache_expire", 60, "redis cache expire");

    // select超时设置(配置里面是ms)
    static blue::ConfigVar<uint64_t>::ConfigVarPtr g_select_timeout =
        blue::Config::Lookup<uint64_t>("select.timeout", 1000, "select timeout");

    // httpconnectionpool size
    static blue::ConfigVar<uint32_t>::ConfigVarPtr g_httpconnpool_mxsize =
        blue::Config::Lookup<uint32_t>("httpconnectionpool.maxsize", 10, "http connectionpool maxsize");

    // mysqlpool size
    static blue::ConfigVar<size_t>::ConfigVarPtr g_mysqlpool_mxsize =
        blue::Config::Lookup<size_t>("httpmysqlpool.maxsize", 10, "http mysqlpool maxsize");

    namespace http
    {
        void blueHttpIniteConfig(); // 初始化函数,使用代理时必须先初始化mysql和redis
    }
}

#endif