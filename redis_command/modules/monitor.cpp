#include "monitor.h"

namespace blue
{
    void MonitorModule::pushToMonitor(const std::string &cmd, MSocket::MSocketPtr sock)
    {
        std::unique_lock<std::shared_mutex> lock(m_monitor_mutex);

        if (m_monitor_clients.empty())
        {
            return;
        }

        // 构造 MONITOR 消息格式: +时间戳 [客户端IP:端口] "命令"
        auto now = std::chrono::system_clock::now();
        auto ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

        std::string message = "+" + std::to_string(ts) +
                                " [" + sock->getRemoteAddress()->toString() + "] \"" +
                                cmd + "\"\r\n";

        for (auto it = m_monitor_clients.begin(); it != m_monitor_clients.end();)
        {
            auto client = it->lock();
            if (!client || !client->isConnected())
            {
                it = m_monitor_clients.erase(it);
                continue;
            }

            // 发送消息给monitor_client
            ::send(client->getSocketfd(), message.data(), message.size(), MSG_NOSIGNAL);
            ++it;
        }
    }

    void MonitorModule::removeMonitor()
    {
        std::unique_lock<std::shared_mutex> lock(m_monitor_mutex);
        m_monitor_clients.erase(
            std::remove_if(m_monitor_clients.begin(), m_monitor_clients.end(),
                            [](const auto &weak)
                            {
                                auto ptr = weak.lock();
                                return !ptr || !ptr->isConnected();
                            }),
            m_monitor_clients.end());
    }

    size_t MonitorModule::size() const
    {
        std::shared_lock<std::shared_mutex> lock(m_monitor_mutex);
        return m_monitor_clients.size();
    }

    void MonitorModule::addMonitorClient(MSocket::MSocketPtr sock)
    {
        std::unique_lock<std::shared_mutex> lock(m_monitor_mutex);
        m_monitor_clients.push_back(sock);
    }
}