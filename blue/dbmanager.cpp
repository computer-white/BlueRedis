#include "dbmanager.h"

// 数据库管理
namespace blue
{
    Logger::LoggerPtr DbManager::s_logger = BLUE_LOG_NAME("system");

    std::shared_ptr<DbManager> DbManager::Create(const std::string& host,const std::string &user,
                                                    const std::string &passward, const std::string &database, 
                                                    uint16_t port)
    {
        std::shared_ptr<DbManager> mgr(new DbManager());
        mgr->m_pool = MySQLPool::Create(host,user,passward,database,port,8);
        if (!mgr->m_pool) return nullptr;
        return mgr;
    }

    bool DbManager::execute(const std::string &sql)
    {
        return _executePooled(sql);
    }

    bool DbManager::_executePooled(const std::string& sql)
    {
        auto conn = m_pool->getConnection();
        if (!conn) return false;
        
        bool ok = (mysql_query(conn->get(), sql.c_str()) == 0);
        
        m_pool->releaseConnection(conn);
        return ok;
    }

    MYSQL_RES *DbManager::query(const std::string &sql)
    {
        auto conn = m_pool->getConnection();
        if (!conn) return nullptr;
        
        if (mysql_query(conn->get(), sql.c_str()) != 0)
        {
            BLUE_LOG_ERROR(s_logger) << "SQL query failed: " << mysql_error(conn->get());
            m_pool->releaseConnection(conn);
            return nullptr;
        }
        
        MYSQL_RES* res = mysql_store_result(conn->get());
        m_pool->releaseConnection(conn);
        return res;
    }

    std::string DbManager::escape(const std::string &str)
    {
        if (str.empty()) return "";
        auto conn = m_pool->getConnection();
        if (!conn) return "";
        
        std::vector<char> buf(str.size() * 2 + 1);
        mysql_real_escape_string(conn->get(), buf.data(), str.c_str(), str.size());
        m_pool->releaseConnection(conn);
        return std::string(buf.data());
    }

    void DbManager::logRequest(const std::string& client_ip,
                            const std::string& method,
                            const std::string& target_url,
                            const std::string& host,
                            int status_code,
                            int body_size,
                            const std::string& user_agent,
                            int duration_ms,
                            bool is_forward,
                            bool is_ssl,
                            const std::string& error_msg)
    {
        std::string sql = "INSERT INTO request_logs "
                      "(client_ip, method, target_url, host, status_code, "
                      "body_size, user_agent, duration_ms, is_forward, is_ssl, error_msg) "
                      "VALUES ('" +
                      escape(client_ip) + "', '" +
                      escape(method) + "', '" +
                      escape(target_url) + "', '" +
                      escape(host) + "', " +
                      std::to_string(status_code) + ", " +
                      std::to_string(body_size) + ", '" +
                      escape(user_agent) + "', " +
                      std::to_string(duration_ms) + ", " +
                      std::to_string(is_forward ? 1 : 0) + ", " +
                      std::to_string(is_ssl ? 1 : 0) + ", '" +
                      escape(error_msg) + "')";
        execute(sql);
    }

    void DbManager::updatePoolStats(const std::string& pool_key,
                        int total, int active, int idle, int requests)
    {
        std::string sql = "INSERT INTO pool_stats "
                      "(pool_key, total_connections, active_connections, "
                      "idle_connections, total_requests) "
                      "VALUES ('" + escape(pool_key) + "', " +
                      std::to_string(total) + ", " +
                      std::to_string(active) + ", " +
                      std::to_string(idle) + ", " +
                      std::to_string(requests) + ") "
                      "ON DUPLICATE KEY UPDATE "
                      "total_connections=" + std::to_string(total) + ", "
                      "active_connections=" + std::to_string(active) + ", "
                      "idle_connections=" + std::to_string(idle) + ", "
                      "total_requests=" + std::to_string(requests);
        execute(sql);
    }

    void DbManager::logError(const std::string& level,
                const std::string& message,
                const std::string& file,
                int line)
    {
        std::string sql = "INSERT INTO error_logs (level, message, file_name, line_number) "
                      "VALUES ('" +
                      escape(level) + "', '" +
                      escape(message) + "', '" +
                      escape(file) + "', " +
                      std::to_string(line) + ")";
        execute(sql);
    }

    
}