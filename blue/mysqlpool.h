#ifndef BLUE_MYSQLPOOL_H
#define BLUE_MYSQLPOOL_H
#include <mysql/mysql.h>
#include <memory>
#include <string>
#include <list>
#include "mthread.h"
#include <condition_variable>
#include <chrono>

// mysql 连接池
namespace blue
{
    /**
     * @brief MySQL 单个连接封装，RAII 管理
     */
    class MySQLConnection
    {
    public:
        using MySQLConnectionPtr = std::shared_ptr<MySQLConnection>;

    public:

        /**
         * @brief 构造函数
         * @param mysql 已建立的 MySQL 连接
         */
        MySQLConnection(MYSQL *mysql)
            : m_mysql(mysql) {}

        /**
         * @brief 析构，自动关闭连接
         */
        ~MySQLConnection()
        {
            if (m_mysql)
                mysql_close(m_mysql);
        }

        /**
         * @brief 获取原始 MySQL 句柄
         * @return MYSQL 指针
         */
        MYSQL *get() const { return m_mysql; }

        /**
         * @brief 检测连接是否存活
         * @return 存活返回 true
         */
        bool ping() { return mysql_ping(m_mysql) == 0; }

    private:
        MYSQL *m_mysql;
    };

    // 连接池最大大小
    extern size_t s_mysqlpool_mxsize;

    /**
     * @brief MySQL 连接池，管理连接的分配与回收
     */
    class MySQLPool 
    {
    public:
        using MySQLPoolPtr = std::shared_ptr<MySQLPool>;
        using MmutexType = blue::Mmutex;

        /**
         * @brief 创建连接池
         * @param host 数据库主机
         * @param user 用户名
         * @param password 密码
         * @param database 数据库名
         * @param port 端口，默认 3306
         * @param pool_size 连接池大小，默认 8
         * @return 连接池实例
         */
        static MySQLPoolPtr Create(const std::string& host, const std::string& user,
                        const std::string& password, const std::string& database,
                        uint16_t port = 3306, size_t pool_size = s_mysqlpool_mxsize);
        
        /**
         * @brief 获取一个数据库连接（阻塞等待）
         * @param timeout_ms 超时时间（毫秒）
         * @return 连接指针，超时返回 nullptr
         */
        MySQLConnection::MySQLConnectionPtr getConnection(int timeout_ms = 3000);

        /**
         * @brief 归还连接到连接池
         * @param conn 要归还的连接
         */
        void releaseConnection(MySQLConnection::MySQLConnectionPtr conn);

        /**
         * @brief 获取空闲连接数
         * @return 空闲连接数
         */
        size_t idleCounts() const { return m_idle.load(std::memory_order_acquire); }

        /**
         * @brief 获取总连接数
         * @return 连接池大小
         */
        size_t totalCounts() const { return m_pool_size; }
        
    private:
        MySQLPool() = default;

        /**
         * @brief 初始化连接池，预创建所有连接
         * @return 成功返回 true
         */
        bool _init(const std::string& host, const std::string& user,
                const std::string& password, const std::string& database,
                uint16_t port, size_t pool_size);
        
        MmutexType m_mutex;
        std::list<MySQLConnection::MySQLConnectionPtr> m_pool;
        std::condition_variable m_cv;
        size_t m_pool_size = 8;
        std::atomic<size_t> m_idle{0};
    };
}

#endif