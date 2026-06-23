#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <string>
#include "blue/msocket.h"

namespace blue
{

    /**
     * @brief 订阅/发布模块
     * 负责管理频道订阅、模式订阅和消息发布
     */
    class SubscriptionModule
    {
    public:
        SubscriptionModule() = default;
        ~SubscriptionModule() = default;

        // 禁止拷贝
        SubscriptionModule(const SubscriptionModule &) = delete;
        SubscriptionModule &operator=(const SubscriptionModule &) = delete;
    public:
        /**
         * @brief 添加订阅者到频道
         * @param channel 频道名
         * @param sock 订阅者socket
         */
        void addSubscriber(const std::string &channel, MSocket::MSocketPtr sock);

        /**
         * @brief 从频道移除订阅者
         * @param channel 频道名
         * @param sock 订阅者socket
         */
        void removeSubscriber(const std::string &channel, MSocket::MSocketPtr sock);

        /**
         * @brief 发布消息到频道
         * @param channel 频道名
         * @param message 消息内容
         * @return 收到消息的订阅者数量
         */
        int publishMessage(const std::string &channel, const std::string &message);

        /**
         * @brief 移除某个socket的所有订阅
         * @param sock 需要清理的socket
         */
        void removeAllSubscribers(MSocket::MSocketPtr sock);

        /**
         * @brief 获取频道订阅者数量
         * @param channel 频道名
         */
        size_t getSubscriberCount(const std::string &channel) const;

        /**
         * @brief 获取所有频道列表
         */
        std::vector<std::string> getChannels() const;

    private:
        mutable std::shared_mutex m_channels_mutex;
        // 频道 -> 订阅者列表
        std::unordered_map<std::string, std::vector<MSocket::MSocketWPtr>> m_channels;

        mutable std::shared_mutex m_patterns_mutex;
        // 模式订阅,支持通配符 (暂不实现)
        std::unordered_map<std::string, std::vector<MSocket::MSocketWPtr>> m_patterns;
    };

} // namespace blue