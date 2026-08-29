#include "blue/configinit.h"
#include "blue/loadConfig.h"
#include "blue/redismanager.h"
#include "blue/dbmanager.h"
#include "blue/mysqlpool.h"
#include "http/httpconnection.h"

// 数据库和redis配置
namespace blue
{
    std::string s_log_rotate_filename_template = ""; // 日志文件模板名
    uint64_t s_log_rotate_file_size = 0;             // 每个文件的大小
    uint32_t s_log_rotate_file_num = 0;              // 轮转文件最大数量

    static blue::ConfigVar<LogRotateDefine>::ConfigVarPtr
        g_logRotateDefine_config_ptr = blue::Config::Lookup<LogRotateDefine>("logrotate",
                                                                             LogRotateDefine(),
                                                                             "logrotate configuration");

    static blue::ConfigVar<std::set<LogDefine>>::ConfigVarPtr
        g_logDefine_config_ptr = blue::Config::Lookup<std::set<LogDefine>>("logs",
                                                                           std::set<LogDefine>(),
                                                                           "logs LogDefine");

    void blueIniteConfig()
    {
        // 添加监听器,同时读取内容设置到日志系统里面
        g_logDefine_config_ptr->addListener([](const std::set<LogDefine> &old_val,
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
                        std::cout << "log config error n_val.LogAppenderDefine.fomatter is error "
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

        s_log_rotate_filename_template = g_logRotateDefine_config_ptr->getValue().rotate_filename;
        s_log_rotate_file_size = g_logRotateDefine_config_ptr->getValue().rotate_file_size;
        s_log_rotate_file_num = g_logRotateDefine_config_ptr->getValue().rotate_file_num;
        g_logRotateDefine_config_ptr->addListener([](const LogRotateDefine &old_val,
                                                     const LogRotateDefine &new_val)
                                                  {
                std::cout << "Begin update Rotate configuration!" << std::endl;
                // 更新
                s_log_rotate_filename_template = new_val.rotate_filename;
                s_log_rotate_file_num = new_val.rotate_file_num;
                s_log_rotate_file_size = new_val.rotate_file_size; });
    }

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