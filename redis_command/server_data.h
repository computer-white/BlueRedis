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
#include <regex>
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
        mutable std::shared_mutex mutex;
        absl::flat_hash_map<std::string, StoreData> store;
        absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, std::string>> hash;
        absl::flat_hash_map<std::string, std::list<std::string>> lists;
        absl::flat_hash_map<std::string, std::unordered_set<std::string>> sets;
        // key -> 跳表(ZSetKey{score + member} -> member)
        absl::flat_hash_map<std::string, SkipList<ZSetKey, std::string>> zset;
        // key -> (member-> score)
        absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, double>> zset_score;
    };

    class ServerData : public std::enable_shared_from_this<ServerData>
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
         *  bool(true表示推送)
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
         *  true 表示扫描完成，false 表示还有更多数据
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
        void setTcpServer(TcpServer *tcp) { m_tcpserver = tcp; }

        /**
         * @brief 获取当前连接数量
         *  当前连接数量
         */
        uint32_t getConnection() const noexcept { return m_tcpserver->getConnection(); }

        /**
         * @brief 获取当前拒绝连接数量
         *  当前连接数量
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
        TcpServer *m_tcpserver = nullptr;

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
} // namespace blue