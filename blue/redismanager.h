#ifndef BLUE_REDISMANAGER_H
#define BLUE_REDISMANAGER_H
#include <hiredis/hiredis.h>
#include <memory>
#include <string>
#include "mthread.h"

// redis 管理
namespace blue
{
    /**
     * @brief Redis 客户端管理器
     */
    class RedisManager
    {
    public:
        using RedisManagerPtr = std::shared_ptr<RedisManager>;
        using MmutexType = Mmutex;
    public:
        /**
         * @brief 创建 Redis 连接
         * @param host Redis 主机，默认 127.0.0.1
         * @param port 端口，默认 6379
         * @param password 密码，可选
         * @return 管理器实例
         */
        static RedisManager::RedisManagerPtr Create(const std::string& host = "127.0.0.1",
                        uint16_t port = 6379,
                        const std::string& password = "");
        ~RedisManager();

        /**
         * @brief 设置键值对
         * @param key 键
         * @param value 值
         * @param expire_sec 过期时间（秒），0 表示永不过期
         * @return 成功返回 true
         */
        bool set(const std::string& key, const std::string& value, int expire_sec = 0);

        /**
         * @brief 获取键的值
         * @param key 键
         * @return 值，不存在返回空字符串
         */
        std::string get(const std::string& key);

        /**
         * @brief 删除键
         * @param key 键
         * @return 删除成功返回 true
         */
        bool del(const std::string& key);

        /**
         * @brief 检查键是否存在
         * @param key 键
         * @return 存在返回 true
         */
        bool exists(const std::string& key);

        /**
         * @brief 自增计数
         * @param key 键
         * @return 自增后的值，失败返回 -1
         */
        long long incr(const std::string& key);

        /**
         * @brief 设置过期时间
         * @param key 键
         * @param seconds 秒数
         * @return 成功返回 true
         */
        bool expire(const std::string& key, int seconds);

    private:
        RedisManager() = default;
        bool _connect(const std::string& host, uint16_t port, const std::string& password);

        redisContext* m_redis = nullptr;
        MmutexType m_mutex;

    };
}

#endif