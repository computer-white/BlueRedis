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
#include "redis_command/loadRedisAOFConfig.h"
#include "redis_command/loadRedisSlowLogConfig.h"
#include "redis_command/loadRedisClientConfig.h"
#include "redis_command/loadRedisReplicationConfig.h"

// 数据库和redis配置
namespace blue
{
    // aof
    extern std::atomic<RedisServerConfig::AOFSyncStrategy> s_aof_sync;     // 保存策略,always(0), everysec(1), no(2)
    extern std::atomic<std::shared_ptr<const std::string>> s_aof_filename; // 文件模板名
    extern std::atomic<size_t> s_aof_max_file_size;                        // 每个文件最大大小
    extern std::atomic<size_t> s_aof_max_buffer_size;                      // aof异步写入文件的最大缓冲区大小
    extern std::atomic<int> s_aof_max_file_number;                         // 保留aof文件数量
    extern std::atomic<bool> s_aof_enabled;                                // 是否开启aof
    // slowlog
    extern std::atomic<int64_t> s_slow_log_slower_than; // 阈值（微秒），默认10ms
    extern std::atomic<size_t> s_slow_log_max_len;      // 慢查询缓存最大保存条数
    // redis
    extern std::atomic<uint64_t> s_redis_server_timeout;    // 每个客户端与服务器最大的待机时长
    extern std::atomic<uint32_t> s_redis_server_maxClients; // 最大客户端数量

    extern std::atomic<size_t> s_max_command_size;    // Resp命令解析器缓冲区最大大小(即解析器缓冲区可以接受的最大大小)
    extern std::atomic<size_t> s_max_batch_size;      // 服务器批量响应大小阈值
    extern std::atomic<size_t> s_max_exec_batch_size; // 服务器批量执行的命令条数(即客户端单次输入的命令最大个数)

    // replication
    extern std::atomic<std::shared_ptr<const std::string>> s_repl_master_addr;     // 主节点地址
    extern std::atomic<std::shared_ptr<const std::string>> s_repl_master_password; // 主节点密码
    extern std::atomic<int64_t> s_repl_offset;                                     // 从节点主从复制进入在线模式后，从节点同步主节点的写命令数量
    extern std::atomic<int32_t> s_repl_retry_count;                                // 从节点连接主节点，尝试连接次数，超过就退出复制循环(即复制线程结束)
    extern std::atomic<uint16_t> s_repl_master_port;                               // 主节点端口
    extern std::atomic<bool> s_repl_is_master;                                     // 是否是主节点

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
    namespace RedisServerConfig
    {
        // redis-cli admin
        static blue::ConfigVar<std::string>::ConfigVarPtr
            g_admin_password = blue::Config::Lookup<std::string>("redis.admin.password",
                                                                 "admin123",
                                                                 "admin password");
        static blue::ConfigVar<RedisServerConfigDefine>::ConfigVarPtr
            g_RedisServerConfigDefine_config_ptr =
                blue::Config::Lookup<RedisServerConfigDefine>("redis",
                                                              RedisServerConfigDefine(),
                                                              "redis server configurations");

        static blue::ConfigVar<AOFConfigDefine>::ConfigVarPtr
            g_AOFDefine_config_ptr = blue::Config::Lookup<AOFConfigDefine>("redis.aof",
                                                                           AOFConfigDefine(),
                                                                           "redis AOF configurations");

        static blue::ConfigVar<SlowLogConfigDefine>::ConfigVarPtr
            g_SlowLogDefine_config_ptr = blue::Config::Lookup<SlowLogConfigDefine>("redis.slowlog",
                                                                                   SlowLogConfigDefine(),
                                                                                   "redis SlowLog search configurations");

        static blue::ConfigVar<ReplicationConfigDefine>::ConfigVarPtr
            g_ReplicationDefine_config_ptr = blue::Config::Lookup<ReplicationConfigDefine>("redis.replication",
                                                                                           ReplicationConfigDefine(),
                                                                                           "redis Replication module configurations");
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