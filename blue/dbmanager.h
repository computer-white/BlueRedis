#ifndef BLUE_DBMANAGER_H
#define BLUE_DBMANAGER_H
#include <mysql/mysql.h>
#include <memory>
#include "mysqlpool.h"
#include "mthread.h"
#include <string>
#include "log.h"

// 数据库管理
namespace blue
{
    /**
     * @brief 数据库管理器，封装 MySQL 连接池操作
     */
    class DbManager
    {
        public:
            using DbManagerPtr = std::shared_ptr<DbManager>;
            using MmutexType = blue::Mmutex;
        public:

            /**
             * @brief 创建数据库管理器实例
             * @param host 数据库主机
             * @param user 用户名
             * @param passward 密码
             * @param database 数据库名
             * @param port 端口，默认 3306
             * @return 管理器实例，失败返回 nullptr
             */
            static std::shared_ptr<DbManager> Create(const std::string& host,const std::string &user,
                                                    const std::string &passward, const std::string &database, uint16_t port = 3306);
            ~DbManager() = default;
        
            /**
             * @brief 执行 SQL（增删改）
             * @param sql SQL 语句
             * @return 成功返回 true
             */
            bool execute(const std::string &sql);

            /**
             * @brief 查询 SQL（查）
             * @param sql SQL 查询语句
             * @return 结果集，调用方负责释放
             */
            MYSQL_RES *query(const std::string &sql);

            /**
             * @brief 转义字符串，防止 SQL 注入
             * @param str 原始字符串
             * @return 转义后的字符串
             */
            std::string escape(const std::string &sql);

            /**
             * @brief 记录代理请求日志
             * @param client_ip 客户端 IP
             * @param method HTTP 方法
             * @param target_url 目标 URL
             * @param host 目标主机
             * @param status_code HTTP 状态码
             * @param body_size 响应体大小
             * @param user_agent 用户代理
             * @param duration_ms 耗时（毫秒）
             * @param is_forward 是否正向代理
             * @param is_ssl 是否 SSL
             * @param error_msg 错误信息（可选）
             */
            void logRequest(const std::string& client_ip,
                            const std::string& method,
                            const std::string& target_url,
                            const std::string& host,
                            int status_code,
                            int body_size,
                            const std::string& user_agent,
                            int duration_ms,
                            bool is_forward,
                            bool is_ssl,
                            const std::string& error_msg = "");
            
            /**
             * @brief 更新连接池统计
             * @param pool_key 连接池标识
             * @param total 总连接数
             * @param active 活跃连接数
             * @param idle 空闲连接数
             * @param requests 请求总数
             */
            void updatePoolStats(const std::string& pool_key,
                                int total, int active, int idle, int requests);
            
            /**
             * @brief 记录错误日志
             * @param level 错误级别
             * @param message 错误消息
             * @param file 文件名（可选）
             * @param line 行号（可选）
             */
            void logError(const std::string& level,
                        const std::string& message,
                        const std::string& file = "",
                        int line = 0);
            
        private:
            DbManager() = default;

            /**
             * @brief 从连接池获取连接执行 SQL
             * @param sql SQL 语句
             * @return 成功返回 true
             */
            bool _executePooled(const std::string& sql);
        private:
            mutable MmutexType m_mutex;
            MySQLPool::MySQLPoolPtr m_pool;
            static Logger::LoggerPtr s_logger;
    };
}


#endif