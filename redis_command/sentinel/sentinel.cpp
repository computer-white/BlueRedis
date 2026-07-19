#include <chrono>
#include <sstream>
#include <regex>
#include "sentinel.h"
#include "blue/await.h"
#include "blue/log.h"
#include "blue/io_manager.h"
#include "blue/address.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("sentinel");

    Sentinel::Sentinel(const SentinelConfig &config)
        : m_config(config)
    {
        m_config.sentinel_id = this->generateSentinelId();
        BLUE_LOG_INFO(g_logger) << "Sentinel initialized, ID: " << m_config.sentinel_id;
    }

    Sentinel::~Sentinel()
    {
        this->stop();
    }

    void Sentinel::start()
    {
        if (m_running.exchange(true))
        {
            return;
        }

        BLUE_LOG_INFO(g_logger) << "Sentinel starting...";

        m_master_info.host = m_config.master_host;
        m_master_info.port = m_config.master_port;
        m_master_info.is_master = true;
        m_master_info.last_ping = std::chrono::steady_clock::now();

        // 启动监控协程
        IOManager::GetThis()->schedule(this->monitorLoop());

        // 启动 Sentinel 服务器
        m_server_thread = std::thread([this]()
                                      { this->startSentinelServer(); });

        BLUE_LOG_INFO(g_logger) << "Sentinel started";
    }

    void Sentinel::stop()
    {
        if (!m_running.exchange(false))
        {
            return;
        }

        BLUE_LOG_INFO(g_logger) << "Sentinel stopping...";

        if (m_server_thread.joinable())
        {
            m_server_thread.join();
        }

        BLUE_LOG_INFO(g_logger) << "Sentinel stopped";
    }

    Task<void> Sentinel::monitorLoop()
    {
        BLUE_LOG_INFO(g_logger) << "Monitor Loop started";

        while (m_running.load(std::memory_order_acquire))
        {
            co_await sleepFor(1);

            std::unique_lock<std::shared_mutex> lock(m_mutex);

            // 检查主节点健康
            co_await this->checkMasterHealth();

            // 检查从节点健康
            co_await this->checkSlavesHeadlth();

            // 发现新的从节点
            this->discoverSlaves();

            // 如果主节点下线，故障转移
            if (m_master_info.is_down && !m_failover_in_progress.load(std::memory_order_acquire))
            {
                lock.unlock();
                this->startFailOver();
                lock.lock();
            }
        }
    }

    Task<void> Sentinel::checkMasterHealth()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_master_info.last_ping).count();

        bool pingOK = co_await this->pingNode(m_master_info.host, m_master_info.port);

        if (pingOK)
        {
            m_master_info.last_ping = now;
            if (m_master_info.is_down)
            {
                BLUE_LOG_INFO(g_logger) << "Master recovered: "
                                        << m_master_info.host << ":" << m_master_info.port;
                m_master_info.is_down = false;
            }
        }
        else if (elapsed > m_config.down_after_milliseconds)
        {
            if (!m_master_info.is_down)
            {
                BLUE_LOG_WARN(g_logger) << "Master is DOWN: "
                                        << m_master_info.host << ":" << m_master_info.port
                                        << " (elapsed: " << elapsed << "ms)";
                m_master_info.is_down = true;

                // 通知其他 Sentinel
                // TODO: 发送 Sentinel 间消息
            }
        }
    }

    Task<void> Sentinel::checkSlavesHeadlth()
    {
        for (auto &slave : m_slaves_info)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - slave.last_ping).count();

            bool pingOK = co_await this->pingNode(slave.host, slave.port);

            if (pingOK)
            {
                slave.last_ping = now;
                if (slave.is_down)
                {
                    BLUE_LOG_INFO(g_logger) << "Slave recovered"
                                            << slave.host << ":" << slave.port;
                    slave.is_down = false;
                }
            }
            else if (elapsed > m_config.down_after_milliseconds)
            {
                if (!slave.is_down)
                {
                    BLUE_LOG_WARN(g_logger) << "Slave is DOWN: "
                                            << slave.host << ":" << slave.port;
                    slave.is_down = true;
                }
            }
        }
    }

    Task<void> Sentinel::discoverSlaves()
    {
        try
        {
            std::string info = co_await this->getNodeInfo(m_master_info.host, m_master_info.port);
            auto slaves = this->parseSlaves(info);

            for (auto &new_slaves : slaves)
            {
                bool found = false;
                for (auto &existed : m_slaves_info)
                {
                    if (existed.host == new_slaves.host &&
                        existed.port == new_slaves.port)
                    {
                        // TODO
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    Task<void> Sentinel::startFailOver()
    {
        if (m_failover_in_progress.exchange(true))
        {
            co_return;
        }

        BLUE_LOG_WARN(g_logger) << "Start failover for master: "
                                << m_master_info.host << ":" << m_master_info.port;
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        try
        {
            NodeInFo *new_master = this->electNewMaster();
            if (!new_master)
            {
                BLUE_LOG_ERROR(g_logger) << "FailOver: no suitable slave found";
                m_failover_in_progress.store(false, std::memory_order_release);
                co_return;
            }

            BLUE_LOG_INFO(g_logger) << "FailOver: elected new master: "
                                    << new_master->host << ":" << new_master->port
                                    << "(offset: " << new_master->offset << ")";
            // 提升为主节点
            bool promoteOK = co_await this->promoteToMaster(*new_master);
            if (!promoteOK)
            {
                BLUE_LOG_ERROR(g_logger) << "FailOver: failed to promote to master";
                m_failover_in_progress.store(false, std::memory_order_release);
                co_return;
            }

            // 更新节点的复制目标
            co_await this->updateReplicas(*new_master);

            // 更新主节点
            std::string old_master_host = m_master_info.host;
            uint16_t old_master_port = m_master_info.port;

            m_master_info = *new_master;
            m_master_info.is_down = false;
            m_master_info.is_master = true;
            m_master_info.is_slave = false;
            m_master_info.master_host = "";
            m_master_info.master_port = 0;
            m_master_info.master_offset = 0;
            m_config_epoch++;

            BLUE_LOG_INFO(g_logger) << "FailOver completed. New master: "
                                    << m_master_info.host << ":" << m_master_info.port
                                    << "(epoch: " << m_config_epoch.load(std::memory_order_acquire) << ")";

            // TODO 当旧的主节点恢复自动成为从节点

        }
        catch (const std::exception& e)
        {
            BLUE_LOG_ERROR(g_logger) << "FailOver failed: " << e.what();
        }

        m_failover_in_progress.store(false, std::memory_order_release);
    }

    NodeInFo *Sentinel::electNewMaster()
    {
        std::vector<NodeInFo*> candidates;
        uint64_t max_offset = 0;

        // 计算最大偏移量
        for (auto& slave : m_slaves_info)
        {
            if (!slave.is_down && slave.offset > max_offset)
            {
                max_offset = slave.offset;
            }
        }

        uint64_t min_offset = max_offset / 2;
        for (auto& slave : m_slaves_info)
        {
            if (!slave.is_down && slave.offset >= min_offset)
            {
                candidates.push_back(&slave);
            }
        }

        if (candidates.empty())
        {
            return nullptr;
        }

        std::sort(candidates.begin(), candidates.end(), [](const NodeInFo* a, const NodeInFo* b){
            if (a->offset != b->offset)
            {
                return a->offset > b->offset;
            }
            return a->replid < b->replid;
        });
        
        return candidates[0];
    }

    Task<bool> Sentinel::promoteToMaster(NodeInFo slave)
    {
        try
        {
            std::string response = co_await this->sendCommand(slave.host, slave.port, "REPLICAOF NO ONE");

            if (response.find("+OK") == std::string::npos)
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to promote: " << response;
                co_return false;
            }

            BLUE_LOG_INFO(g_logger) << "Promoted: " << slave.host << ":" << slave.port
                                    << "to master";
            co_return true;
        }
        catch (const std::exception& e)
        {
            BLUE_LOG_ERROR(g_logger) << "Promote failed: " << e.what();
            co_return false;
        }
    }

    Task<void> Sentinel::updateReplicas(NodeInFo newMaster)
    {
        for (auto& slave : m_slaves_info)
        {
            // 跳过新的主节点
            if (slave.host == newMaster.host && slave.port == newMaster.port)
            {
                continue;
            }

            try
            {
                std::string cmd = "REPLICAOF" + newMaster.host + " "
                                    + std::to_string(newMaster.port);
                std::string response = co_await this->sendCommand(slave.host, slave.port, cmd);

                if (response.find("+OK") != std::string::npos)
                {
                    BLUE_LOG_INFO(g_logger) << "Updated slave: "
                                            << slave.host << ":" << slave.port
                                            << " -> " 
                                            << newMaster.host << ":" << newMaster.port;
                }
            }
            catch (const std::exception& e)
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to update slave: "
                                         << slave.host << ":" << slave.port
                                         << ": "
                                         << e.what();
            }

        }
        co_return;
    }

    Task<std::string> Sentinel::sendCommand(std::string host, uint16_t port, std::string cmd)
    {
        std::string response;
        std::string host_with_port = host + ":" + std::to_string(port);
        auto addr = Address::LookupAnyIpAddress(host_with_port);
        if (!addr)
        {
            BLUE_LOG_INFO(g_logger) << "Failed to LookupIpAddress: " << host_with_port;
            co_return response;
        }
        auto sock = MSocket::CreateTcpSocket();
        if (!sock)
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to CreateTcpSocket";
            co_return response;
        }
        bool ConnOK = co_await sock->connect(addr);
        if (!ConnOK)
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to connect: " << addr->toString();
            co_return response;
        }

        // 构造消息
        std::string request = RespValue::encode(*RespValue::array(
            std::vector<RespValue>{*RespValue::bulk_string(cmd)}));
        
        ssize_t res = co_await sock->send(request.data(), request.size());
        if (res < 0)
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to send, request: " << request;
            co_return response;
        }

        // 接受响应
        char buffer[8192];
        ssize_t n = co_await sock->recv(buffer, sizeof(buffer));
        if (n <= 0)
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to recv, errno : " << errno
                                     << "strerror : " << strerror(errno);
            co_return response;
        }
        response.append(buffer,n);
        co_return response;
    }

    Task<std::string> Sentinel::sendCommandWithPassWord(std::string host, uint16_t port, std::string cmd, std::string password)
    {
        std::string response;
        response = co_await this->sendCommand(host, port, std::move(password));
        if (response.find("+OK") == std::string::npos)
        {
            co_return response;
        }
        response = co_await this->sendCommand(host, port, std::move(cmd));
        co_return response;
    }

    Task<bool> Sentinel::pingNode(std::string host, uint16_t port)
    {
        try
        {
            std::string response = co_await this->sendCommand(std::move(host), port, "PING");
            co_return response.find("+PONG") != std::string::npos;
        }
        catch(...)
        {
            co_return false;
        }
    }

    Task<std::string> Sentinel::getNodeInfo(std::string host, uint16_t port)
    {
        auto res = co_await this->sendCommand(std::move(host), port, "INFO REPLICATION");
        co_return res;
    }

    NodeInFo Sentinel::parseInfo(const std::string &info)
    {

    }

    std::vector<NodeInFo> Sentinel::parseSlaves(const std::string &info)
    {

    }

    AutoRespValue Sentinel::handleSentinelMaster()
    {
    }

    AutoRespValue Sentinel::handleSentinelSlaves(const std::string &master_name)
    {
    }

    AutoRespValue Sentinel::handleSentinelGetMasterAddr(const std::string &master_name)
    {
    }

    AutoRespValue Sentinel::handleSentinelReset(const std::string &pattern)
    {
    }

    AutoRespValue Sentinel::handleSentinelFailOver(const std::string &master_name)
    {
    }

    AutoRespValue Sentinel::handleSentinelMonitor(const std::string &master_name,
                                                  const std::string &host, uint16_t port,
                                                  int quorum)
    {
    }

    AutoRespValue Sentinel::handleSentinelRemove(const std::string &master_name)
    {
    }

    void Sentinel::handleSentinelMessage(const std::string &msg)
    {
    }

    AutoRespValue Sentinel::handleCommand(std::vector<RespValue> &args, MSocket::MSocketPtr sock)
    {
    }

    void Sentinel::startSentinelServer()
    {
    }

    std::string Sentinel::generateSentinelId()
    {
    }

    std::pair<std::string, int> Sentinel::getMasterAddr() const
    {
    }

    std::vector<NodeInFo> Sentinel::getSlaves() const
    {
    }

    bool Sentinel::failOver()
    {
    }
}