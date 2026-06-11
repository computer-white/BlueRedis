#pragma once
#include <memory>
#include <regex>
#include <chrono>
#include <list>
#include "task.h"
#include "tcpServer.h"
#include "resp_parser.h"
#include "asyncio.h"
#include "await.h"
#include "skiplist.h"

namespace blue
{
    template <typename T>
    class CommandHandler : public TcpServer<T>
    {
    public:
        using CommandHandlerPtr = std::shared_ptr<CommandHandler>;
        using SteadyClock = std::chrono::steady_clock;
        using TimePoint = SteadyClock::time_point;

    public:
        CommandHandler(int level = -1, int option_name = -1, T option = T(), IOManager *manager = IOManager::GetThis(),
                       IOManager *acceptmanager = IOManager::GetThis());

        ~CommandHandler();
        RespValue execute(const std::vector<RespValue> &args);

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

    private:
        std::shared_mutex m_mutex;
        std::unordered_map<std::string, std::string> m_store;
        std::unordered_map<std::string, TimePoint> m_expire;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_hash;
        std::unordered_map<std::string, std::list<std::string>> m_lists;
        // key -> 跳表(ZSetKey{score + member} -> member)
        std::unordered_map<std::string, SkipList<ZSetKey, std::string>> m_zset;
        // key -> (member-> score)
        std::unordered_map<std::string, std::unordered_map<std::string, double>> m_zset_score;
    };

    template <typename T>
    CommandHandler<T>::CommandHandler(int level, int option_name, T option, IOManager *manager,
                                      IOManager *acceptmanager)
        : TcpServer<T>(level, option_name, option, manager, acceptmanager)
    {
        IOManager::GetThis()->schedule(expireTime());
    }

    template <typename T>
    CommandHandler<T>::~CommandHandler()
    {
    }

    template <typename T>
    void CommandHandler<T>::removeExpireCycle()
    {
        int count = 0;
        auto now = SteadyClock::now();
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_expire.begin();
        while (it != m_expire.end() && count < 20)
        {
            if (it->second < now)
            {
                m_store.erase(it->first);
                it = m_expire.erase(it);
            }
            else
            {
                it++;
            }
            ++count;
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
    RespValue CommandHandler<T>::execute(const std::vector<RespValue> &args)
    {
        if (args.empty())
        {
            return RespValue::error("ERR empty command");
        }

        std::string cmd = args[0].str;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "PING")
        {
            return RespValue::simple_string("PONG");
        }
        else if (cmd == "SET") // SET key val [EX [s]]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'SET'");
            }
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_store[args[1].str] = args[2].str;
                if (args.size() >= 5)
                {
                    std::string subcmd = args[3].str;
                    std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
                    if (subcmd == "EX")
                    {
                        int64_t seconds = std::stoll(args[4].str);
                        m_expire[args[1].str] = SteadyClock::now() + std::chrono::seconds(seconds);
                    }
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
            std::shared_lock<std::shared_mutex> redlock(m_mutex);
            auto it = m_store.find(args[1].str);
            if (it == m_store.end())
            {
                return RespValue::null_bulk();
            }
            redlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_store.end())
            {
                return RespValue::null_bulk();
            }
            auto expire_it = m_expire.find(args[1].str);
            if (expire_it != m_expire.end() && expire_it->second < SteadyClock::now())
            {
                m_expire.erase(expire_it);
                m_store.erase(it);
                return RespValue::null_bulk();
            }
            return RespValue::bulk_string(it->second);
        }
        else if (cmd == "MSET") // MSET key val [key val]
        {
            if (args.size() < 3 || (args.size() & 1) != 1)
            {
                return RespValue::error("ERR wrong of arguments for 'MSET'");
            }
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (size_t i = 1; i < args.size(); i += 2)
            {
                const std::string key = args[i].str;
                const std::string val = args[i + 1].str;
                m_store[key] = val;
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
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            for (size_t i = 1; i < args.size(); i++)
            {
                auto it = m_store.find(args[i].str);
                if (it == m_store.end())
                {
                    results.push_back(RespValue::null_bulk());
                    continue;
                }
                auto expire_it = m_expire.find(args[i].str);
                if (expire_it != m_expire.end() && expire_it->second < SteadyClock::now())
                {
                    m_expire.erase(expire_it);
                    m_store.erase(it);
                    results.push_back(RespValue::null_bulk());
                    continue;
                }
                results.push_back(RespValue::bulk_string(it->second));
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
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_store.find(key);
            if (it == m_store.end())
            {
                m_store[key] = val;
            }
            else
            {
                it->second.append(val);
            }
            return RespValue::integer(m_store[key].size());
        }
        else if (cmd == "SETNX") // SETNX key val
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong of arguments for 'SEtNX'");
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_store.find(key);
            if (it == m_store.end())
            {
                m_store[key] = val;
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            for (size_t i = 1; i < args.size(); i++)
            {
                if (m_store.find(args[i].str) != m_store.end())
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
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (size_t i = 1; i < args.size(); i++)
            {
                auto it = m_store.find(args[i].str);
                if (it != m_store.end())
                {
                    std::string key = it->first;
                    m_store.erase(it);
                    if (m_expire.contains(key))
                    {
                        m_expire.erase(key);
                    }
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
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (size_t i = 2; i < args.size(); i += 2)
            {
                auto &field = args[i].str;
                auto &value = args[i + 1].str; // size 是偶数所以不会出界
                if (m_hash[key].find(field) == m_hash[key].end())
                {
                    ++count; // 新字段
                }
                m_hash[key][field] = value;
            }
            return RespValue::integer(count);
        }
        else if (cmd == "HGET") // HGET key field
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'HGET'");
            }
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_hash.find(args[1].str);
            if (it == m_hash.end())
            {
                return RespValue::null_bulk();
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_hash.end())
            {
                return RespValue::null_bulk();
            }
            auto fit = it->second.find(args[2].str);
            if (fit == it->second.end())
            {
                return RespValue::null_bulk();
            }
            return RespValue::bulk_string(fit->second);
        }
        else if (cmd == "HGETALL") // HGETALL key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'HGETALL'");
            }
            std::vector<RespValue> result;
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_hash.find(args[1].str);
            if (it == m_hash.end())
            {
                return RespValue::array(std::move(result));
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_hash.end())
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_hash.find(args[1].str);
            if (it == m_hash.end())
            {
                return RespValue::integer(count);
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_hash.end())
            {
                return RespValue::integer(count);
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                if (it->second.contains(args[i].str))
                {
                    it->second.erase(args[i].str);
                    ++count;
                }
            }
            if (it->second.empty())
            {
                m_hash.erase(it);
            }
            return RespValue::integer(count);
        }
        else if (cmd == "HLEN") // HLEN key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong of arguments for 'HLEN'");
            }
            const std::string key = args[1].str;
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_hash.find(key);
            if (it == m_hash.end())
            {
                if (m_store.find(key) != m_store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_lists.find(key) != m_lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_zset.find(key) != m_zset.end())
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
                return RespValue::error("ERR wrong of arguments for 'HEXISTS'");
            }
            const std::string key = args[1].str;
            const std::string field = args[2].str;
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_hash.find(key);
            if (it == m_hash.end())
            {
                if (m_store.find(key) != m_store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_lists.find(key) != m_lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_zset.find(key) != m_zset.end())
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
                return RespValue::error("ERR wrong of arguments for 'HKEYS'");
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_hash.find(key);
            if (it == m_hash.end())
            {
                if (m_store.find(key) != m_store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_lists.find(key) != m_lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_zset.find(key) != m_zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::array(std::move(results));
            }
            for (auto& [field,_] : it->second)
            {
                results.push_back(RespValue::bulk_string(field));
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "HVALS") // HVALS key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong of arguments for 'HKEYS'");
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_hash.find(key);
            if (it == m_hash.end())
            {
                if (m_store.find(key) != m_store.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_lists.find(key) != m_lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_zset.find(key) != m_zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::array(std::move(results));
            }
            for (auto& [_,val] : it->second)
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

            std::regex re(regex_str);
            std::vector<RespValue> result;

            std::shared_lock<std::shared_mutex> lock(m_mutex);
            // 遍历 m_store
            for (auto &[key, value] : m_store)
            {
                if (std::regex_match(key, re))
                {
                    result.push_back(RespValue::bulk_string(key));
                }
            }
            // 遍历 m_hash
            for (auto &[key, fields] : m_hash)
            {
                for (auto &[field, val] : fields)
                {
                    if (std::regex_match(field, re))
                    {
                        result.push_back(RespValue::bulk_string(field));
                    }
                }
                if (std::regex_match(key, re))
                {
                    result.push_back(RespValue::bulk_string(key));
                }
            }

            return RespValue::array(std::move(result));
        }
        else if (cmd == "LPUSH") // LPUSH key val [val...]
        {
            if (args.size() < 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'LPUSH'");
            }
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto &lhs = m_lists[args[1].str];
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
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto &lhs = m_lists[args[1].str];
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
            int count = args.size() == 3 ? std::stoi(args[2].str) : 1;
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_lists.find(args[1].str);
            if (it == m_lists.end() || it->second.empty())
            {
                return RespValue::null_bulk();
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_lists.end() || it->second.empty())
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
                m_lists.erase(it);
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
            int count = args.size() == 3 ? std::stoi(args[2].str) : 1;
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_lists.find(args[1].str);
            if (it == m_lists.end() || it->second.empty())
            {
                return RespValue::null_bulk();
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_lists.end() || it->second.empty())
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
                m_lists.erase(it);
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_lists.find(args[1].str);
            if (it == m_lists.end())
            {
                return RespValue::array({});
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_lists.end())
            {
                return RespValue::array({});
            }
            int start = std::stoi(args[2].str);
            int stop = std::stoi(args[3].str);
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
        }
        else if (cmd == "ZADD") // ZADD key score member [score member]
        {
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return RespValue::error("ERR wrong number of arguments for 'ZADD'");
            }
            int count = 0;
            const std::string key = args[1].str;
            std::unique_lock<std::shared_mutex> lock(m_mutex);

            m_zset_score.try_emplace(key);
            m_zset.try_emplace(key);
            auto &score_map = m_zset_score[key];
            auto &skiplist = m_zset[key];

            for (size_t i = 2; i < args.size(); i += 2)
            {
                double score;
                try
                {
                    score = std::stod(args[i].str);
                }
                catch (const std::exception &e)
                {
                    return RespValue::error("ERR value is not a valid float");
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_zset.find(args[1].str);
            if (it == m_zset.end())
            {
                return RespValue::array({});
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_zset.end())
            {
                return RespValue::array({});
            }
            int start = std::stoi(args[2].str);
            int stop = std::stoi(args[3].str);
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);

            auto it = m_zset.find(key);
            if (it == m_zset.end())
            {
                return RespValue::integer(count);
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_zset.end())
            {
                return RespValue::integer(count);
            }
            auto sit = m_zset_score.find(key);
            if (sit != m_zset_score.end())
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
                    m_zset.erase(it);
                    m_zset_score.erase(key);
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_zset_score.find(key);
            if (it == m_zset_score.end())
            {
                return RespValue::null_bulk();
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_zset_score.end())
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
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_zset_score.find(key);
            if (it == m_zset_score.end())
            {
                return RespValue::null_bulk();
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_zset_score.end())
            {
                return RespValue::null_bulk();
            }
            auto &score_map = it->second;
            auto sit = score_map.find(member);
            if (sit != score_map.end())
            {
                // key  = {score,member}
                int rank = m_zset[key].getRank({sit->second, member});
                if (rank >= 0)
                {
                    return RespValue::integer(rank);
                }
            }
            return RespValue::null_bulk();
        }
        else if (cmd == "INCR") // INCR key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'INCR'");
            }
            const std::string key = args[1].str;
            int64_t val = 0;
            std::unique_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_store.find(key);
            if (it != m_store.end())
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
            m_store[key] = std::to_string(val);
            return RespValue::integer(val);
        }
        else if (cmd == "INCRBY") // INCR key integer
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
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_store.find(key);
            int64_t val = 0;
            if (it != m_store.end())
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
            m_store[key] = std::to_string(val);
            return RespValue::integer(val);
        }
        else if (cmd == "STRLEN") // STRLEN key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'STRLEN'");
            }
            const std::string key = args[1].str;
            std::shared_lock<std::shared_mutex> rdlock(m_mutex);
            auto it = m_store.find(key);
            if (it == m_store.end())
            {
                if (m_hash.find(key) != m_hash.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_lists.find(key) != m_lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_zset.find(key) != m_zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::integer(0);
            }
            rdlock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(m_mutex);
            if (it == m_store.end())
            {
                if (m_hash.find(key) != m_hash.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_lists.find(key) != m_lists.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                if (m_zset.find(key) != m_zset.end())
                {
                    return RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value");
                }

                return RespValue::integer(0);
            }
            auto expire_it = m_expire.find(key);
            if (expire_it != m_expire.end() && expire_it->second < SteadyClock::now())
            {
                m_expire.erase(expire_it);
                m_store.erase(it);
                return RespValue::integer(0);
            }
            return RespValue::integer(it->second.size());
        }
        else if (cmd == "TYPE") // TYPE key
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'TYPE'");
            }
            const std::string key = args[1].str;
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            if (m_store.find(key) != m_store.end())
            {
                return RespValue::bulk_string("string");
            }
            if (m_hash.find(key) != m_hash.end())
            {
                return RespValue::bulk_string("hash");
            }
            if (m_lists.find(key) != m_lists.end())
            {
                return RespValue::bulk_string("list");
            }
            if (m_zset.find(key) != m_zset.end())
            {
                return RespValue::bulk_string("zset");
            }
            return RespValue::null_bulk();
        }
        return RespValue::error("ERR unknown command");
    }

    template <typename T>
    Task<void> CommandHandler<T>::handleClient(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(xx::g_logger) << "handleClient begin, fd=" << sock->getSocketfd();
        RespStreamParser parser;
        const size_t MAX_COMMAND_SIZE = 1024 * 1024;
        do
        {
            char tmp[4096];
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

            BLUE_LOG_DEBUGE(xx::g_logger) << "Received " << ret << " bytes from fd " << sock->getSocketfd();

            // 喂入数据
            if (!parser.feed({tmp, static_cast<size_t>(ret)}))
            {
                BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                             << "] 缓冲区溢出，关闭连接";
                break;
            }

            // 处理所有完整的命令
            RespValue cmd;
            while (parser.next(cmd))
            {
                if (cmd.type == RespValue::Type::ARRAY && cmd.arr.size() > 1000)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "[client " << sock->getSocketfd()
                                                << "] 命令数组过大: " << cmd.arr.size();
                    auto error_resp = RespValue::error("ERR command too large");
                    auto data = RespValue::encode(error_resp);
                    size_t sent = 0;
                    while (sent < data.size())
                    {
                        ssize_t n = co_await sock->send(data.data() + sent, data.size() - sent);
                        if (n <= 0)
                        {
                            BLUE_LOG_ERROR(xx::g_logger) << "Send failed";
                            break;
                        }
                        sent += n;
                    }
                    continue;
                }

                // 执行命令
                auto start = std::chrono::steady_clock::now();
                auto response = execute(cmd.arr);
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
                if (duration.count() > 100)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "Slow command: " << duration.count() << "us";
                }
                auto data = RespValue::encode(response);

                BLUE_LOG_DEBUGE(xx::g_logger) << "Sending response, size=" << data.size();
                // 发送响应
                size_t sent = 0;
                while (sent < data.size())
                {
                    ssize_t n = co_await sock->send(data.data() + sent, data.size() - sent);
                    if (n <= 0)
                    {
                        BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                                     << "] 发送失败";
                        break;
                    }
                    sent += n;
                }
                if (sent == data.size())
                {
                    BLUE_LOG_DEBUGE(xx::g_logger) << "Response sent successfully";
                }
            }

            // 定期检查缓冲区大小，防止内存泄漏
            if (parser.bufferSize() > MAX_COMMAND_SIZE)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                             << "] 命令过大，关闭连接";
                break;
            }

        } while (true);
        sock->close();
        TcpServer<T>::subConnection();
        if (TcpServer<T>::getConnection() == 0)
        {
            bool end = co_await TcpServer<T>::stop();
            if (end)
                BLUE_LOG_INFO(xx::g_logger) << "tcpserver stoped";
        }
        BLUE_LOG_INFO(xx::g_logger) << "handleClient end, fd=" << sock->getSocketfd();
        co_return;
    }
}
