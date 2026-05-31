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
        std::string s_db_host = "";                              // 数据库主机名
        std::string s_db_user = "";                              // 数据库user
        std::string s_db_database = "";                          // database
        std::string s_db_passward = "";                          // 密码
        uint16_t s_db_port = 3306;                               // 端口
        blue::DbManager::DbManagerPtr s_dbmanager_ptr = nullptr; // 数据库管理智能指针

        std::string s_redis_host = "";                                    // redis 主机
        uint16_t s_redis_port = 6379;                                     // redis 端口
        std::string s_redis_passward = "";                                // redis 密码
        blue::RedisManager::RedisManagerPtr s_redismanager_ptr = nullptr; // redis管理智能指针

        uint64_t s_rate_limit = 0;        // redis限流数量
        uint64_t s_rate_limit_expire = 0; // redis限流窗口大小
        uint64_t s_cache_expire = 0;      // cache过期时间

        uint64_t s_select_timeout = 0; // select 轮询超时时长

        uint32_t s_httpconnpool_mxsize = 0; // httpconnnetion pool 连接池最大大小
        size_t s_mysqlpool_mxsize = 0;      // mysql 连接池最大大小
        void IniteConfig();                 // 初始化函数,使用代理时必须先初始化mysql和redis
    }
}

#endif