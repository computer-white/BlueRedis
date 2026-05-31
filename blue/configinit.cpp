#include "configinit.h"
#include "config.h"
#include "redismanager.h"
#include "http/httpconnection.h"
#include "dbmanager.h"
#include "mysqlpool.h"

// 数据库和redis配置
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
        // 相关数据库配置
        static blue::ConfigVar<std::string>::ConfigVarPtr g_db_host = 
                blue::Config::Lookup<std::string>("db.host","localhost","db host");

        static blue::ConfigVar<std::string>::ConfigVarPtr g_db_user = 
                blue::Config::Lookup<std::string>("db.user","blue","db user");
        
        static blue::ConfigVar<std::string>::ConfigVarPtr g_db_passward = 
                blue::Config::Lookup<std::string>("db.passward","","db passward");
        
        static blue::ConfigVar<std::string>::ConfigVarPtr g_db_database = 
                blue::Config::Lookup<std::string>("db.database","blue_proxy","db database");
        
        static blue::ConfigVar<uint16_t>::ConfigVarPtr g_db_port = 
                blue::Config::Lookup<uint16_t>("db.port",3306,"db port");
            
        // 相关redis配置
        static blue::ConfigVar<std::string>::ConfigVarPtr g_redis_host = 
                blue::Config::Lookup<std::string>("redis.host","127.0.0.1","redis host");
        
        static blue::ConfigVar<uint16_t>::ConfigVarPtr g_redis_port = 
                blue::Config::Lookup<uint16_t>("redis.port",6379,"redis port");
        
        static blue::ConfigVar<std::string>::ConfigVarPtr g_redis_passward = 
                blue::Config::Lookup<std::string>("redis.passward","","redis passward");

        static blue::ConfigVar<uint64_t>::ConfigVarPtr g_rate_limit = 
                blue::Config::Lookup<uint64_t>("redis.rate_limit",100,"redis rate limit");

        static blue::ConfigVar<uint64_t>::ConfigVarPtr g_rate_limit_expire = 
                blue::Config::Lookup<uint64_t>("redis.rate_limit_expire",60,"redis rate limit expire");

        static blue::ConfigVar<uint64_t>::ConfigVarPtr g_cache_expire = 
                blue::Config::Lookup<uint64_t>("redis.cache_expire",60,"redis cache expire");
        
        // select超时设置(配置里面是ms)
        static blue::ConfigVar<uint64_t>::ConfigVarPtr g_select_timeout = 
                blue::Config::Lookup<uint64_t>("select.timeout",1000,"select timeout");
        
        // httpconnectionpool size
        static blue::ConfigVar<uint32_t>::ConfigVarPtr g_httpconnpool_mxsize = 
                blue::Config::Lookup<uint32_t>("httpconnectionpool.maxsize",10,"http connectionpool maxsize");
        
        // mysqlpool size
        static blue::ConfigVar<size_t>::ConfigVarPtr g_mysqlpool_mxsize = 
                blue::Config::Lookup<size_t>("httpmysqlpool.maxsize",10,"http mysqlpool maxsize");

        void IniteConfig()
        {
            // db
            s_db_host = g_db_host->getValue();
            s_db_user = g_db_user->getValue();
            s_db_passward = g_db_passward->getValue();
            s_db_database = g_db_database->getValue();
            s_db_port = g_db_port->getValue();
            s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_passward,s_db_database,s_db_port);
            if (!s_dbmanager_ptr) 
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to create DbManager";
            }
            else
            {
                BLUE_LOG_INFO(g_logger) << "DbManager created";
            }
            g_db_host->addListener([](const std::string &old_val, const std::string &new_val){
                s_db_host = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_passward,s_db_database,s_db_port);
            });
            g_db_user->addListener([](const std::string &old_val, const std::string &new_val){
                s_db_user= new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_passward,s_db_database,s_db_port);
            });
            g_db_passward->addListener([](const std::string &old_val, const std::string &new_val){
                s_db_passward= new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_passward,s_db_database,s_db_port);
            });
            g_db_database->addListener([](const std::string &old_val, const std::string &new_val){
                s_db_database = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_passward,s_db_database,s_db_port);
            });
            g_db_port->addListener([](const uint16_t &old_val, const uint16_t &new_val){
                s_db_port = new_val;
                s_dbmanager_ptr = blue::DbManager::Create(s_db_host,s_db_user,s_db_passward,s_db_database,s_db_port);
            });
            // redis
            s_redis_host = g_redis_host->getValue();
            s_redis_passward = g_redis_passward->getValue();
            s_redis_port = g_redis_port->getValue();
            s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_passward);
            if (!s_redismanager_ptr) 
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to create RedisManager";
            }
            else
            {
                BLUE_LOG_INFO(g_logger) << "RedisManager created";
            }
            g_redis_host->addListener([](const std::string &old_val, const std::string &new_val){
                s_redis_host = new_val;
                s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_passward);
            });
            g_redis_passward->addListener([](const std::string &old_val, const std::string &new_val){
                s_redis_passward = new_val;
                s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_passward);
            });
            g_redis_port->addListener([](const uint16_t &old_val, const uint16_t &new_val){
                s_redis_port = new_val;
                s_redismanager_ptr = blue::RedisManager::Create(s_redis_host,s_redis_port,s_redis_passward);
            });
            s_rate_limit = g_rate_limit->getValue();
            s_rate_limit_expire = g_rate_limit_expire->getValue();
            s_cache_expire = g_cache_expire->getValue();
            g_rate_limit->addListener([](const uint64_t &old_val, const uint64_t &new_val){
                s_rate_limit = new_val;
            });
            g_cache_expire->addListener([](const uint64_t &old_val, const uint64_t &new_val){
                s_cache_expire = new_val;
            });
            g_rate_limit_expire->addListener([](const uint64_t &old_val, const uint64_t &new_val){
                s_rate_limit_expire = new_val;
            });
            // select
            s_select_timeout = g_select_timeout->getValue();
            g_select_timeout->addListener([](const uint64_t &old_val, const uint64_t &new_val){
                s_select_timeout = new_val;
            });
            // httpconnectionpool
            s_httpconnpool_mxsize = g_httpconnpool_mxsize->getValue();
            g_httpconnpool_mxsize->addListener([](const uint32_t &old_val, const uint32_t &new_val){
                s_httpconnpool_mxsize = new_val;
            });
            // mysqlpool
            s_mysqlpool_mxsize = g_mysqlpool_mxsize->getValue();
            g_mysqlpool_mxsize->addListener([](const size_t &old_val, const size_t &new_val){
                s_mysqlpool_mxsize = new_val;
            });
        }
    }
}