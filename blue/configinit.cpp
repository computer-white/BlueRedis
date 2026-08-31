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
#include "blue/configinit.h"
#include "blue/dbmanager.h"
#include "blue/redismanager.h"
#include "blue/mysqlpool.h"
#include "http/httpconnection.h"

// 数据库和redis配置
namespace blue
{
    // redis server AOF configuration
    std::atomic<bool> s_aof_enabled{false};
    std::atomic<const char *> s_aof_filename{"appendonly.aof"};
    std::atomic<size_t> s_aof_max_file_size{1024};
    std::atomic<size_t> s_aof_max_file_number{5};
    std::atomic<redisServerAOFConfig::AOFSyncStrategy> s_aof_sync{redisServerAOFConfig::AOFSyncStrategy::EVERYSEC};
    std::atomic<size_t> s_aof_max_buffer_size{1024 * 1024};

    // redis server SlowLog configuration
    std::atomic<int64_t> s_slow_log_slower_than{10'000}; // 阈值（微秒），默认10ms
    std::atomic<size_t> s_slow_log_max_len{128};         // 慢查询缓存最大保存条数

    struct InitConfig
    {
        InitConfig()
        {
            // 添加监听器,同时读取内容设置到日志系统里面
            logSystemConfig::g_logDefine_config_ptr->addListener([](const std::set<LogDefine> &old_val,
                                                                    const std::set<LogDefine> &new_val)
                                                                 {
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) 
            << " on_change_cb conf changed! ";
            // 新增
            for (auto& n_val : new_val)
            {
                auto it = old_val.find(n_val);
                blue::Logger::LoggerPtr new_logger;
                if (it == old_val.end())
                {
                    // 新增,利用名字查找,这样就会将新增的放入到\
                    Message管理的logger里面,同时设置了默认的Appender
                    new_logger = BLUE_LOG_NAME(n_val.name);
                }
                else
                {
                    if (*it == n_val)
                    {
                        continue;
                    }
                    // 修改
                    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT())  
                    << " Update logger: [" << n_val.name << "]";
                    new_logger = BLUE_LOG_NAME(n_val.name);
                }
                // 这里也可以不用判断,因为不知道文件的level是什么或者设置的是错误的,那么有必要去提醒？
                if (n_val.level != blue::Level::NOKNOW)
                {
                    new_logger->setlevel(n_val.level);
                }
                if (!n_val.formatter.empty())
                {
                    new_logger->setFormatter(n_val.formatter);
                }
                // 清除默认的new_logger的Appender
                new_logger->clearAppender();
                for (auto& a : n_val.appenders)
                {
                    blue::LogAppender::LogAppenderPtr new_appender;
                    if (a.type == 1) // file
                    {
                        new_appender.reset(new blue::FileoutLogAppender(a.file));
                    }
                    else if (a.type == 2) // std
                    {
                        new_appender.reset(new blue::StdoutLogAppender);
                    }
                    else
                    {
                        BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                        << " LogDefine.type的值未知 type : [" << a.type << "]";
                        continue;
                    }
                    if (a.level != blue::Level::NOKNOW)
                    {
                        new_appender->setLevel(a.level);
                    }
                    if (!a.formatter.empty())
                    {
                        auto a_formatter = 
                        std::make_shared<blue::LogFormatter>(a.formatter);
                        if (!a_formatter->getHasError())
                        {
                            new_appender->setformatter(a_formatter);
                        }
                        else
                        {
                            // 有formatter但是解析出来有错误，我们不添加到new_logger里面
                            std::cerr << "log config error n_val.LogAppenderDefine.fomatter is error "
                                        << __FILE__ << " " << __LINE__ << std::endl;
                            continue;
                        }
                    }
                    // 这里addAppender时如果new_appender没有自己的forrmatter,就会被设置为new_logger的formatter
                    new_logger->addAppender(new_appender);
                }
            }
            // 删除
            for (auto& o_val : old_val)
            {
                auto it = new_val.find(o_val);
                if (it == new_val.end())
                {
                    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT())  
                    << " Remove logger: [" << o_val.name << "]";
                    auto logger = BLUE_LOG_NAME(o_val.name);
                    if (logger)
                    {
                        logger->clearAppender();
                        logger->setlevel(static_cast<blue::Level>(100));
                    }
                }
            } });

            redisServerAOFConfig::g_AOFDefine_config_ptr->addListener([](const blue::AOFConfigDefine &old_val,
                                                                         const blue::AOFConfigDefine &new_val)
                                                                      {
                    std::cout << "Update AOF configuration!" << std::endl;

                    // enabled
                    s_aof_enabled.store(new_val.aof_enabled, std::memory_order_release);
                    // filename
                    redisServerAOFConfig::aof_name = new_val.aof_filename;
                    s_aof_filename.store(redisServerAOFConfig::aof_name.c_str(), std::memory_order_release);
                    // max_buffer_size
                    s_aof_max_buffer_size.store(new_val.aof_max_buffer_size, std::memory_order_release);
                    // max_file_size
                    s_aof_max_file_size.store(new_val.aof_max_file_size, std::memory_order_release);
                    // max_file_number
                    s_aof_max_file_number.store(new_val.aof_max_file_number, std::memory_order_release);
                    // sync strategy
                    s_aof_sync.store(new_val.aof_sync, std::memory_order_release);

                    std::cout << "Update AOF configuration Successful!" << std::endl; });
        }
    };

    static InitConfig blueinit;

    namespace http
    {
        std::string s_db_host = "";                              // 数据库主机名
        std::string s_db_user = "";                              // 数据库user
        std::string s_db_database = "";                          // database
        std::string s_db_password = "";                          // 密码
        uint16_t s_db_port = 3306;                               // 端口
        blue::DbManager::DbManagerPtr s_dbmanager_ptr = nullptr; // 数据库管理智能指针

        std::string s_redis_host = "";                                    // redis 主机
        uint16_t s_redis_port = 6379;                                     // redis 端口
        std::string s_redis_password = "";                                // redis 密码
        blue::RedisManager::RedisManagerPtr s_redismanager_ptr = nullptr; // redis管理智能指针

        uint64_t s_rate_limit = 0;        // redis限流数量
        uint64_t s_rate_limit_expire = 0; // redis限流窗口大小
        uint64_t s_cache_expire = 0;      // cache过期时间

        uint64_t s_select_timeout = 0; // select 轮询超时时长

        uint32_t s_httpconnpool_mxsize = 0; // httpconnnetion pool 连接池最大大小
        size_t s_mysqlpool_mxsize = 0;      // mysql 连接池最大大小

        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        void blueHttpIniteConfig()
        {
            // db
            s_db_host = g_db_host->getValue();
            s_db_user = g_db_user->getValue();
            s_db_password = g_db_password->getValue();
            s_db_database = g_db_database->getValue();
            s_db_port = g_db_port->getValue();
            s_dbmanager_ptr = blue::DbManager::Create(s_db_host, s_db_user, s_db_password, s_db_database, s_db_port);
            if (!s_dbmanager_ptr)
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to create DbManager";
            }
            else
            {
                BLUE_LOG_INFO(g_logger) << "DbManager created";
            }
            g_db_host->addListener([](const std::string &old_val, const std::string &new_val)
                                   {
                s_db_host = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_password,s_db_database,s_db_port); });
            //
            g_db_user->addListener([](const std::string &old_val, const std::string &new_val)
                                   {
                s_db_user = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_password,s_db_database,s_db_port); });
            //
            g_db_password->addListener([](const std::string &old_val, const std::string &new_val)
                                       {
                s_db_password = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_password,s_db_database,s_db_port); });
            //
            g_db_database->addListener([](const std::string &old_val, const std::string &new_val)
                                       {
                s_db_database = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_password,s_db_database,s_db_port); });
            //
            g_db_port->addListener([](const uint16_t &old_val, const uint16_t &new_val)
                                   {
                s_db_port = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_password,s_db_database,s_db_port); });
            // redis
            s_redis_host = g_redis_host->getValue();
            s_redis_password = g_redis_password->getValue();
            s_redis_port = g_redis_port->getValue();
            s_redismanager_ptr = blue::RedisManager::Create(s_redis_host, s_redis_port, s_redis_password);
            if (!s_redismanager_ptr)
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to create RedisManager";
            }
            else
            {
                BLUE_LOG_INFO(g_logger) << "RedisManager created";
            }
            //
            g_redis_host->addListener([](const std::string &old_val, const std::string &new_val)
                                      {
                s_redis_host = new_val;
                s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_password); });

            //
            g_redis_password->addListener([](const std::string &old_val, const std::string &new_val)
                                          {
                s_redis_password = new_val;
                s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_password); });
            //
            g_redis_port->addListener([](const uint16_t &old_val, const uint16_t &new_val)
                                      {
                s_redis_port = new_val;
                s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_password); });
            //
            s_rate_limit = g_rate_limit->getValue();
            s_rate_limit_expire = g_rate_limit_expire->getValue();
            s_cache_expire = g_cache_expire->getValue();
            //
            g_rate_limit->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                      { s_rate_limit = new_val; });
            //
            g_cache_expire->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                        { s_cache_expire = new_val; });
            //
            g_rate_limit_expire->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                             { s_rate_limit_expire = new_val; });
            // select
            s_select_timeout = g_select_timeout->getValue();
            //
            g_select_timeout->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                          { s_select_timeout = new_val; });
            // httpconnectionpool
            s_httpconnpool_mxsize = g_httpconnpool_mxsize->getValue();
            //
            g_httpconnpool_mxsize->addListener([](const uint32_t &old_val, const uint32_t &new_val)
                                               { s_httpconnpool_mxsize = new_val; });
            // mysqlpool
            s_mysqlpool_mxsize = g_mysqlpool_mxsize->getValue();
            //
            g_mysqlpool_mxsize->addListener([](const size_t &old_val, const size_t &new_val)
                                            { s_mysqlpool_mxsize = new_val; });
        }
    }
}