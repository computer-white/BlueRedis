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
#include "http/loadHttpParserConfig.h"
#include "http/loadHttpDbConfig.h"
#include "http/loadHttpRedisConfig.h"

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
            g_logRotateDefine_config_ptr = blue::Config::Lookup<LogRotateDefine>(
                "logrotate", LogRotateDefine(), "logrotate configuration");

        static blue::ConfigVar<std::set<LogDefine>>::ConfigVarPtr
            g_logDefine_config_ptr = blue::Config::Lookup<std::set<LogDefine>>(
                "logs", std::set<LogDefine>(), "logs LogDefine");
    }

    // redis server AOF config
    namespace RedisServerConfig
    {
        // redis-cli admin
        static blue::ConfigVar<std::string>::ConfigVarPtr
            g_admin_password = blue::Config::Lookup<std::string>(
                "redis.admin.password", "admin123", "admin password");
        static blue::ConfigVar<RedisServerConfigDefine>::ConfigVarPtr
            g_RedisServerConfigDefine_config_ptr =
                blue::Config::Lookup<RedisServerConfigDefine>(
                    "redis", RedisServerConfigDefine(), "redis server configurations");

        static blue::ConfigVar<AOFConfigDefine>::ConfigVarPtr
            g_AOFDefine_config_ptr = blue::Config::Lookup<AOFConfigDefine>(
                "redis.aof", AOFConfigDefine(), "redis AOF configurations");

        static blue::ConfigVar<SlowLogConfigDefine>::ConfigVarPtr
            g_SlowLogDefine_config_ptr = blue::Config::Lookup<SlowLogConfigDefine>(
                "redis.slowlog", SlowLogConfigDefine(), "redis SlowLog search configurations");

        static blue::ConfigVar<ReplicationConfigDefine>::ConfigVarPtr
            g_ReplicationDefine_config_ptr = blue::Config::Lookup<ReplicationConfigDefine>(
                "redis.replication", ReplicationConfigDefine(), "redis Replication module configurations");
    }

    namespace http
    {
        extern std::atomic<size_t> s_http_request_buffer_size;    // http request 缓冲大小
        extern std::atomic<size_t> s_http_request_max_body_size;  // http request max body size
        extern std::atomic<size_t> s_http_response_buffer_size;   // http response 缓冲大小
        extern std::atomic<size_t> s_http_response_max_body_size; // http response max body size

        // 不需要使用atomic,这些变量在程序中是只读的,跟上面redis相关配置不一样,他们还可以被用户登录进入服务器使用命令修改,所以要使用原子变量
        extern std::string s_db_host;
        extern std::string s_db_user;
        extern std::string s_db_database;
        extern std::string s_db_passward;
        extern uint16_t s_db_port;
        extern blue::DbManager::DbManagerPtr s_dbmanager_ptr;

        extern std::string s_redis_host;
        extern uint16_t s_redis_port;
        extern std::string s_redis_password;
        extern blue::RedisManager::RedisManagerPtr s_redismanager_ptr;

        extern uint64_t s_rate_limit;
        extern uint64_t s_rate_limit_expire;
        extern uint64_t s_cache_expire;

        namespace HttpParserConfig
        {
            static blue::ConfigVar<HttpParserConfigDefine>::ConfigVarPtr
                g_HttpRequestParserDefine_config_ptr = blue::Config::Lookup<HttpParserConfigDefine>(
                    "http.parser.request", HttpParserConfigDefine(), "http request Parser configurations");

            static blue::ConfigVar<HttpParserConfigDefine>::ConfigVarPtr
                g_HttpResponseParserDefine_config_ptr = blue::Config::Lookup<HttpParserConfigDefine>(
                    "http.parser.response", HttpParserConfigDefine(), "http response Parser configurations");
        };

        namespace HttpServerConfig
        {
            static blue::ConfigVar<HttpDbDefine>::ConfigVarPtr
                g_HttpDbDefine_config_ptr = blue::Config::Lookup<HttpDbDefine>(
                    "http.db", HttpDbDefine(), "http Db configurations");

            static blue::ConfigVar<HttpRedisDefine>::ConfigVarPtr
                g_HttpRedisDefine_config_ptr = blue::Config::Lookup<HttpRedisDefine>(
                    "http.redis", HttpRedisDefine(), "http Redis configurations");

            // httpconnectionpool size
            static blue::ConfigVar<uint32_t>::ConfigVarPtr
                g_httpconnpool_mxsize = blue::Config::Lookup<uint32_t>(
                    "httpconnectionpool.maxsize", 10, "http connectionpool maxsize");

            // mysqlpool size
            static blue::ConfigVar<size_t>::ConfigVarPtr
                g_mysqlpool_mxsize = blue::Config::Lookup<size_t>(
                    "httpmysqlpool.maxsize", 10, "http mysqlpool maxsize");
        };
    }
}

#endif