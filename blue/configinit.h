#ifndef BLUE_CONFIGINIT_H
#define BLUE_CONFIGINIT_H
#include "dbmanager.h"
#include "redismanager.h"
#include <string>

// 数据库和redis配置
namespace blue
{
    namespace http
    {
        extern std::string s_db_host;                              // 数据库主机名
        extern std::string s_db_user;                              // 数据库user
        extern std::string s_db_database;                          // database
        extern std::string s_db_password;                          // 密码
        extern uint16_t s_db_port;                               // 端口
        extern blue::DbManager::DbManagerPtr s_dbmanager_ptr; // 数据库管理智能指针

        extern std::string s_redis_host;                                    // redis 主机
        extern uint16_t s_redis_port;                                     // redis 端口
        extern std::string s_redis_password;                                // redis 密码
        extern blue::RedisManager::RedisManagerPtr s_redismanager_ptr; // redis管理智能指针

        extern uint64_t s_rate_limit;        // redis限流数量
        extern uint64_t s_rate_limit_expire; // redis限流窗口大小
        extern uint64_t s_cache_expire;      // cache过期时间

        extern uint64_t s_select_timeout; // select 轮询超时时长

        extern uint32_t s_httpconnpool_mxsize; // httpconnnetion pool 连接池最大大小
        extern size_t s_mysqlpool_mxsize;      // mysql 连接池最大大小
        void IniteConfig();                 // 初始化函数,使用代理时必须先初始化mysql和redis
    }
}

#endif