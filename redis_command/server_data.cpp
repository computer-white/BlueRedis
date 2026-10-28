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
#include "server_data.h"
#include "src/core/await.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    void ServerData::saveToFile()
    {
        try
        {
            const std::string filename = "/var/lib/blueRedis/dump.rdb";
            std::ofstream file(filename, std::ios::binary);

            if (!file)
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to open " << filename;
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
            BLUE_LOG_INFO(g_logger) << "RDB saved to " << filename;
        }
        catch (...)
        {
            return;
        }
    }

    void ServerData::loadFromFile()
    {
        try
        {
            BLUE_LOG_INFO(g_logger) << "loadFromFile";
            const std::string filename = "/var/lib/blueRedis/dump.rdb";
            std::ifstream file(filename);

            if (!file)
            {
                BLUE_LOG_INFO(g_logger) << "No existing RDB file";
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
                    shard.store.insert_or_assign(key, DataShard::StoreData(value));

                    if (parts.size() >= 6)
                    {
                        int64_t expire_time = std::stoll(parts[5]);
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
                    auto [it, inserted] = shard.hash.try_emplace(key, absl::flat_hash_map<std::string, std::string>{});
                    it->second.insert_or_assign(field, value);
                }
                else if (type == "LIST" && parts.size() >= 5)
                {
                    std::string key = parts[3];
                    std::string value = parts[4];

                    int shard_idx = getShardIndex(key);
                    auto &shard = target_db[shard_idx];
                    std::unique_lock lock(shard.mutex);
                    auto [it, inserted] = shard.lists.try_emplace(key, std::list<std::string>{});
                    it->second.push_back(value);
                }
                else if (type == "SET" && parts.size() >= 5)
                {
                    std::string key = parts[3];
                    std::string member = parts[4];

                    int shard_idx = getShardIndex(key);
                    auto &shard = target_db[shard_idx];
                    std::unique_lock lock(shard.mutex);
                    auto [it, inserted] = shard.sets.try_emplace(key, std::unordered_set<std::string>{});
                    it->second.insert(member);
                }
                else if (type == "ZSET" && parts.size() >= 6)
                {
                    std::string key = parts[3];
                    double score = std::stod(parts[4]);
                    std::string member = parts[5];

                    int shard_idx = getShardIndex(key);
                    auto &shard = target_db[shard_idx];
                    std::unique_lock lock(shard.mutex);
                    // zset_score
                    auto [sit, sinserted] = shard.zset_score.try_emplace(key, absl::flat_hash_map<std::string, double>{});
                    sit->second.insert_or_assign(member, score);

                    // zset
                    auto [zit, zinserted] = shard.zset.try_emplace(key, SkipList<ZSetKey, std::string>{});
                    zit->second.insert({score, member}, member);
                }
            }
            BLUE_LOG_INFO(g_logger) << "RDB loaded from " << filename;
        }
        catch (...)
        {
            return;
        }
    }

    Task<void> ServerData::expireTime()
    {
        while (m_shutdown.load(std::memory_order_acquire))
        {
            co_await sleepFor(1);
            removeExpireCycle();
        }
    }

    void ServerData::removeExpireCycle()
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
    Generator<std::string> ServerData::generateRDB()
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
                                          value.expire.value() - now)
                                          .count();
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
    std::regex ServerData::getRegex(const std::string &pattern) const
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
    bool ServerData::matchPattern(const std::string &key, const std::string &pattern) const
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
    template <typename Container>
    size_t ServerData::getContainerSize(const Container &container) const
    {
        return container.size();
    }

    // 扫描单个容器
    template <typename Container>
    bool ServerData::scanContainer(const Container &container, const std::string &pattern,
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
    bool ServerData::scanKeys(int db, ScanCursor &cursor, const std::string &pattern,
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
            BLUE_LOG_INFO(g_logger) << "cursor completed";
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
    size_t ServerData::getTotalKeys(int db) const
    {
        if (db < 0 || db >= DB_COUNT)
        {
            return 0;
        }

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
}