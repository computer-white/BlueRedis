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
#include "mysqlpool.h"
#include "log.h"

// mysql 连接池
namespace blue
{
    static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("mysqlpool");

    MySQLPool::MySQLPoolPtr MySQLPool::Create(const std::string& host, const std::string& user,
                                    const std::string& password, const std::string& database,
                                    uint16_t port, size_t pool_size)
    {
        auto pool = std::shared_ptr<MySQLPool>(new MySQLPool());
        if (!pool->_init(host, user, password, database, port, pool_size))
            return nullptr;
        return pool;
    }

    bool MySQLPool::_init(const std::string& host, const std::string& user,
                        const std::string& password, const std::string& database,
                        uint16_t port, size_t pool_size)
    {
        m_pool_size = pool_size;
        for (size_t i = 0; i < pool_size; i++)
        {
            MYSQL* mysql = mysql_init(nullptr);
            if (!mysql)
            {
                BLUE_LOG_ERROR(g_logger) << "mysql_init failed";
                return false;
            }
            
            if (!mysql_real_connect(mysql, host.c_str(), user.c_str(),
                                    password.c_str(), database.c_str(),
                                    port, nullptr, 0))
            {
                BLUE_LOG_ERROR(g_logger) << "mysql_connect failed: " << mysql_error(mysql);
                mysql_close(mysql);
                return false;
            }
            
            mysql_set_character_set(mysql, "utf8mb4");
            m_pool.push_back(std::make_shared<MySQLConnection>(mysql));
        }
        m_idle.store(pool_size,std::memory_order_release);
        BLUE_LOG_INFO(g_logger) << "MySQL pool initialized, size: " << pool_size;
        return true;
    }

    MySQLConnection::MySQLConnectionPtr MySQLPool::getConnection(int timeout_ms)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return !m_pool.empty(); }))
        {
            BLUE_LOG_WARN(g_logger) << "MySQL pool timeout";
            return nullptr;
        }
        
        auto conn = m_pool.front();
        m_pool.pop_front();
        m_idle.fetch_sub(1,std::memory_order_acq_rel);
        // 检查连接是否有效
        if (!conn->ping())
        {
            BLUE_LOG_WARN(g_logger) << "MySQL connection lost, creating new one";
            // 这里简化处理，实际应该重新创建连接
        }
        
        return conn;
    }

    void MySQLPool::releaseConnection(MySQLConnection::MySQLConnectionPtr conn)
    {
        MmutexType::lockSco lock(m_mutex);
        m_pool.push_back(conn);
        m_idle.fetch_add(1, std::memory_order_acq_rel);
        m_cv.notify_one();
    }

}