#include "blue/log.h"
#include "replication.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    void ReplicationModule::startReplication()
    {
        if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE)
        {
            return;
        }

        BLUE_LOG_INFO(g_logger) << "Starting replication to " 
                                << m_repl_config.master_host << ":"
                                << m_repl_config.master_port;
        
        m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);

        // 
        if (m_relp_thread.joinable())
        {
            m_relp_thread.join();
        }

        m_relp_thread = std::thread([this](){
            
        });
    }

    void ReplicationModule::stopReplication()
    {
        BLUE_LOG_INFO(g_logger) << "Stopping replication";

        m_repl_state.store(REPL_STATE_NONE, std::memory_order_release);

        if (m_repl_sock)
        {
            m_repl_sock->close();
            m_repl_sock.reset();
        }

        if (m_relp_thread.joinable())
        {
            m_relp_thread.join();
        }

        BLUE_LOG_INFO(g_logger) << "Replication stopped";
    }

    void ReplicationModule::addSlaves(MSocket::MSocketPtr sock)
    {
        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);
        m_slaves.push_back(sock);
    }

    Task<void> ReplicationModule::replicationLoop()
    {
        BLUE_LOG_INFO(g_logger) << "Replication Loop start";
        RespStreamParser parser;
        while (m_repl_state.load(std::memory_order_acquire) != REPL_STATE_NONE)
        {
            if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            BLUE_LOG_INFO(g_logger) << "Connecting to master " 
                                    << m_repl_config.master_host << ":"
                                    << m_repl_config.master_port;
            
            m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
            std::string host_with_port = m_repl_config.master_host + ":" + std::to_string(m_repl_config.master_port);
            auto addr = Address::LookupAnyIpAddress(host_with_port);
            if (!addr)
            {
                BLUE_LOG_DEBUGE(g_logger) << "Invalid master address";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            auto sock = MSocket::CreateTcp(addr);
            if (!sock)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            bool conn = co_await sock->connect(addr,3000);
            if (!conn)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            BLUE_LOG_INFO(g_logger) << "Connected to master, fd=" << sock->getSocketfd();
            m_repl_sock = sock;
            m_repl_state.store(REPL_STATE_HANDSHAKE, std::memory_order_release);

            if (!m_repl_config.master_password.empty())
            {
                std::vector<RespValue> auth_args;
                auth_args.push_back(RespValue::bulk_string("AUTH"));
                auth_args.push_back(RespValue::bulk_string(m_repl_config.master_password));
                std::string auth_cmd = RespValue::encode(RespValue::array(std::move(auth_args)));
                ssize_t send = co_await sock->send(auth_cmd.data(), auth_cmd.size(),MSG_NOSIGNAL);
                if (send <= 0)
                {
                    BLUE_LOG_DEBUGE(g_logger) << "Failed to send AUTH";
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                char buf[128];
                ssize_t ret = co_await sock->recv(buf, sizeof(buf));
                if (ret <= 0)
                {
                    BLUE_LOG_DEBUGE(g_logger) << "Failed to recv AUTH response";
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }

                std::string resp(buf, ret);
                if (resp.find("+OK") == std::string::npos)
                {
                    BLUE_LOG_ERROR(g_logger) << "AUTH failed: " << resp;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }
                BLUE_LOG_INFO(g_logger) << "AUTH successful";
            }

            // 发送SYNC命令
            std::string sync_cmd = "*1\r\n$4\r\nSYNC\r\n";
            ssize_t send = co_await sock->send(sync_cmd.data(), sync_cmd.size());
            if (send <= 0)
            {
                BLUE_LOG_DEBUGE(g_logger) << "Failed to send SYNC";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

            BLUE_LOG_INFO(g_logger) << "Send SYNC, wait for RDB...";
            m_repl_state.store(REPL_STATE_TRANSFER, std::memory_order_release);

            // 接收rdb回复
            




        }

    }   

    void ReplicationModule::broadcastToSlaves(const std::string& cmd)
    {

    }

    void ReplicationModule::generateRDB()
    {

    }

}