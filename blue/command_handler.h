#pragma once
#include <memory>
#include <regex>
#include <chrono>
#include <list>
#include <unordered_set>
#include "config.h"
#include "task.h"
#include "tcpServer.h"
#include "resp_parser.h"
#include "asyncio.h"
#include "await.h"
#include "skiplist.h"

namespace blue
{
    // redis-cli admin
    static blue::ConfigVar<std::string>::ConfigVarPtr g_admin_password =
        blue::Config::Lookup<std::string>("admin.password", "admin123", "admin password");

    static std::string s_admin_password = "";
    struct __AdminIniter__
    {
        __AdminIniter__()
        {
            s_admin_password = g_admin_password->getValue();
            g_admin_password->addListener([](const std::string &old_val, const std::string &new_val)
                                          { s_admin_password = new_val; });
        }
    };
    using TimePoint = std::chrono::steady_clock::time_point;
    constexpr int SHARD_COUNT = 32;
    // 单个分片的结构
    struct DataShard
    {
        std::shared_mutex mutex;
        std::unordered_map<std::string, std::string> store;
        std::unordered_map<std::string, TimePoint> expire;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash;
        std::unordered_map<std::string, std::list<std::string>> lists;
        std::unordered_map<std::string, std::unordered_set<std::string>> sets;
        // key -> 跳表(ZSetKey{score + member} -> member)
        std::unordered_map<std::string, SkipList<ZSetKey, std::string>> zset;
        // key -> (member-> score)
        std::unordered_map<std::string, std::unordered_map<std::string, double>> zset_score;
    };
    template <typename T>
    class CommandHandler : public TcpServer<T>
    {
    public:
        using CommandHandlerPtr = std::shared_ptr<CommandHandler>;
        using SteadyClock = std::chrono::steady_clock;
        // using TimePoint = SteadyClock::time_point;

    public:
        CommandHandler(int level = -1, int option_name = -1, T option = T(), IOManager *manager = IOManager::GetThis(),
                       IOManager *acceptmanager = IOManager::GetThis());

        ~CommandHandler();
        RespValue execute(std::vector<RespValue> args);

    protected:
        /**
         * @brief 处理client事件
         * @param sock 客户端 socket fd
         */
        virtual Task<void> handleClient(MSocket::MSocketPtr sock) override;

    private:
        Task<void> expireTime();
        void removeExpireCycle();
        std::string format_score(double score)
        {
            std::string s = std::to_string(score);
            // 去掉末尾多余的0
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            // 去掉可能的小数点
            if (!s.empty() && s.back() == '.')
            {
                s.pop_back();
            }
            return s;
        }

        // 获取 key 对应的分片
        int getShardIndex(const std::string &key) const
        {
            return std::hash<std::string>{}(key) % SHARD_COUNT;
        }

        // 获取分片引用
        DataShard &getShard(const std::string &key)
        {
            return m_shards[getShardIndex(key)];
        }

        const DataShard &getShard(const std::string &key) const
        {
            return m_shards[getShardIndex(key)];
        }

        // 持久化到文件
        void saveToFile()
        {
            const std::string filename = "dump.rdb";
            std::ofstream file(filename, std::ios::binary);

            if (!file)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Failed to open " << filename;
                return;
            }
            for (auto &shard : m_shards)
            {
                std::shared_lock lock(shard.mutex);

                for (auto &[key, value] : shard.store)
                {
                    file << "STR|" << key << "|" << value;

                    auto it = shard.expire.find(key);
                    if (it != shard.expire.end())
                    {
                        auto expire_time = it->second.time_since_epoch().count();
                        file << "|" << expire_time;
                    }
                    file << "\n";
                }

                for (auto &[key, fields] : shard.hash)
                {
                    for (auto &[field, value] : fields)
                    {
                        file << "HASH|" << key << "|" << field << "|" << value << "\n";
                    }
                }

                for (auto &[key, list] : shard.lists)
                {
                    for (auto &value : list)
                    {
                        file << "LIST|" << key << "|" << value << "\n";
                    }
                }

                for (auto &[key, set] : shard.sets)
                {
                    for (auto &member : set)
                    {
                        file << "SET|" << key << "|" << member << "\n";
                    }
                }

                for (auto &[key, zset] : shard.zset_score)
                {
                    for (auto &[member, score] : zset)
                    {
                        file << "ZSET|" << key << "|" << score << "|" << member << "\n";
                    }
                }
            }

            BLUE_LOG_INFO(xx::g_logger) << "RDB saved to " << filename;
        }

        // 从文件加载
        void loadFromFile()
        {
            const std::string filename = "dump.rdb";
            std::ifstream file(filename);

            if (!file)
            {
                BLUE_LOG_INFO(xx::g_logger) << "No existing RDB file";
                return;
            }

            std::string line;
            while (std::getline(file, line))
            {
                std::vector<std::string> parts;
                size_t pos = 0;
                std::string token;

                while ((pos = line.find('|')) != std::string::npos)
                {
                    token = line.substr(0, pos);
                    parts.push_back(token);
                    line.erase(0, pos + 1);
                }
                parts.push_back(line); // 最后一部分

                if (parts.empty())
                    continue;

                if (parts[0] == "STR" && parts.size() >= 3)
                {
                    auto &shard = getShard(parts[1]);
                    std::unique_lock lock(shard.mutex);
                    shard.store[parts[1]] = parts[2];

                    if (parts.size() >= 4)
                    {
                        int64_t expire_time = std::stoll(parts[3]);
                        shard.expire[parts[1]] = TimePoint(std::chrono::nanoseconds(expire_time));
                    }
                }
                else if (parts[0] == "HASH" && parts.size() >= 4)
                {
                    auto &shard = getShard(parts[1]);
                    std::unique_lock lock(shard.mutex);
                    shard.hash[parts[1]][parts[2]] = parts[3];
                }
                else if (parts[0] == "LIST" && parts.size() >= 3)
                {
                    auto &shard = getShard(parts[1]);
                    std::unique_lock lock(shard.mutex);
                    shard.lists[parts[1]].push_back(parts[2]);
                }
                else if (parts[0] == "SET" && parts.size() >= 3)
                {
                    auto &shard = getShard(parts[1]);
                    std::unique_lock lock(shard.mutex);
                    shard.sets[parts[1]].insert(parts[2]);
                }
                else if (parts[0] == "ZSET" && parts.size() >= 4)
                {
                    auto &shard = getShard(parts[1]);
                    std::unique_lock lock(shard.mutex);
                    double score = std::stod(parts[2]);
                    std::string member = parts[3];
                    shard.zset_score[parts[1]][member] = score;
                    shard.zset[parts[1]].insert({score, member}, member);
                }
            }
            BLUE_LOG_INFO(xx::g_logger) << "RDB loaded from " << filename;
        }

    private:
        // commandserver 配置
        struct CommConfig
        {
            std::string requirepassword; // 认证密码
            int32_t maxClients;          // 最大客户端数量
            int32_t timeout_s;           // 客户端超时(s)
            std::string save;            // 保存策略
        } m_config;

        static CommConfig s_commConfig;

    private:
        std::array<DataShard, SHARD_COUNT> m_shards;
        std::atomic<uint32_t> m_commands{0};
        std::atomic<bool> m_shutdown{false};
        std::atomic<bool> m_authenticated{false};
        std::string m_password = "";
    };

    template <typename T>
    CommandHandler<T>::CommandHandler(int level, int option_name, T option, IOManager *manager,
                                      IOManager *acceptmanager)
        : TcpServer<T>(level, option_name, option, manager, acceptmanager)
    {
        loadFromFile();
        IOManager::GetThis()->schedule(expireTime());
        if (s_admin_password.empty())
        {
            s_admin_password = "admin123";
        }
        m_password = s_admin_password;
        m_config.requirepassword = "admin123";
        m_config.maxClients = 1000;
        m_config.timeout_s = 0;
    }

    template <typename T>
    CommandHandler<T>::~CommandHandler()
    {
        saveToFile();
    }

    template <typename T>
    void CommandHandler<T>::removeExpireCycle()
    {
        int count = 0;
        auto now = SteadyClock::now();
        for (auto &shards : m_shards)
        {
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.expire.begin();
            while (it != shards.expire.end() && count < 20)
            {
                if (it->second < now)
                {
                    shards.store.erase(it->first);
                    it = shards.expire.erase(it);
                }
                else
                {
                    it++;
                }
                ++count;
            }
        }
    }

    template <typename T>
    Task<void> CommandHandler<T>::expireTime()
    {
        while (true)
        {
            co_await sleepFor(1);
            removeExpireCycle();
        }
    }

    template <typename T>
    RespValue CommandHandler<T>::execute(std::vector<RespValue> args)
    {
        if (args.empty())
        {
            return RespValue::error("ERR empty command");
        }

        std::string cmd = args[0].str;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "PING")
        {
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'PING'");
            }
            return RespValue::simple_string("PONG");
        }
        else if (cmd == "AUTH") // AUTH password
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'AUTH'");
            }
            if (args[1].str == m_password)
            {
                m_authenticated.store(true, std::memory_order_release);
                return RespValue::simple_string("OK");
            }
            return RespValue::error("ERR invalid password");
        }
        else if (cmd == "CONFIG") // CONFIG (GET [...])/SET [...], 获取给客户端的配置信息
        {
            if (args.size() < 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'CONFIG'");
            }
            std::string subcmd = args[1].str;
            std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
            if (subcmd == "GET")
            {
                if (args.size() != 3)
                {
                    return RespValue::error("ERR wrong number of arguments for 'CONFIG GET'");
                }
                std::string pattern = args[2].str;
                std::vector<RespValue> result;

                if (pattern == "*" || pattern == "requirepass")
                {
                    result.push_back(RespValue::bulk_string("requirepass"));
                    result.push_back(RespValue::bulk_string(m_config.requirepassword));
                }
                if (pattern == "*" || pattern == "maxclients")
                {
                    result.push_back(RespValue::bulk_string("maxclients"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_config.maxClients)));
                }
                if (pattern == "*" || pattern == "timeout")
                {
                    result.push_back(RespValue::bulk_string("timeout"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_config.timeout_s)));
                }
                return RespValue::array(std::move(result));
            }
            else if (subcmd == "SET")
            {
                if (args.size() != 4)
                {
                    return RespValue::error("ERR wrong number of arguments for 'CONFIG SET'");
                }

                std::string param = args[2].str;
                std::string value = args[3].str;

                if (param == "requirepass")
                {
                    m_config.requirepassword = value;
                    m_password = value; // 同步更新认证密码
                    return RespValue::simple_string("OK");
                }
                else if (param == "maxclients")
                {
                    try
                    {
                        m_config.maxClients = std::stoi(value);
                        return RespValue::simple_string("OK");
                    }
                    catch (...)
                    {
                        return RespValue::error("ERR invalid integer value");
                    }
                }
                else if (param == "timeout")
                {
                    try
                    {
                        m_config.timeout_s = std::stoi(value);
                        return RespValue::simple_string("OK");
                    }
                    catch (...)
                    {
                        return RespValue::error("ERR invalid integer value");
                    }
                }

                return RespValue::error("ERR Unsupported CONFIG parameter: " + param);
            }
            return RespValue::error("ERR wrong arguments for 'CONFIG'");
        } // string
        else if (cmd == "SET") // SET key val [EX [s]]/[PX [ms]]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'SET'");
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            shards.store[key] = val;
            if (args.size() >= 5)
            {
                std::string subcmd = args[3].str;
                std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
                if (subcmd == "EX")
                {
                    int64_t seconds;
                    try
                    {
                        seconds = std::stoll(args[4].str);
                        if (seconds < 0)
                        {
                            return RespValue::error("ERR value can't be nagative");
                        }
                    }
                    catch (...)
                    {
                        return RespValue::error("ERR value is not a integer or out of range");
                    }
                    shards.expire[key] = SteadyClock::now() + std::chrono::seconds(seconds);
                }
                else if (subcmd == "PX")
                {
                    int64_t milliseconds;
                    try
                    {
                        milliseconds = std::stoll(args[4].str);
                        if (milliseconds < 0)
                        {
                            return RespValue::error("ERR value can't be nagative");
                        }
                    }
                    catch (...)
                    {
                        return RespValue::error("ERR value is not a integer or out of range");
                    }
                    shards.expire[key] = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
                }
            }
            return RespValue::simple_string("OK");
        }
        else if (cmd == "GET") // GET key
        {
            if (args.size() < 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'GET'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return RespValue::null_bulk();
            }
            auto expire_it = shards.expire.find(key);
            if (expire_it != shards.expire.end() && expire_it->second < SteadyClock::now())
            {
                shards.expire.erase(expire_it);
                shards.store.erase(it);
                return RespValue::null_bulk();
            }
            const std::string val = it->second;
            return RespValue::bulk_string(val);
        }
        else if (cmd == "MSET") // MSET key val [key val]
        {
            if (args.size() < 3 || (args.size() & 1) != 1)
            {
                return RespValue::error("ERR wrong of arguments for 'MSET'");
            }

            for (size_t i = 1; i < args.size(); i += 2)
            {
                const std::string key = args[i].str;
                const std::string val = args[i + 1].str;
                auto &shards = getShard(key);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                shards.store[key] = val;
            }
            return RespValue::simple_string("OK");
        }
        else if (cmd == "MGET") // MGET key [key...]
        {
            if (args.size() < 2)
            {
                return RespValue::error("ERR wrong of arguments for 'MGET'");
            }
            std::vector<RespValue> results;

            for (size_t i = 1; i < args.size(); i++)
            {
                auto &shards = getShard(args[i].str);
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                auto it = shards.store.find(args[i].str);
                if (it == shards.store.end())
                {
                    results.push_back(RespValue::null_bulk());
                }
                else
                {
                    auto expire_it = shards.expire.find(args[i].str);
                    if (expire_it != shards.expire.end() && expire_it->second < SteadyClock::now())
                    {
                        shards.expire.erase(expire_it);
                        shards.store.erase(it);
                        results.push_back(RespValue::null_bulk());
                    }
                    else
                    {
                        const std::string val = it->second;
                        results.push_back(RespValue::bulk_string(val));
                    }
                }
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "APPEND") // APPEND key val
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong of arguments for 'APPEND'");
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = val;
            }
            else
            {
                it->second.append(val);
            }
            return RespValue::integer(shards.store[key].size());
        }
        else if (cmd == "SETNX") // SETNX key val, 没有才设置
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong of arguments for 'SETNX'");
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = val;
                return RespValue::integer(1);
            }
            return RespValue::integer(0);
        }
        else if (cmd == "EXISTS") // EXISTS key [key...]
        {
            if (args.size() < 2)
            {
                return RespValue::error("ERR wrong of arguments for 'EXISTS'");
            }
            int64_t count = 0;
            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string key = args[i].str;
                auto &shards = getShard(key);
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                if (shards.store.find(key) != shards.store.end() ||
                    shards.hash.find(key) != shards.hash.end() ||
                    shards.lists.find(key) != shards.lists.end() ||
                    shards.sets.find(key) != shards.sets.end() ||
                    shards.zset.find(key) != shards.zset.end())
                {
                    count++;
                }
            }
            return RespValue::integer(count);
        }
        else if (cmd == "DEL") // DEL key [key...]
        {
            if (args.size() < 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'DEL'");
            }
            int count = 0;
            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string key = args[i].str;
                auto &shards = getShard(key);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto it = shards.store.find(key);
                if (it != shards.store.end())
                {
                    shards.store.erase(it);
                    shards.expire.erase(key);
                    count++;
                }
            }
            return RespValue::integer(count);
        }
        else if (cmd == "HSET") // HSET key field value [field value ...]
        {
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return RespValue::error("ERR wrong number of arguments for 'HSET'");
            }
            int count = 0;
            std::string key = args[1].str;
            for (size_t i = 2; i < args.size(); i += 2)
            {
                auto &shards = getShard(key);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto &field = args[i].str;
                auto &value = args[i + 1].str; // size 是偶数所以不会出界
                if (shards.hash[key].find(field) == shards.hash[key].end())
                {
                    ++count; // 新字段
                }
                shards.hash[key][field] = value;
            }
            return RespValue::integer(count);
        } // 哈希集合操作
        else if (cmd == "HGET") // HGET key field
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'HGET'");
            }
            const std::string key = args[1].str;
            const std::string field = args[2].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return RespValue::null_bulk();
            }
            auto fit = it->second.find(field);
            if (fit == it->second.end())
            {
                return RespValue::null_bulk();
            }
            const std::string val = fit->second;
            return RespValue::bulk_string(val);
        }
        else if (cmd == "HGETALL") // HGETALL key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'HGETALL'");
            }
            std::vector<RespValue> result;
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return RespValue::array(std::move(result));
            }
            for (auto &[field, value] : it->second)
            {
                result.push_back(RespValue::bulk_string(field));
                result.push_back(RespValue::bulk_string(value));
            }
            return RespValue::array(std::move(result));
        }
        else if (cmd == "HDEL") // HDEL key field [field ...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'HDEL'");
            }
            int count = 0;
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return RespValue::integer(count);
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                count += it->second.erase(args[i].str);
            }
            if (it->second.empty())
            {
                shards.hash.erase(it);
            }
            return RespValue::integer(count);
        }
        else if (cmd == "HLEN") // HLEN key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'HLEN'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> rdlock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }
                return RespValue::integer(0);
            }
            return RespValue::integer(it->second.size());
        }
        else if (cmd == "HEXISTS") // HEXISTS key field
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'HEXISTS'");
            }
            const std::string key = args[1].str;
            const std::string field = args[2].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }
                return RespValue::integer(0);
            }
            auto filed_it = it->second.find(field);
            if (filed_it == it->second.end())
            {
                return RespValue::integer(0);
            }
            return RespValue::integer(1);
        }
        else if (cmd == "HKEYS") // HKEYS key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'HKEYS'");
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::array(std::move(results));
            }
            for (auto &[field, _] : it->second)
            {
                results.push_back(RespValue::bulk_string(field));
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "HVALS") // HVALS key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'HKEYS'");
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::array(std::move(results));
            }
            for (auto &[_, val] : it->second)
            {
                results.push_back(RespValue::bulk_string(val));
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "KEYS") // KEYS *
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'KEYS'");
            }

            // 把 glob 风格转成 regex
            std::string pattern = args[1].str;
            std::string regex_str;
            for (char c : pattern)
            {
                if (c == '*')
                {
                    regex_str += ".*";
                }
                else if (c == '?')
                {
                    regex_str += ".";
                }
                else if (c == '.' || c == '+' || c == '[' || c == ']' ||
                         c == '(' || c == ')' || c == '\\')
                {
                    regex_str += '\\';
                    regex_str += c;
                }
                else
                {
                    regex_str += c;
                }
            }

            constexpr size_t MAX_KEYS = 10000;
            std::regex re(regex_str);
            std::vector<RespValue> result;

            // 遍历所有分片
            for (auto &shard : m_shards)
            {
                if (result.size() >= MAX_KEYS)
                {
                    break;
                }
                std::shared_lock<std::shared_mutex> lock(shard.mutex);

                for (auto &[key, value] : shard.store)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // Hash 类型的 key
                for (auto &[key, fields] : shard.hash)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // List 类型的 key
                for (auto &[key, list] : shard.lists)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // Set 类型的 key
                for (auto &[key, set] : shard.sets)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // ZSet 类型的 key
                for (auto &[key, zset] : shard.zset)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
            }

            return RespValue::array(std::move(result));
        } // 链表操作
        else if (cmd == "LPUSH") // LPUSH key val [val...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'LPUSH'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &lhs = shards.lists[key];
            for (size_t i = 2; i < args.size(); i++)
            {
                lhs.push_front(args[i].str);
            }
            return RespValue::integer(lhs.size());
        }
        else if (cmd == "RPUSH") // RPUSH key val [val...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'RPUSH'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &lhs = shards.lists[key];
            for (size_t i = 2; i < args.size(); i++)
            {
                lhs.push_back(args[i].str);
            }
            return RespValue::integer(lhs.size());
        }
        else if (cmd == "LPOP") // LPOP key [count]
        {
            if (args.size() < 2 || args.size() > 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'LPOP'");
            }
            int count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return RespValue::error("ERR count can't be nagative");
                }
            }
            catch (...)
            {
                return RespValue::error("ERR value is not a integer or out of range");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end() || it->second.empty())
            {
                return RespValue::null_bulk();
            }
            std::vector<RespValue> results;
            for (int i = 0; i < count && !it->second.empty(); i++)
            {
                results.push_back(RespValue::bulk_string(it->second.front()));
                it->second.pop_front();
            }
            if (it->second.empty())
            {
                shards.lists.erase(it);
            }
            if (results.size() == 1 && args.size() == 2)
            {
                return results[0];
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "RPOP") // RPOP key [count]
        {
            if (args.size() < 2 || args.size() > 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'LPOP'");
            }
            int count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return RespValue::error("ERR count can't be nagative");
                }
            }
            catch (...)
            {
                return RespValue::error("ERR value is not a integer or out of range");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end() || it->second.empty())
            {
                return RespValue::null_bulk();
            }
            std::vector<RespValue> results;
            for (int i = 0; i < count && !it->second.empty(); i++)
            {
                results.push_back(RespValue::bulk_string(it->second.back()));
                it->second.pop_back();
            }
            if (it->second.empty())
            {
                shards.lists.erase(it);
            }
            if (results.size() == 1 && args.size() == 2)
            {
                return results[0];
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "LRANGE") // LRANGE key start stop
        {
            if (args.size() != 4)
            {
                return RespValue::error("ERR wrong number of arguments for 'LRANGE'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            int start, stop;
            try
            {
                start = std::stoi(args[2].str);
                stop = std::stoi(args[3].str);
            }
            catch (...)
            {
                return RespValue::error("ERR value is not a integer or out of range");
            }
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end())
            {
                return RespValue::array({});
            }
            int size = it->second.size();
            if (start < 0)
            {
                start += size; // 尽量偏移到正数
            }
            if (stop < 0)
            {
                stop += size;
            }
            if (start < 0) // 还是小于0就从0开始
            {
                start = 0;
            }
            if (stop >= size)
            {
                stop = size - 1;
            }
            if (start > stop)
            {
                return RespValue::array({});
            }
            std::vector<RespValue> result;
            auto iter = it->second.begin();
            std::advance(iter, start);
            for (int i = start; i <= stop && iter != it->second.end(); i++, iter++)
            {
                result.push_back(RespValue::bulk_string(*iter));
            }
            return RespValue::array(std::move(result));
        } // 有序集合操作
        else if (cmd == "ZADD") // ZADD key score member [score member]
        {
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return RespValue::error("ERR wrong number of arguments for 'ZADD'");
            }
            int count = 0;
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);

            shards.zset_score.try_emplace(key);
            shards.zset.try_emplace(key);
            auto &score_map = shards.zset_score[key];
            auto &skiplist = shards.zset[key];

            for (size_t i = 2; i < args.size(); i += 2)
            {
                double score;
                try
                {
                    score = std::stod(args[i].str);
                }
                catch (const std::exception &e)
                {
                    return RespValue::error("ERR value is not a double or out of range");
                }
                std::string member = args[i + 1].str;

                auto it = score_map.find(member);
                if (it != score_map.end())
                {
                    // 已存在，删掉旧的
                    ZSetKey old_key(it->second, it->first);
                    skiplist.remove(old_key);
                }
                else
                {
                    count++;
                }
                score_map[member] = score;
                ZSetKey newkey(score, member);
                skiplist.insert(newkey, member);
            }
            return RespValue::integer(count);
        }
        else if (cmd == "ZRANGE") // ZRANGE key start stop [WITHSCORES]
        {
            if (args.size() < 4 || args.size() > 5)
            {
                return RespValue::error("ERR wrong number of arguments for 'ZRANGE'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            int start, stop;
            try
            {
                start = std::stoi(args[2].str);
                stop = std::stoi(args[3].str);
            }
            catch (...)
            {
                return RespValue::error("ERR value is not a integer or out of range");
            }
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset.find(key);
            if (it == shards.zset.end())
            {
                return RespValue::array({});
            }
            bool withscores = (args.size() == 5 && args[4].str == "WITHSCORES");
            auto &skiplist_map = it->second;
            int size = skiplist_map.size();
            if (start < 0)
            {
                start += size;
            }
            if (stop < 0)
            {
                stop += size;
            }
            if (start < 0)
            {
                start = 0;
            }
            if (stop >= size)
            {
                stop = size - 1;
            }
            if (start > stop)
            {
                return RespValue::array({});
            }
            std::vector<RespValue> results;
            for (int i = start; i <= stop; i++)
            {
                auto *node = skiplist_map.getByIndex(i);
                if (!node)
                {
                    break;
                }
                results.push_back(RespValue::bulk_string(node->val));
                if (withscores)
                {
                    results.push_back(RespValue::bulk_string(format_score(node->key.score)));
                }
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "ZREM") // ZREM key member [member...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'ZREM'");
            }
            int count = 0;
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset.find(key);
            if (it == shards.zset.end())
            {
                return RespValue::integer(count);
            }
            auto sit = shards.zset_score.find(key);
            if (sit != shards.zset_score.end())
            {
                auto &skiplist_map = it->second;
                auto &scores = sit->second;

                for (size_t i = 2; i < args.size(); i++)
                {
                    const std::string member = args[i].str;
                    auto member_score = scores.find(member);
                    if (member_score != scores.end())
                    {
                        // key = {score,member}
                        skiplist_map.remove({member_score->second, member_score->first});
                        scores.erase(member_score);
                        count++;
                    }
                }
                if (skiplist_map.empty())
                {
                    shards.zset.erase(it);
                    shards.zset_score.erase(key);
                }
            }
            return RespValue::integer(count);
        }
        else if (cmd == "ZSCORE") // // ZSCORE key member
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'ASCORE'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset_score.find(key);
            if (it == shards.zset_score.end())
            {
                return RespValue::null_bulk();
            }
            auto &skiplist_map = it->second;
            auto sit = skiplist_map.find(args[2].str);
            if (sit != skiplist_map.end())
            {
                return RespValue::bulk_string(format_score(sit->second));
            }
            return RespValue::null_bulk();
        }
        else if (cmd == "ZRANK") // ZRANK key member
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'ZRANK'");
            }
            const std::string key = args[1].str, member = args[2].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset_score.find(key);
            if (it == shards.zset_score.end())
            {
                return RespValue::null_bulk();
            }
            auto &score_map = it->second;
            auto sit = score_map.find(member);
            if (sit != score_map.end())
            {
                // key  = {score,member}
                int rank = shards.zset[key].getRank({sit->second, member});
                if (rank >= 0)
                {
                    return RespValue::integer(rank);
                }
            }
            return RespValue::null_bulk();
        }
        else if (cmd == "INCR") // INCR key 自增
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'INCR'");
            }
            const std::string key = args[1].str;
            int64_t val = 0;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> rdlock(shards.mutex);
            auto it = shards.store.find(key);
            if (it != shards.store.end())
            {
                try
                {
                    val = std::stoll(it->second);
                }
                catch (...)
                {
                    return RespValue::error("ERR value is not a integer or out of range");
                }
            }
            val++;
            shards.store[key] = std::to_string(val);
            return RespValue::integer(val);
        }
        else if (cmd == "INCRBY") // INCR key integer 自增或自减 integer
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'INCRBY'");
            }
            const std::string key = args[1].str;
            int64_t increment;
            try
            {
                increment = std::stoll(args[2].str);
            }
            catch (const std::exception &e)
            {
                return RespValue::error("ERR value is not an integer or out of range");
            }
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            int64_t val = 0;
            if (it != shards.store.end())
            {
                try
                {
                    val = std::stoll(it->second);
                }
                catch (...)
                {
                    return RespValue::error("ERR value is not a integer or out of range");
                }
            }
            val += increment;
            shards.store[key] = std::to_string(val);
            return RespValue::integer(val);
        }
        else if (cmd == "STRLEN") // STRLEN key, 返回key对于val的长度
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'STRLEN'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                if (shards.hash.find(key) != shards.hash.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::integer(0);
            }
            auto expire_it = shards.expire.find(key);
            if (expire_it != shards.expire.end() && expire_it->second < SteadyClock::now())
            {
                shards.expire.erase(expire_it);
                shards.store.erase(it);
                return RespValue::integer(0);
            }
            return RespValue::integer(it->second.size());
        }
        else if (cmd == "TYPE") // TYPE key, key的类型
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'TYPE'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            if (shards.store.find(key) != shards.store.end())
            {
                return RespValue::bulk_string("string");
            }
            if (shards.hash.find(key) != shards.hash.end())
            {
                return RespValue::bulk_string("hash");
            }
            if (shards.lists.find(key) != shards.lists.end())
            {
                return RespValue::bulk_string("list");
            }
            if (shards.zset.find(key) != shards.zset.end())
            {
                return RespValue::bulk_string("zset");
            }
            if (shards.sets.find(key) != shards.sets.end())
            {
                return RespValue::bulk_string("set");
            }
            return RespValue::null_bulk();
        } // 无序集合操作
        else if (cmd == "SADD") // SADD key member [member...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of argument for 'SADD'");
            }
            const std::string key = args[1].str;
            int32_t count = 0;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string member = args[i].str;
                if (shards.sets[key].find(member) == shards.sets[key].end())
                {
                    auto [_, res] = shards.sets[key].insert(member);
                    if (res)
                    {
                        count++;
                    }
                }
            }
            return RespValue::integer(count);
        }
        else if (cmd == "SMEMBERS") // SMEMBERS key, 返回key所有的成员
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'SMEMBERS'");
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return RespValue::array(std::move(results));
            }
            for (auto &member : it->second)
            {
                results.push_back(RespValue::bulk_string(member));
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "SREM") // SREM key member [member...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'SREM'");
            }
            const std::string key = args[1].str;
            int32_t count = 0;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return RespValue::integer(0);
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                count += it->second.erase(args[i].str);
            }
            if (it->second.empty())
            {
                shards.sets.erase(it);
            }
            return RespValue::integer(count);
        }
        else if (cmd == "SISMEMBER") // SISMEMBER key member
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR number wrong of arguments for 'SISMEMBER'");
            }
            const std::string key = args[1].str;
            const std::string member = args[2].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return RespValue::integer(0);
            }
            return RespValue::integer(it->second.count(member));
        }
        else if (cmd == "SCARD") // SCARD key, key的集合大小
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR number wrong of arguments for 'SCARD'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return RespValue::integer(0);
            }
            return RespValue::integer(it->second.size());
        }
        else if (cmd == "SRANDMEMBER") // SRANDMEMBER key [count], 随机返回count个member,count < 0可包含重复值, > 0不重复
        {
            if (args.size() < 2 && args.size() > 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'SRANDMEMBER'");
            }
            int32_t count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 0;
            }
            catch (...)
            {
                return RespValue::error("ERR value is not an integer or out of range");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                if (args.size() == 2)
                {
                    return RespValue::null_bulk();
                }
                else
                {
                    return RespValue::array({});
                }
            }
            const auto &set = it->second;
            std::vector<std::string> members(set.begin(), set.end());
            std::vector<RespValue> results;
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                return RespValue::bulk_string(members[idx]);
            }

            if (count >= 0)
            {
                // 正数：不重复
                int num = std::min(count, (int32_t)(members.size()));
                std::shuffle(members.begin(), members.end(), std::mt19937(std::random_device()()));
                for (int i = 0; i < num; i++)
                {
                    results.push_back(RespValue::bulk_string(members[i]));
                }
            }
            else
            {
                // 负数：可重复
                int num = -count;
                for (int i = 0; i < num; i++)
                {
                    int idx = rand() % members.size();
                    results.push_back(RespValue::bulk_string(members[idx]));
                }
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "SPOP") // SPOP key [count], 随机返回并删除count个member,count只能大于0
        {
            if (args.size() < 2 && args.size() > 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'SRANDMEMBER'");
            }
            int32_t count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 0;
                if (count < 0)
                {
                    return RespValue::error("ERR value can't be nagative");
                }
            }
            catch (...)
            {
                return RespValue::error("ERR value is not an integer or out of range");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                if (args.size() == 2)
                {
                    return RespValue::null_bulk();
                }
                else
                {
                    return RespValue::array({});
                }
            }
            auto &set = it->second;
            std::vector<std::string> members(set.begin(), set.end());
            std::vector<RespValue> results;
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                set.erase(members[idx]);
                return RespValue::bulk_string(members[idx]);
            }

            if (count >= 0)
            {
                // 正数：不重复
                int num = std::min(count, (int32_t)(members.size()));
                std::shuffle(members.begin(), members.end(), std::mt19937(std::random_device()()));
                for (int i = 0; i < num; i++)
                {
                    set.erase(members[i]);
                    results.push_back(RespValue::bulk_string(members[i]));
                }
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "FLUSHDB") // FLUSHDB, 清空数据库(还没有持久化)
        {
            if (!m_authenticated.load(std::memory_order_acquire))
            {
                return RespValue::error("ERR permission denied");
            }
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'FLUSHDB'");
            }
            for (auto &shards : m_shards)
            {
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                shards.store.clear();
                shards.expire.clear();
                shards.lists.clear();
                shards.hash.clear();
                shards.sets.clear();
                shards.zset.clear();
                shards.zset_score.clear();
            }
            return RespValue::simple_string("OK");
        }
        else if (cmd == "DBSIZE") // DBSIZE, 当前数据库大小
        {
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'DBSIZE'");
            }
            int64_t count = 0;
            for (auto &shards : m_shards)
            {
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                count += shards.store.size() + shards.lists.size() + shards.hash.size() + shards.zset.size();
            }
            return RespValue::integer(count);
        }
        else if (cmd == "EXPIRE") // EXPIRE key seconds, 设置key的过期时间
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'EXPIRE'");
            }
            int64_t second;
            try
            {
                second = std::stoll(args[2].str);
                if (second <= 0)
                {
                    return RespValue::error("ERR invalid expire time");
                }
            }
            catch (...)
            {
                return RespValue::error("ERR value is not an integer or out of range");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return RespValue::integer(0);
            }
            shards.expire[key] = SteadyClock::now() + std::chrono::seconds(second);
            return RespValue::integer(1);
        }
        else if (cmd == "TTL") // TTL key, 查看key的过期时间
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'TTL'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            auto expire_it = shards.expire.find(key);
            if (it == shards.store.end())
            {
                return RespValue::integer(-2);
            }
            else if (expire_it == shards.expire.end())
            {
                return RespValue::integer(-1);
            }
            // 计算剩余秒数
            auto now = SteadyClock::now();
            if (now >= expire_it->second)
            {
                return RespValue::integer(-2);
            }

            auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                                 expire_it->second - now)
                                 .count();

            return RespValue::integer(remaining);
        }
        else if (cmd == "PEXPIRE") // PEXPIRE key milliseconds
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'PEXPIRE'");
            }
            int64_t milliseconds;
            try
            {
                milliseconds = std::stoll(args[2].str);
                if (milliseconds <= 0)
                {
                    return RespValue::error("ERR invalid expire time");
                }
            }
            catch (...)
            {
                return RespValue::error("ERR value is not an integer or out of range");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return RespValue::integer(0);
            }
            shards.expire[key] = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
            return RespValue::integer(1);
        }
        else if (cmd == "PTTL") // PTTL key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'PTTL'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            auto expire_it = shards.expire.find(key);
            if (it == shards.store.end())
            {
                return RespValue::integer(-2);
            }
            else if (expire_it == shards.expire.end())
            {
                return RespValue::integer(-1);
            }
            // 计算剩余秒数
            auto now = SteadyClock::now();
            if (now >= expire_it->second)
            {
                return RespValue::integer(-2);
            }

            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 expire_it->second - now)
                                 .count();

            return RespValue::integer(remaining);
        }
        else if (cmd == "PERSIST") // PERSIST key, 撤销key的过期时间
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of argument for 'PERSIST'");
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            auto expire_it = shards.expire.find(key);
            // 不存在或没有过期时间
            if (it == shards.store.end() || expire_it == shards.expire.end())
            {
                return RespValue::integer(0);
            }
            shards.expire.erase(expire_it);
            return RespValue::integer(1);
        }
        else if (cmd == "RENAME") // RENAME key newkey
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'RENAME'");
            }
            const std::string key = args[1].str;
            const std::string newkey = args[2].str;

            if (key == newkey)
            {
                return RespValue::simple_string("OK");
            }

            int old_shard_idx = getShardIndex(key);
            int new_shard_idx = getShardIndex(newkey);

            // 按顺序锁，避免死锁
            int first = old_shard_idx;
            int second = new_shard_idx;
            if (first > second)
            {
                std::swap(first, second);
            }

            std::unique_lock<std::shared_mutex> lock1(m_shards[first].mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (old_shard_idx != new_shard_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(m_shards[second].mutex);
            }

            auto &old_shard = m_shards[old_shard_idx];
            auto &new_shard = m_shards[new_shard_idx];

            // 检查 key 是否存在
            bool exists = false;
            int type = -1; // 0:string, 1:hash, 2:list, 3:set, 4:zset

            if (old_shard.store.find(key) != old_shard.store.end())
            {
                exists = true;
                type = 0;
            }
            else if (old_shard.hash.find(key) != old_shard.hash.end())
            {
                exists = true;
                type = 1;
            }
            else if (old_shard.lists.find(key) != old_shard.lists.end())
            {
                exists = true;
                type = 2;
            }
            else if (old_shard.sets.find(key) != old_shard.sets.end())
            {
                exists = true;
                type = 3;
            }
            else if (old_shard.zset.find(key) != old_shard.zset.end())
            {
                exists = true;
                type = 4;
            }

            if (!exists)
            {
                return RespValue::error("ERR no such key");
            }

            // 删除 newkey（如果存在）
            if (new_shard.store.find(newkey) != new_shard.store.end())
            {
                new_shard.store.erase(newkey);
                new_shard.expire.erase(newkey);
            }
            else if (new_shard.hash.find(newkey) != new_shard.hash.end())
            {
                new_shard.hash.erase(newkey);
            }
            else if (new_shard.lists.find(newkey) != new_shard.lists.end())
            {
                new_shard.lists.erase(newkey);
            }
            else if (new_shard.sets.find(newkey) != new_shard.sets.end())
            {
                new_shard.sets.erase(newkey);
            }
            else if (new_shard.zset.find(newkey) != new_shard.zset.end())
            {
                new_shard.zset.erase(newkey);
                new_shard.zset_score.erase(newkey);
            }
            new_shard.expire.erase(newkey);

            // 移动数据
            if (old_shard_idx == new_shard_idx)
            {
                // 同分片：直接移动
                if (type == 0)
                {
                    new_shard.store[newkey] = std::move(old_shard.store[key]);
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = std::move(old_shard.hash[key]);
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = std::move(old_shard.lists[key]);
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = std::move(old_shard.sets[key]);
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                // 移动过期时间
                auto expire_it = old_shard.expire.find(key);
                if (expire_it != old_shard.expire.end())
                {
                    new_shard.expire[newkey] = expire_it->second;
                    old_shard.expire.erase(expire_it);
                }
            }
            else
            {
                // 跨分片：复制到新分片，删除旧分片
                if (type == 0)
                {
                    new_shard.store[newkey] = old_shard.store[key];
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = old_shard.hash[key];
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = old_shard.lists[key];
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = old_shard.sets[key];
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                auto expire_it = old_shard.expire.find(key);
                if (expire_it != old_shard.expire.end())
                {
                    new_shard.expire[newkey] = expire_it->second;
                    old_shard.expire.erase(expire_it);
                }
            }
            return RespValue::simple_string("OK");
        }
        else if (cmd == "RENAMENX") // RENAMENX key newkey 当newkey不存在时创建
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'RENAME'");
            }
            const std::string key = args[1].str;
            const std::string newkey = args[2].str;

            if (key == newkey)
            {
                return RespValue::integer(1);
            }

            int old_shard_idx = getShardIndex(key);
            int new_shard_idx = getShardIndex(newkey);

            // 按顺序锁，避免死锁
            int first = old_shard_idx;
            int second = new_shard_idx;
            if (first > second)
            {
                std::swap(first, second);
            }

            std::unique_lock<std::shared_mutex> lock1(m_shards[first].mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (old_shard_idx != new_shard_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(m_shards[second].mutex);
            }

            auto &old_shard = m_shards[old_shard_idx];
            auto &new_shard = m_shards[new_shard_idx];

            // 检查 key 是否存在
            bool exists = false;
            int type = -1; // 0:string, 1:hash, 2:list, 3:set, 4:zset

            if (old_shard.store.find(key) != old_shard.store.end())
            {
                exists = true;
                type = 0;
            }
            else if (old_shard.hash.find(key) != old_shard.hash.end())
            {
                exists = true;
                type = 1;
            }
            else if (old_shard.lists.find(key) != old_shard.lists.end())
            {
                exists = true;
                type = 2;
            }
            else if (old_shard.sets.find(key) != old_shard.sets.end())
            {
                exists = true;
                type = 3;
            }
            else if (old_shard.zset.find(key) != old_shard.zset.end())
            {
                exists = true;
                type = 4;
            }

            if (!exists)
            {
                return RespValue::error("ERR no such key");
            }

            // 查看newkey是否存在
            bool newkey_exists = false;
            if (new_shard.store.find(newkey) != new_shard.store.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.hash.find(newkey) != new_shard.hash.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.lists.find(newkey) != new_shard.lists.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.sets.find(newkey) != new_shard.sets.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.zset.find(newkey) != new_shard.zset.end())
            {
                newkey_exists = true;
            }
            if (newkey_exists)
            {
                return RespValue::integer(0);
            }

            // 移动数据
            if (old_shard_idx == new_shard_idx)
            {
                // 同分片：直接移动
                if (type == 0)
                {
                    new_shard.store[newkey] = std::move(old_shard.store[key]);
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = std::move(old_shard.hash[key]);
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = std::move(old_shard.lists[key]);
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = std::move(old_shard.sets[key]);
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                // 移动过期时间
                auto expire_it = old_shard.expire.find(key);
                if (expire_it != old_shard.expire.end())
                {
                    new_shard.expire[newkey] = expire_it->second;
                    old_shard.expire.erase(expire_it);
                }
            }
            else
            {
                // 跨分片：复制到新分片，删除旧分片
                if (type == 0)
                {
                    new_shard.store[newkey] = old_shard.store[key];
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = old_shard.hash[key];
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = old_shard.lists[key];
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = old_shard.sets[key];
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                auto expire_it = old_shard.expire.find(key);
                if (expire_it != old_shard.expire.end())
                {
                    new_shard.expire[newkey] = expire_it->second;
                    old_shard.expire.erase(expire_it);
                }
            }
            return RespValue::integer(1);
        }
        else if (cmd == "RANDOMKEY") // RANDOMKEY, 随机返回一个key
        {
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'RANDOMKEY'");
            }
            std::vector<std::string> all_keys;
            for (auto &shards : m_shards)
            {
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                // string
                for (auto &[key, _] : shards.store)
                {
                    all_keys.push_back(key);
                }
                // hash
                for (auto &[key, _] : shards.hash)
                {
                    all_keys.push_back(key);
                }
                // lists
                for (auto &[key, _] : shards.lists)
                {
                    all_keys.push_back(key);
                }
                // sets
                for (auto &[key, _] : shards.sets)
                {
                    all_keys.push_back(key);
                }
                // zset
                for (auto &[key, _] : shards.zset)
                {
                    all_keys.push_back(key);
                }
            }
            if (all_keys.size() == 0)
            {
                return RespValue::null_bulk();
            }
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, all_keys.size() - 1);
            return RespValue::bulk_string(all_keys[dis(gen)]);
        }
        else if (cmd == "INFO") // INFO 返回服务器信息
        {
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'INFO'");
            }
            std::string info;
            // Server
            info += "# Server\r\n";
            info += "redis_version:1.0.0\r\n";
            info += "tcp_port:6666\r\n";
            info += "\r\n";

            // Stats
            info += "# Stats\r\n";
            info += "total_connections_received:" + std::to_string(TcpServer<T>::getConnection()) + "\r\n";
            info += "total_commands_processed:" + std::to_string(m_commands.load(std::memory_order_acquire)) + "\r\n";
            info += "\r\n";

            // Memory
            info += "# Memory\r\n";
            size_t total_keys = 0;
            for (auto &shard : m_shards)
            {
                std::shared_lock lock(shard.mutex);
                total_keys += shard.store.size() + shard.hash.size() + shard.lists.size() + shard.sets.size() + shard.zset.size();
            }
            info += "total_keys:" + std::to_string(total_keys) + "\r\n";
            return RespValue::bulk_string(info);
        }
        else if (cmd == "SAVE") // SAVE 持久化
        {
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'SAVE'");
            }
            std::thread([this]
                        { saveToFile(); })
                .detach();
            return RespValue::simple_string("OK");
        }
        else if (cmd == "SHUTDOWN") // SHUTDOWN 关闭服务器,如果连接数为0
        {
            if (!m_authenticated.load(std::memory_order_acquire))
            {
                return RespValue::error("ERR permission denied");
            }
            if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'SHUTDOWN'");
            }
            m_shutdown.store(true, std::memory_order_release);
            return RespValue::bulk_string("OK - waiting for clients to disconnect");
        }
        return RespValue::error("ERR unknown command");
    }

    template <typename T>
    Task<void> CommandHandler<T>::handleClient(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(xx::g_logger) << "handleClient begin, fd=" << sock->getSocketfd();
        RespStreamParser parser;                     // 解析器
        const size_t MAX_COMMAND_SIZE = 1024 * 1024; // 解析缓冲区最大大小
        const size_t BATCH_SIZE = 8192;              // 批量响应大小阈值

        do
        {
            char tmp[8192];
            ssize_t ret = co_await sock->recv(tmp, sizeof(tmp));
            if (ret <= 0)
            {
                if (ret == 0)
                {
                    BLUE_LOG_INFO(xx::g_logger) << "[client " << sock->getSocketfd() << "] 正常关闭";
                }
                else
                {
                    BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                                 << "] 读取出错: " << strerror(errno);
                }
                break;
            }

            if (!parser.feed({tmp, static_cast<size_t>(ret)}))
            {
                BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                             << "] 缓冲区溢出，关闭连接";
                break;
            }

            // 批量处理命令
            std::string batch_response;
            RespValue cmd;
            int cmd_count = 0;

            // 喂入数据
            while (parser.next(cmd))
            {
                auto copy_arr = cmd.arr;
                // 安全检查
                if (cmd.type == RespValue::Type::ARRAY && copy_arr.size() > 1000)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "[client " << sock->getSocketfd()
                                                << "] 命令数组过大: " << copy_arr.size();
                    auto error_resp = RespValue::error("ERR command too large");
                    batch_response += RespValue::encode(error_resp);
                    break;
                }

                // 执行命令
                auto response = execute(std::move(copy_arr));
                batch_response += RespValue::encode(response);
                cmd_count++;
                m_commands.fetch_add(1, std::memory_order_acq_rel);

                // 如果批量响应达到阈值，立即发送
                if (batch_response.size() >= BATCH_SIZE)
                {
                    size_t sent = 0;
                    while (sent < batch_response.size())
                    {
                        ssize_t n = co_await sock->send(batch_response.data() + sent,
                                                        batch_response.size() - sent);
                        if (n <= 0)
                        {
                            BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                                         << "] 发送失败";
                            break;
                        }
                        sent += n;
                    }
                    batch_response.clear();

                    // 让出 CPU，避免饿死其他协程
                    co_await std::suspend_always{};
                }
            }

            // 发送剩余的响应
            if (!batch_response.empty())
            {
                size_t sent = 0;
                while (sent < batch_response.size())
                {
                    ssize_t n = co_await sock->send(batch_response.data() + sent,
                                                    batch_response.size() - sent);
                    if (n <= 0)
                    {
                        BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                                     << "] 发送失败";
                        break;
                    }
                    sent += n;
                }
            }

            if (cmd_count > 0)
            {
                BLUE_LOG_DEBUGE(xx::g_logger) << "Processed " << cmd_count
                                              << " commands in batch, batch_size="
                                              << batch_response.size();
            }

            // 检查缓冲区大小
            if (parser.bufferSize() > MAX_COMMAND_SIZE)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                             << "] 命令过大，关闭连接";
                break;
            }

        } while (true);

        sock->close();
        BLUE_LOG_INFO(xx::g_logger) << "one Client exit, fd:" << sock->getSocketfd();
        TcpServer<T>::subConnection();
        if (TcpServer<T>::getConnection() == 0 && m_shutdown.load(std::memory_order_acquire))
        {
            bool end = co_await TcpServer<T>::stop();
            if (end)
            {
                BLUE_LOG_INFO(xx::g_logger) << "tcpserver stoped";
            }
        }
        co_return;
    }
}
