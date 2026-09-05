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
/**
 * @file server_data.h
 * @brief redis server 的各模块数据以及相关操作
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.28
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <chrono>
#include <optional>
#include <list>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include "absl/container/flat_hash_map.h"
#include "blue/config.h"
#include "blue/msocket.h"
#include "blue/skiplist.h"
#include "blue/resp_parser.h"
#include "blue/tcpServer.h"
#include "blue/scan_cursor.h"
#include "modules/subscription.h"
#include "modules/slowlog.h"
#include "modules/monitor.h"
#include "modules/AOF.h"
#include "modules/replication.h"
#include "redis_command/generator.h"

namespace blue
{
    // 128个分片
    constexpr int SHARD_COUNT = 128;
    // 16个数据库
    constexpr int DB_COUNT = 16;

    // 单个分片的结构
    struct DataShard
    {
        using TimePoint = std::chrono::steady_clock::time_point;
        struct StoreData
        {
            std::string val;
            std::optional<TimePoint> expire;
            StoreData() : val(""), expire(std::nullopt) {}
            StoreData(const std::string &v, std::optional<TimePoint> e = std::nullopt) : val(v), expire(e) {}
            bool is_expired() const
            {
                if (!expire.has_value())
                {
                    return false;
                }
                return std::chrono::steady_clock::now() > expire.value();
            }
        };
        // std::shared_mutex mutex;
        // std::unordered_map<std::string, StoreData> store;
        // // std::unordered_map<std::string, std::string> store;
        // // std::unordered_map<std::string, TimePoint> expire;
        // std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash;
        // std::unordered_map<std::string, std::list<std::string>> lists;
        // std::unordered_map<std::string, std::unordered_set<std::string>> sets;
        // // key -> 跳表(ZSetKey{score + member} -> member)
        // std::unordered_map<std::string, SkipList<ZSetKey, std::string>> zset;
        // // key -> (member-> score)
        // std::unordered_map<std::string, std::unordered_map<std::string, double>> zset_score;

        std::shared_mutex mutex;
        absl::flat_hash_map<std::string, StoreData> store;
        absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, std::string>> hash;
        absl::flat_hash_map<std::string, std::list<std::string>> lists;
        absl::flat_hash_map<std::string, std::unordered_set<std::string>> sets;
        // key -> 跳表(ZSetKey{score + member} -> member)
        absl::flat_hash_map<std::string, SkipList<ZSetKey, std::string>> zset;
        // key -> (member-> score)
        absl::flat_hash_map<std::string, std::unordered_map<std::string, double>> zset_score;
    };

    template <typename T>
    class ServerData : public std::enable_shared_from_this<ServerData<T>>
    {
    public:
        using SteadyClock = std::chrono::steady_clock;
        using TimePoint = SteadyClock::time_point;
        ServerData() = default;
        ~ServerData() = default;
        ServerData(const ServerData &) = delete;
        ServerData &operator=(const ServerData &) = delete;

    public:
        /**
         * @brief 获取数据库
         */
        std::array<std::array<DataShard, SHARD_COUNT>, DB_COUNT> &getDBs() { return m_dbs; }

        /**
         * @brief 获取当前执行命令数量原子变量引用
         */
        std::atomic<uint32_t> &getCommands() { return m_commands; }

        /**
         * @brief 获取当前执行命令的数量
         */
        uint32_t getCommandsCount() const { return m_commands.load(std::memory_order_acquire); }

        /**
         * @brief 执行命令数量自增
         */
        void incrementCommands() { m_commands.fetch_add(1, std::memory_order_acq_rel); }

        /**
         * @brief 服务器是否停止原子变量引用
         */
        std::atomic<bool> &getShutdown() { return m_shutdown; }

        /**
         * @brief 服务器是否设置了shutdown
         */
        bool isShutdown() const { return m_shutdown.load(std::memory_order_acquire); }

        /**
         * @brief 设置shutdown原子变量
         */
        void setShutdown(bool val) { m_shutdown.store(val, std::memory_order_release); }

        /**
         * @brief 获取最新一次前台RDB保存时间的原子变量引用
         */
        std::atomic<time_t> &getLastSaveTime() { return m_last_time; }

        /**
         * @brief 获取最新一次前台RDB保存时间
         */
        time_t getLastSaveTime() const { return m_last_time.load(std::memory_order_acquire); }

        /**
         * @brief 设置最新一次前台RDB保存时间
         */
        void setLastSaveTime(time_t t) { m_last_time.store(t, std::memory_order_release); }

        /**
         * @brief 获取最新一次后台RDB保存时间的原子变量引用
         */
        std::atomic<bool> &getBgSaveRunning() { return m_bgsave_running; }

        /**
         * @brief 后台RDB保存是否正在进行中
         */
        bool isBgSaveRunning() const { return m_bgsave_running.load(std::memory_order_acquire); }

        /**
         * @brief 设置是否进行后台RDB保存
         */
        void setBgSaveRunning(bool val) { m_bgsave_running.store(val, std::memory_order_release); }

        /**
         * @brief 获取管理员Socket
         */
        MSocket::MSocketWPtr &getAdminSocket() { return m_admin_sock; }

        /**
         * @brief 设置管理员Socket
         */
        void setAdminSocket(MSocket::MSocketWPtr sock) { m_admin_sock = sock; }

        /**
         * @brief 是否是管理员
         * @param sock 判断sock是否是管理员
         */
        bool isAdmin(MSocket::MSocketPtr sock) const
        {
            auto admin = m_admin_sock.lock();
            return admin && admin == sock;
        }

        /**
         * @brief 获取发布和订阅模块对象引用
         */
        SubscriptionModule &getSubscription() { return m_subscription; }

        /**
         * @brief 获取慢查询模块对象引用
         */
        SlowLogModule &getSlowLog() { return m_slowLog; }

        /**
         * @brief 获取监控模块对象引用
         */
        MonitorModule &getMonitor() { return m_monitor; }

        /**
         * @brief 获取AOF持久化模块对象引用
         */
        AOFModule &getAOF() { return m_aof; }

        /**
         * @brief 获取主从复制模块对象引用
         */
        ReplicationModule &getReplication() { return m_replication; }

        /**
         * @brief 获取是否推送monitor原子变量引用
         */
        std::atomic<bool> &getPushMonitor() { return m_push_monitor; }

        /**
         * @brief 是否推送monitor
         * @return bool(true表示推送)
         */
        bool isPushMonitor() const { return m_push_monitor.load(std::memory_order_acquire); }

        /**
         * @brief 设置pushMonitor
         */
        void setPushMonitor(bool val) { m_push_monitor.store(val, std::memory_order_release); }

        /**
         * @brief 持久化到文件
         */
        void saveToFile();

        /**
         * @brief 从文件加载
         */
        void loadFromFile();

        /**
         * @brief 定期删除过期的命令
         */
        Task<void> expireTime();
        void removeExpireCycle();

        /**
         * @brief 生产RDB 消息供 replication 使用
         */
        Generator<std::string> generateRDB();

        /**
         * @brief 扫描键（完整实现）
         * @param db 数据库编号
         * @param cursor 游标（引用，会被更新）
         * @param pattern 匹配模式
         * @param count 每次返回数量
         * @param keys 输出：匹配的键列表
         * @return true 表示扫描完成，false 表示还有更多数据
         */
        bool scanKeys(int db, ScanCursor &cursor, const std::string &pattern,
                      int count, std::vector<std::string> &keys);

        /**
         * @brief 获取数据库中所有键的总数
         */
        size_t getTotalKeys(int db) const;

        /**
         * @brief 获取 key 对应的分片
         * @param key 键值
         */
        int getShardIndex(const std::string &key) const
        {
            return std::hash<std::string>{}(key) % SHARD_COUNT;
        }

        /**
         * @brief 获取分片引用
         * @param key 键值
         * @param sock socket 智能指针对象
         */
        DataShard &getShard(const std::string &key, MSocket::MSocketPtr sock)
        {
            return m_dbs[sock->getClientId()][getShardIndex(key)];
        }

        const DataShard &getShard(const std::string &key, MSocket::MSocketPtr sock) const
        {
            return m_dbs[sock->getClientId()][getShardIndex(key)];
        }

        bool isReadOnlyCommand(const std::string &cmd) const
        {
            static const std::unordered_set<std::string> read_only = {
                "GET", "MGET", "EXISTS", "TTL", "PTTL", "STRLEN", "TYPE",
                "HGET", "HGETALL", "HLEN", "HEXISTS", "HKEYS", "HVALS",
                "LLEN", "LINDEX", "LRANGE",
                "SCARD", "SISMEMBER", "SMEMBERS", "SRANDMEMBER",
                "SDIFF", "SUNION", "SINTER"
                                   "ZSCORE",
                "ZRANK", "ZCOUNT", "ZRANGE", "ZRANGEBYSCORE",
                "KEYS", "DBSIZE", "INFO", "TIME", "LOCALTIME", "LASTSAVE"};
            return read_only.count(cmd) > 0;
        }

        /**
         * @brief 获取key版本
         * @param key 键值
         * @param sock 封装的socket 类智能指针
         * @note Watch + Transaction 模式使用
         */
        uint64_t getKeyVersion(const std::string &key, MSocket::MSocketPtr sock)
        {
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);

            auto it = shard.store.find(key);
            if (it != shard.store.end())
            {
                return std::hash<std::string>{}(it->second.val);
            }
            return 0;
        }

        /**
         * @brief 格式化socre
         */
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

        /**
         * @brief 设置tcpserver
         */
        void setTcpServer(TcpServer<T> *tcp) { m_tcpserver = tcp; }

        /**
         * @brief 获取当前连接数量
         * @return 当前连接数量
         */
        uint32_t getConnection() const noexcept { return m_tcpserver->getConnection(); }

        /**
         * @brief 获取当前拒绝连接数量
         * @return 当前连接数量
         */
        uint32_t getRejectConnection() const noexcept { return m_tcpserver->getRejectConnection(); }

    private:
        /**
         * @brief 通配符匹配
         */
        bool matchPattern(const std::string &key, const std::string &pattern) const;

        /**
         * @brief 编译正则表达式（带缓存）
         */
        std::regex getRegex(const std::string &pattern) const;

        /**
         * @brief 从单个分片扫描指定类型的数据
         */
        template <typename Container>
        bool scanContainer(const Container &container, const std::string &pattern,
                           size_t startOffset, int count,
                           std::vector<std::string> &keys, size_t &offset,
                           int &matchedCount);

        /**
         * @brief 获取容器大小
         */
        template <typename Container>
        size_t getContainerSize(const Container &container) const;

    private:
        TcpServer<T> *m_tcpserver = nullptr;

    private:
        /* REDIS SERVER */
        std::array<std::array<DataShard, SHARD_COUNT>, DB_COUNT> m_dbs; // 数据库
        std::atomic<uint32_t> m_commands{0};                            // 总共命令数量
        std::atomic<bool> m_shutdown{false};                            // 服务器关闭标识
        std::atomic<time_t> m_last_time{0};                             // 上一次保存rdb文件时间
        std::atomic<bool> m_bgsave_running{false};                      // 后台保存rdb
        MSocket::MSocketWPtr m_admin_sock;                              // 用于同一时间只能一个管理员上线
    private:
        /* SUBSCRIPTION */
        SubscriptionModule m_subscription;
        /* SLOWLOG */
        SlowLogModule m_slowLog; // slowlog
        /* MONITOR */
        MonitorModule m_monitor;                // monitor模式
        std::atomic<bool> m_push_monitor{true}; // 是否推送给monitor
        /* AOF */
        AOFModule m_aof; // AOF
        /* Replication */
        ReplicationModule m_replication;

        // 正则表达式缓存
        mutable std::unordered_map<std::string, std::regex> m_regexCache;
        mutable std::shared_mutex m_regexMutex;
    };

    template <typename T>
    void ServerData<T>::saveToFile()
    {
        const std::string filename = "/var/lib/blueRedis/dump.rdb";
        std::ofstream file(filename, std::ios::binary);

        if (!file)
        {
            BLUE_LOG_ERROR(xx::g_logger) << "Failed to open " << filename;
            return;
        }
        for (int db = 0; db < DB_COUNT; db++)
        {
            for (auto &shard : m_dbs[db])
            {
                std::shared_lock lock(shard.mutex);

                for (auto &[key, value] : shard.store)
                {
                    file << "DB|" << db << "|STR|" << key << "|" << value.val;

                    if (value.is_expired())
                    {
                        auto expire_time = value.expire.value().time_since_epoch().count();
                        file << "|" << expire_time;
                    }
                    file << "\n";
                }

                for (auto &[key, fields] : shard.hash)
                {
                    for (auto &[field, value] : fields)
                    {
                        file << "DB|" << db << "|HASH|" << key << "|" << field << "|" << value << "\n";
                    }
                }

                for (auto &[key, list] : shard.lists)
                {
                    for (auto &value : list)
                    {
                        file << "DB|" << db << "|LIST|" << key << "|" << value << "\n";
                    }
                }

                for (auto &[key, set] : shard.sets)
                {
                    for (auto &member : set)
                    {
                        file << "DB|" << db << "|SET|" << key << "|" << member << "\n";
                    }
                }

                for (auto &[key, zset] : shard.zset_score)
                {
                    for (auto &[member, score] : zset)
                    {
                        file << "DB|" << db << "|ZSET|" << key << "|" << score << "|" << member << "\n";
                    }
                }
            }
        }
        m_last_time.store(time(nullptr), std::memory_order_release);
        BLUE_LOG_INFO(xx::g_logger) << "RDB saved to " << filename;
    }

    template <typename T>
    void ServerData<T>::loadFromFile()
    {
        BLUE_LOG_INFO(xx::g_logger) << "loadFromFile";
        const std::string filename = "/var/lib/blueRedis/dump.rdb";
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
            parts.push_back(line);

            if (parts.empty())
            {
                continue;
            }

            // parts[0] = "DB"
            // parts[1] = 数据库编号
            // parts[2] = 类型 (STR/HASH/LIST/SET/ZSET)

            int db = 0;
            try
            {
                db = std::stoi(parts[1]);
            }
            catch (...)
            {
                return;
            }
            std::string type = parts[2];

            // 临时保存到对应的数据库
            auto &target_db = m_dbs[db];

            if (type == "STR" && parts.size() >= 5)
            {
                std::string key = parts[3];
                std::string value = parts[4];

                // 找到正确的分片
                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                // shard.store[key] = DataShard::StoreData(value);
                shard.store.insert_or_assign(key, DataShard::StoreData(value));

                if (parts.size() >= 6)
                {
                    int64_t expire_time = std::stoll(parts[5]);
                    // shard.store[key].expire = TimePoint(std::chrono::nanoseconds(expire_time));
                    shard.store.insert_or_assign(key, DataShard::StoreData(value, TimePoint(std::chrono::nanoseconds(expire_time))));
                }
            }
            else if (type == "HASH" && parts.size() >= 6)
            {
                std::string key = parts[3];
                std::string field = parts[4];
                std::string value = parts[5];

                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                absl::flat_hash_map<std::string, std::string> internal;
                internal.insert_or_assign(field, value);
                shard.hash.insert_or_assign(key, std::move(internal));
                // shard.hash[key][field] = value;
            }
            else if (type == "LIST" && parts.size() >= 5)
            {
                std::string key = parts[3];
                std::string value = parts[4];

                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                // shard.lists[key].push_back(value);
                std::list<std::string> list;
                list.push_back(value);
                shard.lists.insert_or_assign(key, std::move(list));
            }
            else if (type == "SET" && parts.size() >= 5)
            {
                std::string key = parts[3];
                std::string member = parts[4];

                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                // shard.sets[key].insert(member);
                std::unordered_set<std::string> set;
                set.insert(member);
                shard.sets.insert_or_assign(key, std::move(set));
            }
            else if (type == "ZSET" && parts.size() >= 6)
            {
                std::string key = parts[3];
                double score = std::stod(parts[4]);
                std::string member = parts[5];

                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                shard.zset_score[key][member] = score;
                shard.zset[key].insert({score, member}, member);
            }
        }
        BLUE_LOG_INFO(xx::g_logger) << "RDB loaded from " << filename;
    }

    template <typename T>
    Task<void> ServerData<T>::expireTime()
    {
        while (m_shutdown.load(std::memory_order_acquire))
        {
            co_await sleepFor(1);
            removeExpireCycle();
        }
    }

    template <typename T>
    void ServerData<T>::removeExpireCycle()
    {
        if (m_shutdown.load(std::memory_order_acquire))
        {
            return;
        }
        int count = 0;
        auto now = SteadyClock::now();
        for (int db = 0; db < DB_COUNT; db++)
        {
            if (m_shutdown.load(std::memory_order_acquire))
            {
                return;
            }
            for (auto &shards : m_dbs[db])
            {
                if (m_shutdown.load(std::memory_order_acquire))
                {
                    return;
                }
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto it = shards.store.begin();
                while (it != shards.store.end() && count < 20 && it->second.expire.has_value())
                {
                    if (it->second.expire.value() < now)
                    {
                        auto tem_it = it;
                        ++it;
                        shards.store.erase(tem_it);
                    }
                    else
                    {
                        ++it;
                    }
                    ++count;
                }
            }
        }
    }

    // 改用Generator流式推送RDB信息，防止数据过大，将内存撑爆并且可以充分享受协程+调度器优势
    template <typename T>
    Generator<std::string> ServerData<T>::generateRDB()
    {
        std::string result;
        for (int db = 0; db < DB_COUNT; db++)
        {
            bool has_data = false;
            for (auto &shard : m_dbs[db])
            {
                if (!shard.store.empty() || !shard.hash.empty() ||
                    !shard.lists.empty() || !shard.sets.empty() ||
                    !shard.zset.empty())
                {
                    has_data = true;
                    break;
                }
            }
            if (!has_data)
            {
                continue;
            }

            // SELECT db
            co_yield "*2\r\n$6\r\nSELECT\r\n$" + std::to_string(std::to_string(db).size()) +
                "\r\n" + std::to_string(db) + "\r\n";

            for (auto &shard : m_dbs[db])
            {
                std::shared_lock<std::shared_mutex> lock(shard.mutex);

                // String
                for (const auto &[key, value] : shard.store)
                {
                    std::string entry = "*3\r\n$3\r\nSET\r\n$" + std::to_string(key.size()) +
                                        "\r\n" + key + "\r\n$" + std::to_string(value.val.size()) +
                                        "\r\n" + value.val + "\r\n";
                    if (value.expire.has_value())
                    {
                        auto now = SteadyClock::now();
                        auto remain = std::chrono::duration_cast<std::chrono::seconds>(
                            value.expire.value() - now).count();
                        if (remain > 0)
                        {
                            entry += "*2\r\n$2\r\nEX\r\n$" + std::to_string(std::to_string(remain).size()) + 
                                    "\r\n" + std::to_string(remain) + "\r\n";
                        }
                    }

                    co_yield std::move(entry);
                }

                // Hash
                for (const auto &[key, fields] : shard.hash)
                {
                    for (auto &[field, value] : fields)
                    {
                        std::string entry = "*4\r\n$4\r\nHSET\r\n$" + std::to_string(key.size()) +
                                  "\r\n" + key + "\r\n$" + std::to_string(field.size()) +
                                  "\r\n" + field + "\r\n$" + std::to_string(value.size()) +
                                  "\r\n" + value + "\r\n";
                        co_yield std::move(entry);
                    }
                }

                // Lists
                for (const auto &[key, list] : shard.lists)
                {
                    for (auto &value : list)
                    {
                        std::string entry = "*3\r\n$5\r\nRPUSH\r\n$" + std::to_string(key.size()) +
                                  "\r\n" + key + "\r\n$" + std::to_string(value.size()) +
                                  "\r\n" + value + "\r\n";
                        co_yield std::move(entry);
                    }
                }

                // Set
                for (const auto &[key, set] : shard.sets)
                {
                    for (const auto &member : set)
                    {
                        std::string entry = "*3\r\n$4\r\nSADD\r\n$" + std::to_string(key.size()) +
                                  "\r\n" + key + "\r\n$" + std::to_string(member.size()) +
                                  "\r\n" + member + "\r\n";
                        co_yield std::move(entry);
                    }
                }

                // Zset
                for (const auto &[key, score_map] : shard.zset_score)
                {
                    for (const auto &[member, score] : score_map)
                    {
                        std::string score_str = std::to_string(score);
                        std::string entry = "*4\r\n$4\r\nZADD\r\n$" + std::to_string(key.size()) +
                                  "\r\n" + key + "\r\n$" + std::to_string(score_str.size()) +
                                  "\r\n" + score_str + "\r\n$" + std::to_string(member.size()) +
                                  "\r\n" + member + "\r\n";
                        co_yield std::move(entry);
                    }
                }
            }
        }
        co_yield "*1\r\n$3\r\nEOF\r\n";
    }

    // 通配符转正则
    inline std::string globToRegex(const std::string &pattern)
    {
        std::string regex_str;
        regex_str.reserve(pattern.size() * 2);
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
                     c == '(' || c == ')' || c == '\\' || c == '^' || c == '$')
            {
                regex_str += '\\';
                regex_str += c;
            }
            else
            {
                regex_str += c;
            }
        }
        return regex_str;
    }

    // 获取编译后的正则（带缓存）
    template <typename T>
    std::regex ServerData<T>::getRegex(const std::string &pattern) const
    {
        // 先检查缓存
        {
            std::shared_lock<std::shared_mutex> lock(m_regexMutex);
            auto it = m_regexCache.find(pattern);
            if (it != m_regexCache.end())
            {
                return it->second;
            }
        }

        // 编译新的正则
        std::string regexPattern = globToRegex(pattern);
        std::regex reg(regexPattern);

        // 存入缓存
        {
            std::unique_lock<std::shared_mutex> lock(m_regexMutex);
            m_regexCache[pattern] = reg;
        }

        return reg;
    }

    // 通配符匹配
    template <typename T>
    bool ServerData<T>::matchPattern(const std::string &key, const std::string &pattern) const
    {
        if (pattern == "*")
        {
            return true;
        }
        if (pattern == key)
        {
            return true;
        }

        try
        {
            std::regex reg = getRegex(pattern);
            return std::regex_match(key, reg);
        }
        catch (...)
        {
            return key.find(pattern) != std::string::npos;
        }
    }

    // 获取容器大小（特化）
    template <typename T>
    template <typename Container>
    size_t ServerData<T>::getContainerSize(const Container &container) const
    {
        return container.size();
    }

    // 扫描单个容器
    template <typename T>
    template <typename Container>
    bool ServerData<T>::scanContainer(const Container &container, const std::string &pattern,
                                      size_t startOffset, int count,
                                      std::vector<std::string> &keys,
                                      size_t &offset, int &matchedCount)
    {
        // 返回true表示还可以继续扫描，也就是容器还有数据没有被扫描
        size_t size = container.size();
        if (startOffset >= size)
        {
            offset = size;
            return false; // 该容器已遍历完
        }

        auto it = container.cbegin();
        std::advance(it, startOffset);
        offset = startOffset;

        while (it != container.cend() && matchedCount < count)
        {
            if (matchPattern(it->first, pattern))
            {
                keys.push_back(it->first);
                ++matchedCount;
            }
            ++it;
            ++offset;
        }

        return matchedCount >= count; // 是否达到 count 限制
    }

    // 主 SCAN 实现
    template <typename T>
    bool ServerData<T>::scanKeys(int db, ScanCursor &cursor, const std::string &pattern,
                                 int count, std::vector<std::string> &keys)
    {
        if (db < 0 || db >= DB_COUNT)
        {
            return false;
        }

        if (count <= 0)
        {
            count = 10;
        }
        if (count > 1000)
        {
            count = 1000; // 限制最大值
        }

        keys.clear();

        // 如果游标已完成，直接返回
        if (cursor.completed)
        {
            BLUE_LOG_INFO(xx::g_logger) << "cursor completed";
            return true;
        }

        int matchedCount = 0;

        // 从当前分片开始遍历
        for (int shardIdx = cursor.shardIndex; shardIdx < SHARD_COUNT && matchedCount < count; ++shardIdx)
        {
            auto &shard = m_dbs[db][shardIdx];
            std::shared_lock<std::shared_mutex> lock(shard.mutex);

            // 确定当前分片的起始数据类型
            int startType = (shardIdx == cursor.shardIndex) ? cursor.dataType : 0;
            size_t startOffset = (shardIdx == cursor.shardIndex) ? cursor.offset : 0;

            // 遍历所有数据类型
            for (int dataType = startType; dataType <= 4 && matchedCount < count; ++dataType)
            {
                size_t offset = (dataType == startType) ? startOffset : 0;
                bool hasMore = false;

                switch (dataType)
                {
                case 0:
                { // string
                    hasMore = scanContainer(shard.store, pattern, offset, count,
                                            keys, offset, matchedCount);
                    if (hasMore)
                    {
                        cursor.shardIndex = shardIdx;
                        cursor.dataType = dataType;
                        cursor.offset = offset;
                        return false; // 还有更多数据,需要保存游标状态
                    }
                    break;
                }
                case 1:
                { // hash
                    hasMore = scanContainer(shard.hash, pattern, offset, count,
                                            keys, offset, matchedCount);
                    if (hasMore)
                    {
                        cursor.shardIndex = shardIdx;
                        cursor.dataType = dataType;
                        cursor.offset = offset;
                        return false;
                    }
                    break;
                }
                case 2:
                { // list
                    hasMore = scanContainer(shard.lists, pattern, offset, count,
                                            keys, offset, matchedCount);
                    if (hasMore)
                    {
                        cursor.shardIndex = shardIdx;
                        cursor.dataType = dataType;
                        cursor.offset = offset;
                        return false;
                    }
                    break;
                }
                case 3:
                { // set
                    hasMore = scanContainer(shard.sets, pattern, offset, count,
                                            keys, offset, matchedCount);
                    if (hasMore)
                    {
                        cursor.shardIndex = shardIdx;
                        cursor.dataType = dataType;
                        cursor.offset = offset;
                        return false;
                    }
                    break;
                }
                case 4:
                { // zset
                    hasMore = scanContainer(shard.zset, pattern, offset, count,
                                            keys, offset, matchedCount);
                    if (hasMore)
                    {
                        cursor.shardIndex = shardIdx;
                        cursor.dataType = dataType;
                        cursor.offset = offset;
                        return false;
                    }
                    break;
                }
                default:
                    break;
                }
            }

            // 当前分片所有数据类型遍历完成
            // 更新游标到下一个分片
            cursor.shardIndex = shardIdx + 1;
            cursor.dataType = 0;
            cursor.offset = 0;
        }

        // 所有分片遍历完成
        cursor.completed = true;
        return true;
    }

    // 获取总键数
    template <typename T>
    size_t ServerData<T>::getTotalKeys(int db) const
    {
        if (db < 0 || db >= DB_COUNT)
            return 0;

        size_t total = 0;
        for (int shardIdx = 0; shardIdx < SHARD_COUNT; ++shardIdx)
        {
            const auto &shard = m_dbs[db][shardIdx];
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            total += shard.store.size();
            total += shard.hash.size();
            total += shard.lists.size();
            total += shard.sets.size();
            total += shard.zset.size();
        }
        return total;
    }

} // namespace blue