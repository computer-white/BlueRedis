#include "redismanager.h"
#include "log.h"

// redis 管理
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    RedisManager::RedisManagerPtr RedisManager::Create(const std::string& host,
                    uint16_t port,
                    const std::string& password)
    {
        auto mgr = std::shared_ptr<RedisManager>(new RedisManager());
        if (!mgr->_connect(host,port,password))
        {
            BLUE_LOG_ERROR(g_logger) << "redis connect failed";
            return nullptr;
        }
        return mgr;
    }
    RedisManager::~RedisManager()
    {
        if (m_redis)
        {
            redisFree(m_redis);
            m_redis = nullptr;
        }

    }
    bool RedisManager::set(const std::string& key, const std::string& value, int expire_sec)
    {
        MmutexType::lockSco lock(m_mutex);
        redisReply* reply;
        if (expire_sec > 0)
            reply = (redisReply*)redisCommand(m_redis, "SETEX %s %d %s",
                                            key.c_str(), expire_sec, value.c_str());
        else
            reply = (redisReply*)redisCommand(m_redis, "SET %s %s",
                                            key.c_str(), value.c_str());

        if (!reply) return false;
        bool ok = (reply->type != REDIS_REPLY_ERROR);
        freeReplyObject(reply);
        return ok;
    }
    std::string RedisManager::get(const std::string& key)
    {
         MmutexType::lockSco lock(m_mutex);
        redisReply* reply = (redisReply*)redisCommand(m_redis, "GET %s", key.c_str());
        if (!reply || reply->type != REDIS_REPLY_STRING)
        {
            if (reply) freeReplyObject(reply);
            return "";
        }
        std::string result(reply->str, reply->len);
        freeReplyObject(reply);
        return result;
    }
    bool RedisManager::del(const std::string& key)
    {
        MmutexType::lockSco lock(m_mutex);
        redisReply* reply = (redisReply*)redisCommand(m_redis, "DEL %s", key.c_str());
        if (!reply) return false;
        bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return ok;
    }
    bool RedisManager::exists(const std::string& key)
    {
        MmutexType::lockSco lock(m_mutex);
        redisReply* reply = (redisReply*)redisCommand(m_redis, "EXISTS %s", key.c_str());
        if (!reply) return false;
        bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return ok;
    }
    long long RedisManager::incr(const std::string& key)
    {
        MmutexType::lockSco lock(m_mutex);
        redisReply* reply = (redisReply*)redisCommand(m_redis, "INCR %s", key.c_str());
        if (!reply || reply->type != REDIS_REPLY_INTEGER)
        {
            if (reply) freeReplyObject(reply);
            return -1;
        }
        long long val = reply->integer;
        freeReplyObject(reply);
        return val;
    }
    bool RedisManager::expire(const std::string& key, int seconds)
    {
        MmutexType::lockSco lock(m_mutex);
        redisReply* reply = (redisReply*)redisCommand(m_redis, "EXPIRE %s %d",
                                                    key.c_str(), seconds);
        if (!reply) return false;
        bool ok = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return ok;
    }
    bool RedisManager::_connect(const std::string& host, uint16_t port, const std::string& password)
    {
        m_redis = redisConnect(host.c_str(),port);
        if (!m_redis || m_redis->err)
        {
            BLUE_LOG_ERROR(g_logger) << "Redis connect failed: "
                                 << (m_redis ? m_redis->errstr : "unknown");
            if (m_redis) redisFree(m_redis);
            m_redis = nullptr;
            return false;
        }
        if (!password.empty())
        {
            auto reply = (redisReply*)redisCommand(m_redis, "AUTH %s", password.c_str());
            if (!reply || reply->type == REDIS_REPLY_ERROR)
            {
                BLUE_LOG_ERROR(g_logger) << "Redis auth failed";
                if (reply) freeReplyObject(reply);
                redisFree(m_redis);
                m_redis = nullptr;
                return false;
            }
            freeReplyObject(reply);
        }
        BLUE_LOG_INFO(g_logger) << "Redis connected: " << host << ":" << port;
        return true;
    }
}