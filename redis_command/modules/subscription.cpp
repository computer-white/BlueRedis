#include "subscription.h"
#include "blue/resp_parser.h"
#include "blue/log.h"
#include <algorithm>

namespace blue
{

    static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    void SubscriptionModule::addSubscriber(const std::string &channel, MSocket::MSocketPtr sock)
    {
        if (!sock)
            return;

        std::unique_lock<std::shared_mutex> lock(m_channels_mutex);
        m_channels[channel].push_back(sock);

        BLUE_LOG_DEBUGE(g_logger) << "Added subscriber to channel: " << channel
                                  << ", fd: " << sock->getSocketfd();
    }

    void SubscriptionModule::removeSubscriber(const std::string &channel, MSocket::MSocketPtr sock)
    {
        if (!sock)
            return;

        std::unique_lock<std::shared_mutex> lock(m_channels_mutex);
        auto it = m_channels.find(channel);
        if (it == m_channels.end())
        {
            return;
        }

        auto &subscribers = it->second;
        subscribers.erase(
            std::remove_if(subscribers.begin(), subscribers.end(),
                           [sock](const auto &weak)
                           {
                               auto ptr = weak.lock();
                               return !ptr || ptr.get() == sock.get();
                           }),
            subscribers.end());

        if (subscribers.empty())
        {
            m_channels.erase(it);
            BLUE_LOG_DEBUGE(g_logger) << "Channel " << channel << " has no subscribers, removed";
        }
        else
        {
            BLUE_LOG_DEBUGE(g_logger) << "Removed subscriber from channel: " << channel
                                      << ", fd: " << sock->getSocketfd();
        }
    }

    int SubscriptionModule::publishMessage(const std::string &channel, const std::string &message)
    {
        std::shared_lock<std::shared_mutex> lock(m_channels_mutex);
        auto it = m_channels.find(channel);
        if (it == m_channels.end())
        {
            return 0;
        }

        // 构造 RESP 消息
        std::vector<RespValue> msg;
        msg.push_back(*RespValue::bulk_string("message"));
        msg.push_back(*RespValue::bulk_string(channel));
        msg.push_back(*RespValue::bulk_string(message));
        std::string data = RespValue::encode(*RespValue::array(std::move(msg)));

        int receiver_count = 0;
        for (auto &weak : it->second)
        {
            auto sock = weak.lock();
            if (sock && sock->isConnected())
            {
                // 同步发送
                size_t sent = 0;
                while (sent < data.size())
                {
                    ssize_t n = ::send(sock->getSocketfd(),
                                       data.data() + sent,
                                       data.size() - sent,
                                       MSG_NOSIGNAL);
                    if (n <= 0)
                    {
                        BLUE_LOG_ERROR(g_logger) << "Failed to send message to subscriber, fd: "
                                                 << sock->getSocketfd();
                        break;
                    }
                    sent += n;
                }
                if (sent == data.size())
                {
                    receiver_count++;
                }
            }
        }

        BLUE_LOG_DEBUGE(g_logger) << "Published message to channel: " << channel
                                  << ", receivers: " << receiver_count;
        return receiver_count;
    }

    void SubscriptionModule::removeAllSubscribers(MSocket::MSocketPtr sock)
    {
        if (!sock)
            return;

        std::unique_lock<std::shared_mutex> lock(m_channels_mutex);
        for (auto it = m_channels.begin(); it != m_channels.end();)
        {
            auto &subscribers = it->second;
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                               [sock](const auto &weak)
                               {
                                   auto ptr = weak.lock();
                                   return !ptr || ptr.get() == sock.get();
                               }),
                subscribers.end());

            if (subscribers.empty())
            {
                it = m_channels.erase(it);
            }
            else
            {
                ++it;
            }
        }

        BLUE_LOG_DEBUGE(g_logger) << "Removed all subscriptions for fd: " << sock->getSocketfd();
    }

    size_t SubscriptionModule::getSubscriberCount(const std::string &channel) const
    {
        std::shared_lock<std::shared_mutex> lock(m_channels_mutex);
        auto it = m_channels.find(channel);
        if (it == m_channels.end())
        {
            return 0;
        }
        return it->second.size();
    }

    std::vector<std::string> SubscriptionModule::getChannels() const
    {
        std::shared_lock<std::shared_mutex> lock(m_channels_mutex);
        std::vector<std::string> channels;
        channels.reserve(m_channels.size());
        for (const auto &[channel, _] : m_channels)
        {
            channels.push_back(channel);
        }
        return channels;
    }

} // namespace blue