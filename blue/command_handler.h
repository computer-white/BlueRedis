#pragma once
#include <memory>
#include <regex>
#include <chrono>
#include <list>
#include <iomanip>
#include <iterator>
#include <unordered_set>
#include "spscqueue.h"
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
    constexpr int DB_COUNT = 16;
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
        RespValue execute(std::vector<RespValue> args, MSocket::MSocketPtr sock, bool RecordAOF = true);

    protected:
        /**
         * @brief 处理client事件
         * @param sock 客户端 socket fd
         */
        virtual Task<void> handleClient(MSocket::MSocketPtr sock) override;

        /**
         * @brief 获取客户端最大连接数量
         */
        virtual const int getMaxClientCount() const noexcept override { return m_config.maxClients; }

    private:
        /**
         * @brief 定期删除过期的命令
         */
        Task<void> expireTime();
        void removeExpireCycle();

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

        /**
         * @brief 持久化到文件
         */
        void saveToFile();

        /**
         * @brief 从文件加载
         */
        void loadFromFile();

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
         * @brief 获取key版本
         * @param key 键值
         * @param sock 封装的socket 类智能指针
         */
        uint64_t getKeyVersion(const std::string &key, MSocket::MSocketPtr sock)
        {
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);

            auto it = shard.store.find(key);
            if (it != shard.store.end())
            {
                return std::hash<std::string>{}(it->second);
            }
            return 0;
        }

        /**
         * @brief 同步推送订阅消息给订阅者
         * @param sock 封装的socket 类智能指针
         * @param channel 频道
         * @param message 消息
         */
        void publishMessage(MSocket::MSocketPtr sock,
                            const std::string &channel,
                            const std::string &message)
        {
            if (!sock || !sock->isConnected())
            {
                return;
            }
            // 构造 RESP 消息
            std::vector<RespValue> msg;
            msg.push_back(RespValue::bulk_string("message"));
            msg.push_back(RespValue::bulk_string(channel));
            msg.push_back(RespValue::bulk_string(message));

            std::string data = RespValue::encode(RespValue::array(std::move(msg)));

            // 使用阻塞发送，确保立即发出
            size_t sent = 0;
            while (sent < data.size())
            {
                ssize_t n = ::send(sock->getSocketfd(),
                                   data.data() + sent,
                                   data.size() - sent,
                                   MSG_NOSIGNAL); // MSG_NOSIGNAL 防止 SIGPIPE
                if (n <= 0)
                {
                    BLUE_LOG_ERROR(xx::g_logger) << "Failed to send subscription message";
                    return;
                }
                sent += n;
            }

            BLUE_LOG_DEBUGE(xx::g_logger) << "Sent subscription message to " << sock->getSocketfd();
            return;
        }

    private:
        // commandserver 配置
        struct CommConfig
        {
            int32_t maxClients = 1000; // 最大客户端数量
            int32_t timeout_s = 0;     // 客户端超时(s)
            std::string save;          // 保存策略
        };

        CommConfig m_config;

        struct AOFConfig
        {
            // AOF
            bool aof_enabled = false;                    // 是否开启aof
            std::string aof_filename = "appendonly.aof"; // 文件模板名
            size_t max_file_size = 1024 * 1024;          // 每个文件最大大小
            size_t max_file_number = 5;                  // 保留5个aof文件
            std::string aof_sync = "everysec";           // 保存策略,always, everysec, no
        };

        AOFConfig m_aof_config;

    private:
        std::array<std::array<DataShard, SHARD_COUNT>, DB_COUNT> m_dbs; // 数据库
        std::atomic<uint32_t> m_commands{0};                            // 总共命令数量
        std::atomic<bool> m_shutdown{false};                            // 服务器关闭标识
        std::string m_password = "";                                    // 管理员密码
        std::atomic<time_t> m_last_time{0};                             // 上一次保存rdb文件时间
        std::atomic<bool> m_bgsave_running{false};                      // 后台保存rdb
        MSocket::MSocketWPtr m_admin_sock;                              // 用于同一时间只能一个管理员上线
    private:
        // 订阅模式
        // 订阅功能
        std::shared_mutex m_channels_mutex;
        // 频道 -> 订阅者列表
        std::unordered_map<std::string, std::vector<MSocket::MSocketWPtr>> m_channels;

        std::shared_mutex m_patterns_mutex;
        // 模式订阅,支持通配符
        std::unordered_map<std::string, std::vector<MSocket::MSocketWPtr>> m_patterns;

    private:
        // 慢查询日志
        struct SlowLogEntry
        {
            int64_t id;                                      // 自增id
            std::chrono::system_clock::time_point timestamp; // 时间戳
            std::chrono::microseconds duration;              // 执行时间(微秒)
            std::string command;                             // 命令字符串
            std::string client_ip;                           // 客户端ip
        };

        SPSCQueue<SlowLogEntry, 2048> m_slow_logs;   // 慢查询日志队列
        std::shared_mutex m_slow_logs_cache_mutex;   // 日志锁
        std::vector<SlowLogEntry> m_slow_logs_cache; // 用于查询的缓存

        std::atomic<int64_t> m_slow_log_id{0};              // 自增ID
        std::atomic<int64_t> m_slow_log_slower_than{10000}; // 阈值（微秒），默认10ms
        std::atomic<size_t> m_slow_log_max_len{128};        // 最大保存条数

        /**
         * @brief 将慢查询日志队列内容同步进入slow_logs_cache
         */
        void syncSlowLogs()
        {
            std::unique_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
            SlowLogEntry entry;
            size_t max_len = m_slow_log_max_len.load(std::memory_order_acquire);
            while (m_slow_logs.pop(entry))
            {
                m_slow_logs_cache.push_back(std::move(entry));
                if (m_slow_logs_cache.size() > max_len)
                {
                    m_slow_logs_cache.erase(m_slow_logs_cache.begin());
                }
            }
        }

    private:
        // MONITOR 模式
        std::shared_mutex m_monitor_mutex;                      // MONITOR 锁
        std::vector<MSocket::MSocketWPtr> m_monitor_clients;    // MONITOR 客户端列表
        std::atomic<bool> m_push_monitor{true};                 // 是否推送给monitor

        /**
         * @brief 推送消息给monitor_client
         * @param cmd 命令
         * @param sock 被推送的客户端
         * @note 过期的sock会被清理
         */
        void pushToMonitor(const std::string &cmd, MSocket::MSocketPtr sock)
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

        /**
         * @brief 集中清理过期的monitor_clients
         */
        void removeMonitor()
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

    private:
        // AOF
        std::shared_mutex m_aof_mutex;           // 互斥锁
        std::string m_aof_current_filename;      // 当前文件名
        std::ofstream m_aof_file;                // 当前打开的文件流
        size_t m_aof_file_idx = 0;               // 当前文件编号
        TimePoint m_last_aof_sync;               // 最新一次写入时间
        std::atomic<bool> m_aof_rotating{false}; // 轮转标志

        // AOF 相关方法
        /**
         * @brief 初始化AOF
         */
        void initAOF();

        /**
         * @brief 追加命令到AOF
         * @param cmd 命令
         */
        void appendToAOF(const std::string &cmd);

        /**
         * @brief 加载AOF
         */
        void loadAOF();

        /**
         * @brief 判断是否是写命令
         * @param cmd 命令
         */
        bool isWriteCommand(const std::string &cmd);

        /**
         * @brief 格式化命令为RESP字符转
         * @param args 命令数组
         */
        std::string formatCommand(const std::vector<RespValue> &args);

        /**
         * @brief 按策略循环后台同步AOF文件
         */
        Task<void> aofSyncLoop();

        /**
         * @brief 轮转
         */
        void rotateAOF();

        /**
         * @brief 获取文件名
         */
        std::string getAOFFilename(int index);

        /**
         * @brief 判断文件是否是旧文件并执行删除，若最后文件名可使用返回true
         * @param filename 需要判断的文件名
         * @return true 表示可以使用
         */
        bool cleanupOldAOFs(const std::string &filename);
    };

    template <typename T>
    CommandHandler<T>::CommandHandler(int level, int option_name, T option, IOManager *manager,
                                      IOManager *acceptmanager)
        : TcpServer<T>(level, option_name, option, manager, acceptmanager)
    {
        loadFromFile();
        loadAOF();
        initAOF(); // 初始化AOF,追加打开AOF文件,并开启AOF同步协程
        IOManager::GetThis()->schedule(expireTime());
        if (s_admin_password.empty())
        {
            s_admin_password = "admin123";
        }
        m_password = s_admin_password;
        m_last_aof_sync = SteadyClock::now();
    }

    template <typename T>
    CommandHandler<T>::~CommandHandler()
    {
        m_shutdown.store(true, std::memory_order_release);

        // 等待其他协程退出
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        saveToFile();
        if (m_aof_file.is_open())
        {
            m_aof_file.flush();
            m_aof_file.close();
        }
    }

    template <typename T>
    void CommandHandler<T>::removeExpireCycle()
    {
        int count = 0;
        auto now = SteadyClock::now();
        for (int db = 0; db < DB_COUNT; db++)
        {
            for (auto &shards : m_dbs[db])
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
    RespValue CommandHandler<T>::execute(std::vector<RespValue> args, MSocket::MSocketPtr sock, bool RecordAOF)
    {
        auto start = SteadyClock::now();

        auto return_with_slowlog = [&](RespValue resp) -> RespValue
        {
            auto end = SteadyClock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            if (duration.count() > m_slow_log_slower_than.load(std::memory_order_acquire))
            {
                std::string cmd_str;
                for (size_t i = 0; i < args.size(); i++)
                {
                    if (i > 0)
                    {
                        cmd_str += " ";
                    }
                    cmd_str += args[i].str;
                }
                SlowLogEntry entry{
                    ++m_slow_log_id,
                    std::chrono::system_clock::now(),
                    duration,
                    cmd_str,
                    sock->getRemoteAddress()->toString()};

                m_slow_logs.push(entry);
            }

            // 写命令记录进入AOF
            if (args.empty())
            {
                return resp;
            }
            std::string cmd = args[0].str;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            if (RecordAOF && isWriteCommand(cmd))
            {
                std::string aof_cmds = formatCommand(args);
                appendToAOF(aof_cmds);
            }

            // 每300条命令异步保存进入rbg
            if (m_commands.load(std::memory_order_acquire) % 300 == 0)
            {
                std::thread([this]()
                            { saveToFile(); })
                    .detach();
            }
            return resp;
        };

        // 不记录AOF模式,设置推送给monitor为false,即不推送
        if (!RecordAOF)
        {
            m_push_monitor.store(false,std::memory_order_release);
        }
        else
        {
            m_push_monitor.store(true, std::memory_order_release);
        }

        if (args.empty())
        {
            return return_with_slowlog(RespValue::error("ERR empty command"));
        }

        std::string cmd = args[0].str;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "PING") // PING [message]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 1 || args.size() > 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'PING'"));
            }
            if (args.size() == 1)
                return return_with_slowlog(RespValue::simple_string("PONG"));
            else
                return return_with_slowlog(RespValue::bulk_string(args[1].str));
        }
        else if (cmd == "AUTH") // AUTH password
        {
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'AUTH'"));
            }
            if (args[1].str == sock->getClientPassword())
            {
                sock->setClientlevel(1);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            if (args[1].str == m_password)
            {
                if (!m_admin_sock.expired())
                {
                    return return_with_slowlog(RespValue::error("ERR admin already logged in elsewhere"));
                }
                m_admin_sock = sock;
                sock->setClientlevel(2);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            return RespValue::error("ERR invalid password");
        }
        else if (cmd == "SELECT") // SELECT [count]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SELECT'"));
            }
            int db;
            try
            {
                db = std::stoi(args[1].str);
                if (db < 0 || db >= DB_COUNT)
                {
                    return return_with_slowlog(RespValue::error("ERR DB index is out of range"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR invalid DB index"));
            }
            sock->setClientId(db);
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "CLIENT") // CLIENT GETNAME/(SETNAME [name])
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT'"));
            }
            std::string subcmd = args[1].str;
            std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
            if (subcmd == "SETNAME")
            {
                if (args.size() != 3)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT SETNAME'"));
                }
                sock->setClientName(args[2].str);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else if (subcmd == "GETNAME")
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT GETNAME'"));
                }
                if (sock->getClientName().empty())
                {
                    return return_with_slowlog(RespValue::null_bulk());
                }
                return return_with_slowlog(RespValue::bulk_string(sock->getClientName()));
            }
            else if (subcmd == "LIST")
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT LIST'"));
                }
                std::string result;
                result += "name=" + sock->getClientName() + " ";
                result += "addr=" + sock->getRemoteAddress()->toString() + " ";
                result += "subScription=" + std::to_string((int)(sock->inSubScription())) + " ";
                result += "transaction=" + std::to_string((int)(sock->inTransaction())) + " ";
                result += "monitor=" + std::to_string((int)(sock->inMonitorMode())) + "\r\n";
                return return_with_slowlog(RespValue::bulk_string(result));
            }
            return return_with_slowlog(RespValue::error("ERR wrong arguments for 'CLIENT'"));
        }
        else if (cmd == "CONFIG") // CONFIG (GET [...])/SET [...], 获取或设置客户端的配置信息
        {
            // 同一时刻只能存在一个管理员，并且由于CommandHandler只有一个实例化，所以修改和获取不需要锁
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CONFIG'"));
            }
            std::string subcmd = args[1].str;
            std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
            if (subcmd == "GET")
            {
                if (args.size() != 3)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CONFIG GET'"));
                }
                std::string pattern = args[2].str;
                std::vector<RespValue> result;

                if (pattern == "*" || pattern == "clientpass")
                {
                    result.push_back(RespValue::bulk_string("clientpass"));
                    result.push_back(RespValue::bulk_string(sock->getClientPassword()));
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
                if (pattern == "*" || pattern == "slowlog-log-slower-than" || pattern == "slowlog-*")
                {
                    result.push_back(RespValue::bulk_string("slowlog-log-slower-than"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_slow_log_slower_than.load(std::memory_order_acquire))));
                }
                if (pattern == "*" || pattern == "slowlog-max-len" || pattern == "slowlog-*")
                {
                    result.push_back(RespValue::bulk_string("slowlog-max-len"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_slow_log_max_len.load(std::memory_order_acquire))));
                }
                if (pattern == "*" || pattern == "aof-enabled" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-enabled"));
                    result.push_back(RespValue::bulk_string(m_aof_config.aof_enabled ? "yes" : "no"));
                }
                if (pattern == "*" || pattern == "aof-filename" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-filename"));
                    result.push_back(RespValue::bulk_string(m_aof_config.aof_filename));
                }
                if (pattern == "*" || pattern == "aof-sync" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-sync"));
                    result.push_back(RespValue::bulk_string(m_aof_config.aof_sync));
                }
                if (pattern == "*" || pattern == "aof-max_file_size" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_file_size"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_aof_config.max_file_size)));
                }
                if (pattern == "*" || pattern == "aof-max_file_number" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_file_number"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_aof_config.max_file_number)));
                }
                return return_with_slowlog(RespValue::array(std::move(result)));
            }
            else if (subcmd == "SET")
            {
                if (args.size() != 4)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CONFIG SET'"));
                }

                std::string param = args[2].str;
                std::string value = args[3].str;

                if (param == "clientpass")
                {
                    sock->setClientPassword(value);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (!isAdmin(sock))
                {
                    return return_with_slowlog(RespValue::error("ERR authentication required"));
                }
                if (param == "maxclients")
                {
                    try
                    {
                        int newmax = std::stoi(value);
                        if (newmax <= 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR invalid maxclients value"));
                        }
                        if (newmax < TcpServer<T>::getConnection())
                        {
                            return return_with_slowlog(RespValue::error("ERR maxclients can't be less than current connections"));
                        }
                        m_config.maxClients = newmax;
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "timeout")
                {
                    try
                    {
                        int timeout = std::stoi(value);
                        if (timeout < 0)
                        {
                            return RespValue::error("ERR invalid timeout value");
                        }
                        m_config.timeout_s = timeout;
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "slowlog-log-slower-than")
                {
                    try
                    {
                        int64_t val = std::stoll(value);
                        if (val < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value must be >= 0"));
                        }
                        m_slow_log_slower_than.store(val, std::memory_order_release);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "slowlog-max-len")
                {
                    try
                    {
                        size_t val = std::stoul(value);
                        if (val <= 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value must be > 0"));
                        }
                        m_slow_log_max_len.store(val, std::memory_order_release);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "aof-enabled")
                {
                    if (value == "yes" || value == "1")
                    {
                        m_aof_config.aof_enabled = true;
                        initAOF();
                    }
                    else if (value == "no" || value == "0")
                    {
                        m_aof_config.aof_enabled = false;
                        if (m_aof_file.is_open())
                        {
                            m_aof_file.close();
                        }
                    }
                    else
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid value"));
                    }
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-sync")
                {
                    if (value == "always" || value == "everysec" || value == "no")
                    {
                        m_aof_config.aof_sync = value;
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    return return_with_slowlog(RespValue::error("ERR invalid sync mode"));
                }
                if (param == "aof-filename")
                {
                    if (value.empty())
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid filename"));
                    }
                    m_aof_config.aof_filename = value;
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_file_size")
                {
                    int64_t val;
                    try
                    {
                        val = std::stoi(value);
                        if (val < 1024 * 1024)
                        {
                            return return_with_slowlog(RespValue::error("ERR max_file_size value too small"));
                        }
                    }
                    catch(...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }

                    m_aof_config.max_file_size = val;
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_file_number")
                {
                    int val;
                    try
                    {
                        val = std::stoi(value);
                        if (val < 5)
                        {
                            return return_with_slowlog(RespValue::error("ERR max_file_number value too small"));
                        }
                    }
                    catch(...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                    m_aof_config.max_file_number = val;
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                return return_with_slowlog(RespValue::error("ERR Unsupported CONFIG parameter: " + param));
            }
            return return_with_slowlog(RespValue::error("ERR wrong arguments for 'CONFIG'"));
        } // string
        else if (cmd == "SET") // SET key val [EX [s]]/[PX [ms]]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SET'"));
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key, sock);
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
                            return return_with_slowlog(RespValue::error("ERR value can't be nagative"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
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
                            return return_with_slowlog(RespValue::error("ERR value can't be nagative"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                    }
                    shards.expire[key] = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
                }
            }
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "GET") // GET key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'GET'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto expire_it = shards.expire.find(key);
            if (expire_it != shards.expire.end() && expire_it->second < SteadyClock::now())
            {
                shards.expire.erase(expire_it);
                shards.store.erase(it);
                return return_with_slowlog(RespValue::null_bulk());
            }
            const std::string val = it->second;
            return return_with_slowlog(RespValue::bulk_string(val));
        }
        else if (cmd == "MSET") // MSET key val [key val]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3 || (args.size() & 1) != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'MSET'"));
            }

            for (size_t i = 1; i < args.size(); i += 2)
            {
                const std::string key = args[i].str;
                const std::string val = args[i + 1].str;
                auto &shards = getShard(key, sock);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                shards.store[key] = val;
            }
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "MGET") // MGET key [key...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'MGET'"));
            }
            std::vector<RespValue> results;

            for (size_t i = 1; i < args.size(); i++)
            {
                auto &shards = getShard(args[i].str, sock);
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
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "GETSET") // GETSET key val 返回旧值设置新值
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'GETSET'"));
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = val;
                return return_with_slowlog(RespValue::bulk_string(val));
            }
            std::string ans = it->second;
            it->second = val;
            return return_with_slowlog(RespValue::bulk_string(ans));
        }
        else if (cmd == "APPEND") // APPEND key val
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'APPEND'"));
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key, sock);
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
            return return_with_slowlog(RespValue::integer(shards.store[key].size()));
        }
        else if (cmd == "SETNX") // SETNX key val, 没有才设置
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'SETNX'"));
            }
            const std::string key = args[1].str;
            const std::string val = args[2].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = val;
                return return_with_slowlog(RespValue::integer(1));
            }
            return return_with_slowlog(RespValue::integer(0));
        }
        else if (cmd == "EXISTS") // EXISTS key [key...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'EXISTS'"));
            }
            int64_t count = 0;
            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string key = args[i].str;
                auto &shards = getShard(key, sock);
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
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "DEL") // DEL key [key...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'DEL'"));
            }
            int count = 0;
            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string key = args[i].str;
                auto &shards = getShard(key, sock);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto it = shards.store.find(key);
                if (it != shards.store.end())
                {
                    shards.store.erase(it);
                    shards.expire.erase(key);
                    count++;
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "HSET") // HSET key field value [field value ...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HSET'"));
            }
            int count = 0;
            std::string key = args[1].str;
            for (size_t i = 2; i < args.size(); i += 2)
            {
                auto &shards = getShard(key, sock);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto &field = args[i].str;
                auto &value = args[i + 1].str; // size 是偶数所以不会出界
                if (shards.hash[key].find(field) == shards.hash[key].end())
                {
                    ++count; // 新字段
                }
                shards.hash[key][field] = value;
            }
            return return_with_slowlog(RespValue::integer(count));
        } // 哈希集合操作
        else if (cmd == "HGET") // HGET key field
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HGET'"));
            }
            const std::string key = args[1].str;
            const std::string field = args[2].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto fit = it->second.find(field);
            if (fit == it->second.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            const std::string val = fit->second;
            return return_with_slowlog(RespValue::bulk_string(val));
        }
        else if (cmd == "HGETALL") // HGETALL key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HGETALL'"));
            }
            std::vector<RespValue> result;
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return return_with_slowlog(RespValue::array(std::move(result)));
            }
            for (auto &[field, value] : it->second)
            {
                result.push_back(RespValue::bulk_string(field));
                result.push_back(RespValue::bulk_string(value));
            }
            return return_with_slowlog(RespValue::array(std::move(result)));
        }
        else if (cmd == "HDEL") // HDEL key field [field ...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HDEL'"));
            }
            int count = 0;
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return return_with_slowlog(RespValue::integer(count));
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                count += it->second.erase(args[i].str);
            }
            if (it->second.empty())
            {
                shards.hash.erase(it);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "HLEN") // HLEN key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HLEN'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> rdlock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "HEXISTS") // HEXISTS key field
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HEXISTS'"));
            }
            const std::string key = args[1].str;
            const std::string field = args[2].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }
                return return_with_slowlog(RespValue::integer(0));
            }
            auto filed_it = it->second.find(field);
            if (filed_it == it->second.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "HKEYS") // HKEYS key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HKEYS'"));
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                return return_with_slowlog(RespValue::array(std::move(results)));
            }
            for (auto &[field, _] : it->second)
            {
                results.push_back(RespValue::bulk_string(field));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "HVALS") // HVALS key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HKEYS'"));
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                return return_with_slowlog(RespValue::array(std::move(results)));
            }
            for (auto &[_, val] : it->second)
            {
                results.push_back(RespValue::bulk_string(val));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "KEYS") // KEYS *
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'KEYS'"));
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
            result.reserve(MAX_KEYS);

            // 遍历所有分片
            for (auto &shard : m_dbs[sock->getClientId()])
            {
                if (result.size() >= MAX_KEYS)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "KEYS command truncated at " << MAX_KEYS << " keys";
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

            return return_with_slowlog(RespValue::array(std::move(result)));
        } // 链表操作
        else if (cmd == "LPUSH") // LPUSH key val [val...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LPUSH'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &lhs = shards.lists[key];
            for (size_t i = 2; i < args.size(); i++)
            {
                lhs.push_front(args[i].str);
            }
            return return_with_slowlog(RespValue::integer(lhs.size()));
        }
        else if (cmd == "RPUSH") // RPUSH key val [val...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RPUSH'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &lhs = shards.lists[key];
            for (size_t i = 2; i < args.size(); i++)
            {
                lhs.push_back(args[i].str);
            }
            return return_with_slowlog(RespValue::integer(lhs.size()));
        }
        else if (cmd == "LPOP") // LPOP key [count]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 || args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LPOP'"));
            }
            int count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return return_with_slowlog(RespValue::error("ERR count can't be nagative"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end() || it->second.empty())
            {
                return return_with_slowlog(RespValue::null_bulk());
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
                return return_with_slowlog(results[0]);
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "RPOP") // RPOP key [count]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 || args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LPOP'"));
            }
            int count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return return_with_slowlog(RespValue::error("ERR count can't be nagative"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end() || it->second.empty())
            {
                return return_with_slowlog(RespValue::null_bulk());
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
                return return_with_slowlog(results[0]);
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "LLEN") // LLEN key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LLEN'"));
            }
            const std::string key = args[1].str;
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "LINSERT") // LINSERT key [BEFORE/AFTER] pivot val, pivot 不存在返回-1
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 5)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LINSERT'"));
            }
            const std::string key = args[1].str;
            const std::string pos = args[2].str;
            const std::string pivot = args[3].str;
            const std::string val = args[4].str;

            auto &shard = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            std::list<std::string> &lists = it->second;
            auto list_it = lists.begin();
            for (; list_it != lists.end(); list_it++)
            {
                if (*list_it == pivot)
                {
                    break;
                }
            }

            if (list_it == lists.end())
            {
                return return_with_slowlog(RespValue::integer(-1)); // pivot不存在
            }
            if (pos == "AFTER")
            {
                list_it++;
            }
            lists.insert(list_it, val);
            return return_with_slowlog(RespValue::integer(lists.size()));
        }
        else if (cmd == "LINDEX") // LINDEX key index
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LINDEX'"));
            }
            const std::string key = args[1].str;
            int64_t idx = 0;
            try
            {
                idx = std::stoll(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            size_t size = it->second.size();
            if (idx < 0)
            {
                idx += size;
            }
            if (idx < 0)
            {
                idx = 0;
            }
            auto list_it = it->second.begin();
            while (idx--)
            {
                list_it++;
            }
            return return_with_slowlog(RespValue::bulk_string(*list_it));
        }
        else if (cmd == "LSET") // LSET key index val
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LSET'"));
            }
            const std::string key = args[1].str;
            const std::string val = args[3].str;
            int64_t idx = 0;
            try
            {
                idx = std::stoll(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            size_t size = it->second.size();
            if (idx < 0)
            {
                idx += size;
            }
            if (idx < 0)
            {
                idx = 0;
            }
            auto list_it = it->second.begin();
            while (idx--)
            {
                list_it++;
            }
            *list_it = val;
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "RPOPLPUSH" || cmd == "LPOPRPUSH") // RPOPLPUSH source destination
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RPOPLPUSH'"));
            }
            const std::string source_key = args[1].str;
            const std::string dest_key = args[2].str;
            if (source_key == dest_key)
            {
                return return_with_slowlog(RespValue::integer(1));
            }
            int src_idx = getShardIndex(source_key);
            int dest_idx = getShardIndex(dest_key);
            if (src_idx > dest_idx)
            {
                std::swap(src_idx, dest_idx);
            }
            auto &src_shard = getShard(source_key, sock);
            auto &dest_shard = getShard(dest_key, sock);
            std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (src_idx != dest_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
            }
            auto src_it = src_shard.lists.find(source_key);
            if (src_it == src_shard.lists.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }

            auto dest_it = dest_shard.lists.find(dest_key);
            if (dest_it == dest_shard.lists.end())
            {
                dest_shard.lists[dest_key] = std::list<std::string>();
                dest_it = dest_shard.lists.find(dest_key);
            }
            if (cmd == "RPOPLPUSH")
            {
                const std::string tem = src_it->second.back();
                src_it->second.pop_back();
                dest_it->second.push_front(tem);
            }
            else
            {
                const std::string tem = src_it->second.front();
                src_it->second.pop_front();
                dest_it->second.push_back(tem);
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "LRANGE") // LRANGE key start stop
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LRANGE'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            int start, stop;
            try
            {
                start = std::stoi(args[2].str);
                stop = std::stoi(args[3].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end())
            {
                return return_with_slowlog(RespValue::array({}));
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
                return return_with_slowlog(RespValue::array({}));
            }
            std::vector<RespValue> result;
            auto iter = it->second.begin();
            std::advance(iter, start);
            for (int i = start; i <= stop && iter != it->second.end(); i++, iter++)
            {
                result.push_back(RespValue::bulk_string(*iter));
            }
            return return_with_slowlog(RespValue::array(std::move(result)));
        } // 有序集合操作
        else if (cmd == "ZADD") // ZADD key score member [score member]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZADD'"));
            }
            int count = 0;
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
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
                    return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
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
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "ZRANGE") // ZRANGE key start stop [WITHSCORES]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() > 5)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZRANGE'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            int start, stop;
            try
            {
                start = std::stoi(args[2].str);
                stop = std::stoi(args[3].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset.find(key);
            if (it == shards.zset.end())
            {
                return return_with_slowlog(RespValue::array({}));
            }
            bool withscores = (args.size() == 5 && (args[4].str == "WITHSCORES" || args[4].str == "withscores"));
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
                return return_with_slowlog(RespValue::array({}));
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
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "ZREM") // ZREM key member [member...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZREM'"));
            }
            int count = 0;
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset.find(key);
            if (it == shards.zset.end())
            {
                return return_with_slowlog(RespValue::integer(count));
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
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "ZSCORE") // // ZSCORE key member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ASCORE'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset_score.find(key);
            if (it == shards.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &skiplist_map = it->second;
            auto sit = skiplist_map.find(args[2].str);
            if (sit != skiplist_map.end())
            {
                return return_with_slowlog(RespValue::bulk_string(format_score(sit->second)));
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZRANK") // ZRANK key member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZRANK'"));
            }
            const std::string key = args[1].str, member = args[2].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset_score.find(key);
            if (it == shards.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &score_map = it->second;
            auto sit = score_map.find(member);
            if (sit != score_map.end())
            {
                // key  = {score,member}
                int rank = shards.zset[key].getRank({sit->second, member});
                if (rank >= 0)
                {
                    return return_with_slowlog(RespValue::integer(rank));
                }
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZINCRBY") // ZINCRBY key incr member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZINCRBY'"));
            }
            const std::string key = args[1].str;
            const std::string member = args[3].str;
            int64_t incr;
            try
            {
                incr = std::stoll(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            auto &shard = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset.find(key);
            if (it == shard.zset.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto sit = shard.zset_score.find(key);
            if (sit == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &skiplist = it->second;
            auto &scores_map = sit->second;
            if (scores_map.find(member) != scores_map.end())
            {
                ZSetKey old_val(scores_map[member], member);
                scores_map[member] += incr;
                skiplist.remove(old_val);
                skiplist.insert({scores_map[member], member}, member);
                return return_with_slowlog(RespValue::bulk_string(format_score(scores_map[member])));
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZINCRBYFLOAT") // ZINCRBYFLOAT
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZINCRBY'"));
            }
            const std::string key = args[1].str;
            const std::string member = args[3].str;
            double incr;
            try
            {
                incr = std::stod(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a float or out of range"));
            }
            auto &shard = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset.find(key);
            if (it == shard.zset.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto sit = shard.zset_score.find(key);
            if (sit == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &skiplist = it->second;
            auto &scores_map = sit->second;
            if (scores_map.find(member) != scores_map.end())
            {
                ZSetKey old_val(scores_map[member], member);
                scores_map[member] += incr;
                skiplist.remove(old_val);
                skiplist.insert({scores_map[member], member}, member);
                return return_with_slowlog(RespValue::bulk_string(format_score(scores_map[member])));
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZCOUNT") // ZCOUNT key min max
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZCOUNT'"));
            }
            const std::string key = args[1].str;
            double min = 0, max = 0;
            try
            {
                min = std::stod(args[2].str);
                max = std::stod(args[3].str);
                if (min > max)
                {
                    return return_with_slowlog(RespValue::error("ERR min can't greater than max"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
            }
            int64_t count = 0;
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset_score.find(key);
            if (it == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            for (const auto &[_, score] : it->second)
            {
                if (score >= min && score <= max)
                {
                    count++;
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "ZRANGEBYSCORE") // ZRANGEBYSCORE key min max [WITHSCORE]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() > 5)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZRANGEBYSCORE'"));
            }
            const std::string key = args[1].str;
            double min = 0, max = 0;
            try
            {
                min = std::stod(args[2].str);
                max = std::stod(args[3].str);
                if (min > max)
                {
                    return return_with_slowlog(RespValue::error("ERR min can't greater than max"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
            }
            std::vector<RespValue> results;
            bool withscore = (args.size() == 5 && (args[4].str == "WITHSCORE" || args[4].str == "withscore"));
            auto &shard = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset_score.find(key);
            if (it == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::array({}));
            }
            for (const auto &[member, score] : it->second)
            {
                if (score >= min && score <= max)
                {
                    results.push_back(RespValue::bulk_string(member));
                    if (withscore)
                    {
                        results.push_back(RespValue::bulk_string((format_score(score))));
                    }
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "ZREMRANGEBYSCORE") // ZREMRANGEBYSCORE key min max
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZREMRANGEBYSCORE'"));
            }
            const std::string key = args[1].str;
            double min = 0, max = 0;
            try
            {
                min = std::stod(args[2].str);
                max = std::stod(args[3].str);
                if (min > max)
                {
                    return return_with_slowlog(RespValue::error("ERR min can't greater than max"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
            }
            int64_t count = 0;
            auto &shard = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset.find(key);
            if (it == shard.zset.end())
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            auto sit = shard.zset_score.find(key);
            if (sit == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            auto &skiplist = it->second;
            auto &scores = sit->second;
            std::vector<std::string> tem;
            for (const auto &[member, score] : scores)
            {
                if (score >= min && score <= max)
                {
                    ZSetKey old_val(score, member);
                    skiplist.remove(old_val);
                    tem.push_back(member);
                    count++;
                }
            }
            for (auto &mem : tem)
            {
                scores.erase(mem);
            }
            if (scores.empty())
            {
                shard.zset.erase(it);
                shard.zset_score.erase(sit);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "INCR") // INCR key 自增
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'INCR'"));
            }
            const std::string key = args[1].str;
            int64_t val = 0;
            auto &shards = getShard(key, sock);
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
                    return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                }
            }
            val++;
            shards.store[key] = std::to_string(val);
            return return_with_slowlog(RespValue::integer(val));
        }
        else if (cmd == "INCRBY") // INCR key integer 自增或自减 integer
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'INCRBY'"));
            }
            const std::string key = args[1].str;
            int64_t increment;
            try
            {
                increment = std::stoll(args[2].str);
            }
            catch (const std::exception &e)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            auto &shards = getShard(key, sock);
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
                    return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                }
            }
            val += increment;
            shards.store[key] = std::to_string(val);
            return return_with_slowlog(RespValue::integer(val));
        }
        else if (cmd == "STRLEN") // STRLEN key, 返回key对于val的长度
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'STRLEN'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                if (shards.hash.find(key) != shards.hash.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                return return_with_slowlog(RespValue::integer(0));
            }
            auto expire_it = shards.expire.find(key);
            if (expire_it != shards.expire.end() && expire_it->second < SteadyClock::now())
            {
                shards.expire.erase(expire_it);
                shards.store.erase(it);
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "TYPE") // TYPE key, key的类型
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'TYPE'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            if (shards.store.find(key) != shards.store.end())
            {
                return return_with_slowlog(RespValue::bulk_string("string"));
            }
            if (shards.hash.find(key) != shards.hash.end())
            {
                return return_with_slowlog(RespValue::bulk_string("hash"));
            }
            if (shards.lists.find(key) != shards.lists.end())
            {
                return return_with_slowlog(RespValue::bulk_string("list"));
            }
            if (shards.zset.find(key) != shards.zset.end())
            {
                return return_with_slowlog(RespValue::bulk_string("zset"));
            }
            if (shards.sets.find(key) != shards.sets.end())
            {
                return return_with_slowlog(RespValue::bulk_string("set"));
            }
            return return_with_slowlog(RespValue::null_bulk());
        } // 无序集合操作
        else if (cmd == "SADD") // SADD key member [member...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of argument for 'SADD'"));
            }
            const std::string key = args[1].str;
            int32_t count = 0;
            auto &shards = getShard(key, sock);
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
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "SMEMBERS") // SMEMBERS key, 返回key所有的成员
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SMEMBERS'"));
            }
            const std::string key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::array(std::move(results)));
            }
            for (auto &member : it->second)
            {
                results.push_back(RespValue::bulk_string(member));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SREM") // SREM key member [member...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SREM'"));
            }
            const std::string key = args[1].str;
            int32_t count = 0;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                count += it->second.erase(args[i].str);
            }
            if (it->second.empty())
            {
                shards.sets.erase(it);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "SISMEMBER") // SISMEMBER key member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR number wrong of arguments for 'SISMEMBER'"));
            }
            const std::string key = args[1].str;
            const std::string member = args[2].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.count(member)));
        }
        else if (cmd == "SCARD") // SCARD key, key的集合大小
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR number wrong of arguments for 'SCARD'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "SRANDMEMBER") // SRANDMEMBER key [count], 随机返回count个member,count < 0可包含重复值, > 0不重复
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 && args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SRANDMEMBER'"));
            }
            int32_t count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 0;
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                if (args.size() == 2)
                {
                    return return_with_slowlog(RespValue::null_bulk());
                }
                else
                {
                    return return_with_slowlog(RespValue::array({}));
                }
            }
            const auto &set = it->second;
            std::vector<std::string> members(set.begin(), set.end());
            std::vector<RespValue> results;
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                return return_with_slowlog(RespValue::bulk_string(members[idx]));
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
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SPOP") // SPOP key [count], 随机返回并删除count个member,count只能大于0
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 && args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SRANDMEMBER'"));
            }
            int32_t count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 0;
                if (count < 0)
                {
                    return return_with_slowlog(RespValue::error("ERR value can't be nagative"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                if (args.size() == 2)
                {
                    return return_with_slowlog(RespValue::null_bulk());
                }
                else
                {
                    return return_with_slowlog(RespValue::array({}));
                }
            }
            auto &set = it->second;
            std::vector<std::string> members(set.begin(), set.end());
            std::vector<RespValue> results;
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                set.erase(members[idx]);
                return return_with_slowlog(RespValue::bulk_string(members[idx]));
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
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SDIFF") // SDIFF key [key...] 差集
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SDIFF'"));
            }
            const std::string key = args[1].str;
            auto &shard = getShard(key, sock);
            std::vector<RespValue> results;
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            for (const auto &member : shard.sets[key])
            {
                bool ok = true;
                for (size_t i = 2; i < args.size(); i++)
                {
                    const std::string tem_key = args[i].str;
                    auto &tem_shard = getShard(tem_key, sock);
                    std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                    auto &tem_members = tem_shard.sets[tem_key];
                    if (tem_members.contains(member))
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                {
                    results.push_back(RespValue::bulk_string(member));
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SINTER") // SINTER key [key...] 交集
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SINTER'"));
            }
            const std::string key = args[1].str;
            auto &shard = getShard(key, sock);
            std::vector<RespValue> results;
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            for (const auto &member : shard.sets[key])
            {
                bool ok = true;
                for (size_t i = 2; i < args.size(); i++)
                {
                    const std::string tem_key = args[i].str;
                    auto &tem_shard = getShard(tem_key, sock);
                    std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                    auto &tem_members = tem_shard.sets[tem_key];
                    if (!tem_members.contains(member))
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                {
                    results.push_back(RespValue::bulk_string(member));
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SUNION") // SUNION key [key...] 并集
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SUNION'"));
            }
            const std::string key = args[1].str;
            auto &shard = getShard(key, sock);
            std::unordered_set<std::string> results_set;
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            for (const auto &member : shard.sets[key])
            {
                results_set.insert(member);
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string tem_key = args[i].str;
                auto &tem_shard = getShard(tem_key, sock);
                std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                auto &tem_members = tem_shard.sets[tem_key];
                for (const auto &tem_member : tem_members)
                {
                    results_set.insert(tem_member);
                }
            }
            lock.unlock();
            std::vector<RespValue> results;
            for (const auto &member : results_set)
            {
                results.push_back(RespValue::bulk_string(member));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SMOVE") // SMOVE source destination member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SMOVE'"));
            }
            const std::string source_key = args[1].str;
            const std::string destination_key = args[2].str;
            if (source_key == destination_key)
            {
                return return_with_slowlog(RespValue::integer(1));
            }
            const std::string member = args[3].str;
            int src_shard_idx = getShardIndex(source_key);
            int dest_shard_idx = getShardIndex(destination_key);
            if (src_shard_idx > dest_shard_idx)
            {
                std::swap(src_shard_idx, dest_shard_idx);
            }
            auto &src_shard = getShard(source_key, sock);
            auto &dest_shard = getShard(destination_key, sock);

            std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (src_shard_idx != dest_shard_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
            }

            auto src_it = src_shard.sets.find(source_key);
            if (src_it == src_shard.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }

            auto dest_it = dest_shard.sets.find(destination_key);
            if (dest_it == dest_shard.sets.end())
            {
                dest_shard.sets[destination_key] = std::unordered_set<std::string>();
                dest_it = dest_shard.sets.find(destination_key);
            }
            // 检查member是否在源集合中
            if (src_it->second.find(member) == src_it->second.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            src_it->second.erase(member);
            dest_it->second.insert(member);
            if (src_it->second.empty())
            {
                src_shard.sets.erase(src_it);
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "FLUSHDB") // FLUSHDB [confirm], 清空数据库(还没有持久化)
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (isAdmin(sock))
            {
                if (args.size() < 1 || args.size() > 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'FLUSHDB'"));
                }
                for (auto &shards : m_dbs[sock->getClientId()])
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
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else if (sock->getClientlevel() == 1)
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR maybe need 'FLUSHAD CONFIRM"));
                }
                std::string confirm = args[1].str;
                std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::toupper);
                if (confirm == "CONFIRM")
                {
                    for (auto &shards : m_dbs[sock->getClientId()])
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
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                return return_with_slowlog(RespValue::error("ERR maybe need 'FLUSHDB CONFIRM'"));
            }
            return return_with_slowlog(RespValue::error("ERR authentication required"));
        }
        else if (cmd == "FLUSHDBALL") // FLUSHDBALL, 清空所有数据库
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (isAdmin(sock))
            {
                if (args.size() < 1 || args.size() > 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'FLUSHDBALL'"));
                }
                for (int db = 0; db < DB_COUNT; db++)
                {
                    for (auto &shards : m_dbs[db])
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
                }
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else if (sock->getClientlevel() == 1)
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR maybe need 'FLUSHADALL CONFIRM"));
                }
                std::string confirm = args[1].str;
                std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::toupper);
                if (confirm == "CONFIRM")
                {
                    for (int db = 0; db < DB_COUNT; db++)
                    {
                        for (auto &shards : m_dbs[db])
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
                    }
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                return return_with_slowlog(RespValue::error("ERR authentication required, maybe need 'FLUSHADALL CONFIRM"));
            }
            return return_with_slowlog(RespValue::error("ERR authentication required"));
        }
        else if (cmd == "DBSIZE") // DBSIZE, 当前数据库大小
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'DBSIZE'"));
            }
            int64_t count = 0;
            for (auto &shards : m_dbs[sock->getClientId()])
            {
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                count += shards.store.size() + shards.lists.size() + shards.hash.size() + shards.zset.size();
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "EXPIRE") // EXPIRE key seconds, 设置key的过期时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'EXPIRE'"));
            }
            int64_t second;
            try
            {
                second = std::stoll(args[2].str);
                if (second <= 0)
                {
                    return return_with_slowlog(RespValue::error("ERR invalid expire time"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            shards.expire[key] = SteadyClock::now() + std::chrono::seconds(second);
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "TTL") // TTL key, 查看key的过期时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'TTL'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            auto expire_it = shards.expire.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(-2));
            }
            else if (expire_it == shards.expire.end())
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            // 计算剩余秒数
            auto now = SteadyClock::now();
            if (now >= expire_it->second)
            {
                return return_with_slowlog(RespValue::integer(-2));
            }

            auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                                 expire_it->second - now)
                                 .count();

            return return_with_slowlog(RespValue::integer(remaining));
        }
        else if (cmd == "PEXPIRE") // PEXPIRE key milliseconds
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'PEXPIRE'"));
            }
            int64_t milliseconds;
            try
            {
                milliseconds = std::stoll(args[2].str);
                if (milliseconds <= 0)
                {
                    return return_with_slowlog(RespValue::error("ERR invalid expire time"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            shards.expire[key] = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "PTTL") // PTTL key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'PTTL'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            auto expire_it = shards.expire.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(-2));
            }
            else if (expire_it == shards.expire.end())
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            // 计算剩余秒数
            auto now = SteadyClock::now();
            if (now >= expire_it->second)
            {
                return return_with_slowlog(RespValue::integer(-2));
            }

            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 expire_it->second - now)
                                 .count();

            return return_with_slowlog(RespValue::integer(remaining));
        }
        else if (cmd == "PERSIST") // PERSIST key, 撤销key的过期时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of argument for 'PERSIST'"));
            }
            const std::string key = args[1].str;
            auto &shards = getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            auto expire_it = shards.expire.find(key);
            // 不存在或没有过期时间
            if (it == shards.store.end() || expire_it == shards.expire.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            shards.expire.erase(expire_it);
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "RENAME") // RENAME key newkey
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RENAME'"));
            }
            const std::string key = args[1].str;
            const std::string newkey = args[2].str;

            if (key == newkey)
            {
                return return_with_slowlog(RespValue::simple_string("OK"));
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
            auto &m_shards = m_dbs[sock->getClientId()];
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
                return return_with_slowlog(RespValue::error("ERR no such key"));
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
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "RENAMENX") // RENAMENX key newkey 当newkey不存在时创建
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RENAME'"));
            }
            const std::string key = args[1].str;
            const std::string newkey = args[2].str;

            if (key == newkey)
            {
                return return_with_slowlog(RespValue::integer(1));
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
            auto &m_shards = m_dbs[sock->getClientId()];
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
                return return_with_slowlog(RespValue::error("ERR no such key"));
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
                return return_with_slowlog(RespValue::integer(0));
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
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "RANDOMKEY") // RANDOMKEY, 随机返回一个key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RANDOMKEY'"));
            }
            std::vector<std::string> all_keys;
            for (auto &shards : m_dbs[sock->getClientId()])
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
                return return_with_slowlog(RespValue::null_bulk());
            }
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, all_keys.size() - 1);
            return return_with_slowlog(RespValue::bulk_string(all_keys[dis(gen)]));
        }
        else if (cmd == "INFO") // INFO 返回服务器信息
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'INFO'"));
            }
            std::string info;
            // Server
            info += "# Server\r\n";
            info += "redis_version:1.0.0\r\n";
            info += "tcp_port:6666\r\n";
            info += "\r\n";

            // Client
            info += "# Client\r\n";
            info += "connections:" + std::to_string(TcpServer<T>::getConnection()) + "\r\n";
            info += "maxclient:" + std::to_string(m_config.maxClients) + "\r\n";
            info += "reject_connections:" + std::to_string(TcpServer<T>::getRejectConnection()) + "\r\n";
            info += "\r\n";

            // AOF
            info += "# AOF\r\n";
            info += "aof_enabled:" + std::string(m_aof_config.aof_enabled ? "1" : "0") + "\r\n";
            info += "aof_sync:" + m_aof_config.aof_sync + "\r\n";
            info += "aof_current_file:" + m_aof_current_filename + "\r\n";
            info += "aof_file_index:" + std::to_string(m_aof_file_idx) + "\r\n";
            info += "aof_current_size:" + std::to_string(m_aof_file.is_open() ? (size_t)m_aof_file.tellp() : 0) + "\r\n";
            info += "aof_max_file_size:" + std::to_string(m_aof_config.max_file_size) + "\r\n";
            info += "aof_max_files:" + std::to_string(m_aof_config.max_file_number) + "\r\n";
            info += "\r\n";

            // Monitor
            info += "# Monitor\r\n";
            info += "monitor_clients:" + std::to_string(m_monitor_clients.size()) + "\r\n";
            info += "\r\n";

            // Stats
            info += "# Stats\r\n";
            info += "total_connections_received:" + std::to_string(TcpServer<T>::getConnection()) + "\r\n";
            info += "total_commands_processed:" + std::to_string(m_commands.load(std::memory_order_acquire)) + "\r\n";
            info += "\r\n";

            // Memory
            info += "# Memory\r\n";
            size_t total_keys = 0;
            for (auto &shard : m_dbs[sock->getClientId()])
            {
                std::shared_lock lock(shard.mutex);
                total_keys += shard.store.size() + shard.hash.size() + shard.lists.size() + shard.sets.size() + shard.zset.size();
            }
            info += "total_keys:" + std::to_string(total_keys) + "\r\n";
            return return_with_slowlog(RespValue::bulk_string(info));
        }
        else if (cmd == "SAVE") // SAVE 持久化
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SAVE'"));
            }
            saveToFile();
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "BGSAVE") // BGSAVE 后台异步保存
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'BGSAVE'"));
            }
            if (m_bgsave_running.load(std::memory_order_acquire))
            {
                return return_with_slowlog(RespValue::error("ERR Background save already in progress"));
            }
            m_bgsave_running.store(true, std::memory_order_release);
            std::thread([this]
                        {
                saveToFile();
                m_bgsave_running.store(false,std::memory_order_release);
                BLUE_LOG_INFO(xx::g_logger) << "BGSAVE completed"; })
                .detach();
            return return_with_slowlog(RespValue::simple_string("Background saving started"));
        }
        else if (cmd == "LASTSAVE") // LASTSAVE 上一次保存成功时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LASTSAVE'"));
            }
            return return_with_slowlog(RespValue::integer(m_last_time.load(std::memory_order_acquire)));
        }
        else if (cmd == "LASTSAVE1") // LASTVATE1 上一次保存成功时间(北京时间)
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LASTSAVE1'"));
            }
            std::time_t beijing_t = m_last_time.load(std::memory_order_acquire) + 8 * 3600;
            std::tm time_local;
#ifdef _WIN32
            gmtime_s(&time_local, &beijing_t);
#else
            gmtime_r(&beijing_t, &time_local);
#endif
            std::ostringstream os;
            os << std::put_time(&time_local, "%Y-%m-%d %H:%M:%S");
            return return_with_slowlog(RespValue::bulk_string(os.str()));
        }
        else if (cmd == "COMMAND")
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'COMMAND'"));
            }

            std::vector<RespValue> commands;

            // 返回所有支持的命令列表
            static const std::vector<std::string> cmd_list = {
                // 连接
                "PING", "AUTH", "SELECT", "CLIENT", "CONFIG",
                // string
                "SET", "GET", "MSET", "MGET", "APPEND", "SETNX",
                "INCR", "INCRBY", "DEL", "EXISTS", "STRLEN", "TYPE",
                "GETSET",
                // hash
                "HSET", "HGET", "HGETALL", "HDEL", "HLEN", "HEXISTS", "HKEYS", "HVALS",
                // list
                "LPUSH", "RPUSH", "LPOP", "RPOP", "LRANGE", "LLEN", "LINSERT", "LINDEX", "LSET",
                "LPOPRPUSH", "RPOPLPUSH",
                // set
                "SADD", "SMEMBERS", "SREM", "SISMEMBER", "SCARD", "SRANDMEMBER", "SPOP", "SDIFF", "SINTER", "SUNION", "SMOVE",
                // zset
                "ZADD", "ZRANGE", "ZREM", "ZSCORE", "ZRANK", "ZINCRBY", "ZCOUNT", "ZRANGEBYSCORE", "ZREMRANGEBYSCORE",
                "ZINCRBYFLOAT",
                // server
                "KEYS", "FLUSHDB", "FLUSHDBALL", "DBSIZE", "INFO", "SAVE", "BGSAVE", "LASTSAVE",
                "LASTSAVE1", "ECHO", "TIME", "LOCALTIME", "SHUTDOWN", "COMMAND",
                "RENAME", "RENAMENX", "RANDOMKEY", "EXPIRE", "TTL", "PEXPIRE", "PTTL",
                "PERSIST",
                // 事务模式
                "MULTI", "EXEC", "DISCARD", "WATCH", "UNWATCH",
                // 订阅模式
                "SUBSCRIBE", "PUBLISH", "UNSUBSCRIBE",
                // 慢查询
                "SLOWLOG",
                // 监控模式
                "MONITOR"};

            for (const auto &name : cmd_list)
            {
                commands.push_back(RespValue::bulk_string(name));
            }

            return return_with_slowlog(RespValue::array(std::move(commands)));
        }
        else if (cmd == "ECHO") // ECHO meaasge
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ECHO'"));
            }
            return return_with_slowlog(RespValue::bulk_string(args[1].str));
        }
        else if (cmd == "TIME") // TIME, 返回当前服务器的秒和微妙
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'TIME'"));
            }
            auto now = SteadyClock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
            std::vector<RespValue> results;
            results.push_back(RespValue::bulk_string(std::to_string(seconds)));
            results.push_back(RespValue::bulk_string(std::to_string(microseconds)));
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "LOCALTIME") // LOCALTIME, 返回当前北京时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LOCALTIME'"));
            }
            auto now = std::chrono::system_clock::now();
            auto now_t = std::chrono::system_clock::to_time_t(now);

            std::time_t beijing_t = now_t + 8 * 3600;
            std::tm time_local;
#ifdef _WIN32
            gmtime_s(&time_local, &beijing_t);
#else
            gmtime_r(&beijing_t, &time_local);
#endif
            std::ostringstream os;
            os << std::put_time(&time_local, "%Y-%m-%d %H:%M:%S");
            return return_with_slowlog(RespValue::bulk_string(os.str()));
        }
        else if (cmd == "WATCH") // WATCH key [key...], 监视key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'WATCH'"));
            }
            sock->clearWatchedKey();

            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string key = args[i].str;
                sock->addWatchKey(key, getKeyVersion(key, sock));
            }
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "UNWATCH") // UNWATCH, 取消所有监视
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'UNWATCH'"));
            }
            sock->clearWatchedKey();
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "SLOWLOG") // SLOWLOG [GET/LEN/RESET]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SLOWLOG'"));
            }
            std::string sub_cmd = args[1].str;
            std::transform(sub_cmd.begin(), sub_cmd.end(), sub_cmd.begin(), ::toupper);
            if (sub_cmd == "GET")
            {
                // 同步数据
                syncSlowLogs();

                int64_t count = 10;
                if (args.size() >= 3)
                {
                    try
                    {
                        count = std::stoll(args[2].str);
                        if (count < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR count must be >= 0"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
                    }
                }

                std::vector<RespValue> results;
                std::shared_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
                size_t start = m_slow_logs_cache.size() - std::min((size_t)(count), m_slow_logs_cache.size());
                for (size_t i = start; i < m_slow_logs_cache.size(); i++)
                {
                    const auto &entry = m_slow_logs_cache[i];
                    std::vector<RespValue> log_entry;

                    // ID
                    log_entry.push_back(RespValue::integer(entry.id));

                    // 时间戳微秒
                    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(
                                  entry.timestamp.time_since_epoch())
                                  .count();
                    log_entry.push_back(RespValue::integer(ts));

                    // 执行时间(微秒)
                    log_entry.push_back(RespValue::integer(entry.duration.count()));

                    // 命令
                    log_entry.push_back(RespValue::bulk_string(entry.command));

                    // 客户端ip
                    log_entry.push_back(RespValue::bulk_string(entry.client_ip));

                    results.push_back(RespValue::array(std::move(log_entry)));
                }
                return return_with_slowlog(RespValue::array(std::move(results)));
            }

            else if (sub_cmd == "LEN")
            {
                syncSlowLogs();
                std::shared_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
                return return_with_slowlog(RespValue::integer(m_slow_logs_cache.size()));
            }
            else if (sub_cmd == "RESET")
            {
                std::unique_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
                m_slow_logs_cache.clear();
                // 清空 SPSCQueue
                SlowLogEntry entry;
                while (m_slow_logs.pop(entry))
                {
                }
                m_slow_log_id.store(0, std::memory_order_release);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else
            {
                return return_with_slowlog(RespValue::error("ERR unknown SLOWLOG subcommand"));
            }
        }
        else if (cmd == "MONITOR") // MONITOR   实时监控和调试服务器
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'MONITOR'"));
            }

            {
                std::unique_lock<std::shared_mutex> lock(m_monitor_mutex);
                m_monitor_clients.push_back(sock);
            }

            sock->setMonitorMode(true);

            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "AOFROTATE") // AOFROTATE 异步轮转AOF日志(不阻塞协程)
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'AOFROTATE'"));
            }
            if (!m_aof_config.aof_enabled) 
            {
                return return_with_slowlog(RespValue::error("ERR AOF is disabled"));
            }
            if (m_aof_rotating.load(std::memory_order_acquire)) 
            {
                return return_with_slowlog(RespValue::error("ERR AOF rotation already in progress"));
            }
            std::thread([this]{
                rotateAOF();
                BLUE_LOG_INFO(xx::g_logger) << "AOF rotation finished";
            }).detach();
            return return_with_slowlog(RespValue::simple_string("AOF rotation started"));
        }
        else if (cmd == "SHUTDOWN") // SHUTDOWN 关闭服务器,如果连接数为0
        {
            if (!isAdmin(sock))
            {
                return return_with_slowlog(RespValue::error("ERR permission denied"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SHUTDOWN'"));
            }
            m_shutdown.store(true, std::memory_order_release);
            return return_with_slowlog(RespValue::bulk_string("OK - waiting for clients to disconnect"));
        }
        return return_with_slowlog(RespValue::error("ERR unknown command"));
    }

    template <typename T>
    Task<void> CommandHandler<T>::handleClient(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(xx::g_logger) << "handleClient begin, fd=" << sock->getSocketfd();
        // BLUE_LOG_INFO(xx::g_logger) << "remote address: " <<  sock->getRemoteAddress()->toString();
        // BLUE_LOG_INFO(xx::g_logger) << "local address : " <<  sock->getLocalAddress()->toString();

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
            RespValue cmd_Resp;
            int cmd_count = 0;

            // 喂入数据
            while (parser.next(cmd_Resp))
            {
                auto copy_arr = cmd_Resp.arr;
                if (copy_arr.empty())
                {
                    continue;
                }
                // 安全检查
                if (cmd_Resp.type == RespValue::Type::ARRAY && copy_arr.size() > 1000)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "[client " << sock->getSocketfd()
                                                << "] 命令数组过大: " << copy_arr.size();
                    auto error_resp = RespValue::error("ERR command too large");
                    batch_response += RespValue::encode(error_resp);
                    break;
                }

                // 命令cmd
                std::string cmd = copy_arr[0].str;
                std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

                blue::RespValue response;

                auto start = SteadyClock::now();
                std::chrono::_V2::steady_clock::time_point end;
                if (sock->inTransaction()) // 事务模式
                {
                    if (cmd == "EXEC") // EXEC
                    {
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() != 1)
                        {
                            response = RespValue::error("ERR wrong number of arguments for 'EXEC'");
                        }
                        else
                        {
                            sock->setVersionChecker([this, sock](const std::string &key) -> uint64_t
                                                    { return this->getKeyVersion(key, sock); });
                            if (sock->hasKeyModified())
                            {
                                sock->clearTransaction();
                                sock->clearWatchedKey();
                                response = RespValue::null_bulk();
                            }
                            else
                            {
                                std::vector<RespValue> results;
                                for (const auto &transaction : sock->getTransaction())
                                {
                                    auto response = execute(transaction, sock, true);
                                    results.push_back(response);
                                }
                                sock->clearTransaction();
                                sock->clearWatchedKey();
                                response = RespValue::array(std::move(results));
                            }
                        }
                        end = SteadyClock::now();
                    }
                    else if (cmd == "DISCARD") // DISCARD 清除所有事务,会退出事务模式
                    {
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() != 1)
                        {
                            response = RespValue::error("ERR wrong number of arguments for 'DISCARD'");
                        }
                        else
                        {
                            sock->clearTransaction();
                            response = RespValue::simple_string("OK");
                        }
                        end = SteadyClock::now();
                    }
                    else
                    {
                        sock->addTransaction(std::move(copy_arr)); // move后copy_arr为空
                        response = RespValue::simple_string("QUEUED");
                    }
                }
                else if (sock->inSubScription()) // 订阅模式
                {
                    if (cmd == "UNSUBSCRIBE") // UNSUBSCRIBE [channel...], 并退出订阅模式
                    {
                        std::vector<std::string> channels;
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() == 1) // 取消所有订阅
                        {
                            channels.assign(sock->getSubScriptionChannels().begin(), sock->getSubScriptionChannels().end());
                        }
                        else
                        {
                            for (size_t i = 1; i < copy_arr.size(); i++)
                            {
                                channels.push_back(copy_arr[i].str);
                            }
                        }

                        std::vector<RespValue> results;

                        for (const auto &channel : channels)
                        {
                            // 从全局列表删除
                            {
                                std::unique_lock<std::shared_mutex> lock(m_channels_mutex);
                                auto it = m_channels.find(channel);
                                if (it != m_channels.end())
                                {
                                    auto &subs = it->second;
                                    subs.erase(std::remove_if(subs.begin(), subs.end(), [sock](const auto &weak)
                                                              {
                                        auto ptr = weak.lock();
                                        return !ptr || ptr.get() == sock.get(); }),
                                               subs.end());
                                    if (subs.empty())
                                    {
                                        m_channels.erase(it);
                                    }
                                }
                            }
                            // 从连接订阅列表移除
                            sock->removeSubScriptionChannel(channel);

                            // 返回取消订阅消息
                            std::vector<RespValue> msg;
                            msg.push_back(RespValue::bulk_string("unsubscribe"));
                            msg.push_back(RespValue::bulk_string(channel));
                            msg.push_back(RespValue::integer(sock->getSubScriptionChannels().size()));
                            results.push_back(RespValue::array(std::move(msg)));
                        }
                        sock->endSubScription();
                        response = RespValue::array(std::move(results));
                        end = SteadyClock::now();
                    }
                    else if (cmd == "PING") // PING [message]
                    {
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() < 1 || copy_arr.size() > 2)
                        {
                            response = RespValue::error("ERR wrong number of arguments for 'PING'");
                        }
                        else
                        {
                            if (copy_arr.size() == 1)
                                response = RespValue::simple_string("PONG");
                            else
                                response = RespValue::bulk_string(copy_arr[1].str);
                        }
                        end = SteadyClock::now();
                    }
                    else
                    {
                        response = RespValue::error("ERR in SubScription, only 'UNSUBSCRIBE' and 'PING'");
                        end = SteadyClock::now();
                    }
                }
                else
                {
                    if (cmd == "MULTI") // MULTI, 进入事务模式
                    {
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() != 1)
                        {
                            response = RespValue::error("ERR wrong number of arguments for 'MULTI'");
                        }
                        else if (!sock->beginTransaction())
                        {
                            response = RespValue::error("ERR already in SubScription");
                        }
                        else
                        {
                            response = RespValue::simple_string("OK");
                        }
                        end = SteadyClock::now();
                    }
                    else if (cmd == "SUBSCRIBE") // SUBSCRIBE channel [channel...] 订阅channel 进入订阅模式
                    {
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() < 2)
                        {
                            response = RespValue::error("ERR wrong number of arguments for 'SUBSCRIBE'");
                        }
                        else if (!sock->beginSubScription())
                        {
                            response = RespValue::error("ERR already in Transaction");
                        }
                        else
                        {
                            std::vector<RespValue> results;

                            for (size_t i = 1; i < copy_arr.size(); i++)
                            {
                                const std::string channel = copy_arr[i].str;
                                // 添加到连接的订阅列表
                                sock->addSubScriptionChannel(channel);

                                // 添加到全局订阅列表
                                {
                                    std::unique_lock<std::shared_mutex> lock(m_channels_mutex);
                                    m_channels[channel].push_back(sock);
                                }

                                // 返回订阅成功消息
                                std::vector<RespValue> msg;
                                msg.push_back(RespValue::bulk_string("subscribe"));
                                msg.push_back(RespValue::bulk_string(channel));
                                msg.push_back(RespValue::integer(1)); // 当前订阅数

                                results.push_back(RespValue::array(std::move(msg)));
                            }
                            response = RespValue::array(std::move(results));
                        }
                        end = SteadyClock::now();
                    }
                    else if (cmd == "PUBLISH") // PUBLISH channel message 发布channel 内容为message
                    {
                        if (sock->getClientlevel() < 1)
                        {
                            response = RespValue::error("ERR authentication required");
                        }
                        else if (copy_arr.size() != 3)
                        {
                            response = RespValue::error("ERR wrong number of arguments for 'PUBLISH'");
                        }
                        else
                        {
                            const std::string channel = copy_arr[1].str;
                            const std::string message = copy_arr[2].str;

                            int receiver_count = 0;
                            std::vector<MSocket::MSocketWPtr> subscribers;

                            // 获取订阅者
                            {
                                std::shared_lock lock(m_channels_mutex);
                                auto it = m_channels.find(channel);
                                if (it != m_channels.end())
                                {
                                    subscribers = it->second;
                                }
                            }

                            // 发送消息给所有订阅者
                            for (auto &weak_sock : subscribers)
                            {
                                auto sub_sock = weak_sock.lock();
                                if (sub_sock && sub_sock->isConnected())
                                {
                                    // 同步发送
                                    publishMessage(sub_sock, channel, message);
                                    receiver_count++;
                                }
                            }
                            response = RespValue::integer(receiver_count);
                        }
                        end = SteadyClock::now();
                    }
                    else
                    {
                        // 执行普通命令
                        response = execute(std::move(copy_arr), sock, true); // move后copy_arr为空
                    }
                }

                std::string cmd_str;
                for (size_t i = 0; i < cmd_Resp.arr.size(); i++)
                {
                    if (i > 0)
                    {
                        cmd_str += " ";
                    }
                    cmd_str += cmd_Resp.arr[i].str;
                }

                // 这些命令或事务模式或订阅模式的命令都需要额外设置慢查询
                if (cmd == "PUBLISH" || cmd == "SUBSCRIBE" || cmd == "MULTI" || sock->inSubScription() ||
                    sock->inTransaction())
                {
                    // 记录慢查询
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    if (duration.count() > m_slow_log_slower_than.load(std::memory_order_acquire))
                    {
                        SlowLogEntry entry{
                            ++m_slow_log_id,
                            std::chrono::system_clock::now(),
                            duration,
                            cmd_str,
                            sock->getRemoteAddress()->toString()};

                        m_slow_logs.push(entry);
                    }
                }

                if (response.str != "QUEUED" && m_push_monitor.load(std::memory_order_acquire))
                {
                    // 推送消息给监控客户端
                    pushToMonitor(cmd_str, sock);
                }

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
        // 检查是否是管理员连接断开
        auto admin = m_admin_sock.lock();
        if (admin && admin.get() == sock.get())
        {
            m_admin_sock.reset();
        }
        // 关闭前清理订阅
        for (const auto &channel : sock->getSubScriptionChannels())
        {
            std::unique_lock<std::shared_mutex> lock(m_channels_mutex);
            auto it = m_channels.find(channel);
            if (it != m_channels.end())
            {
                auto &subs = it->second;
                subs.erase(std::remove_if(subs.begin(), subs.end(), [sock](const auto &weak)
                                          {
                    auto ptr = weak.lock();
                    return !ptr || ptr.get() == sock.get(); }),
                           subs.end());
                if (subs.empty())
                {
                    m_channels.erase(it);
                }
            }
        }
        sock->clearSubScription();
        // BLUE_LOG_INFO(xx::g_logger) << "one Client exit, fd:" << sock->getSocketfd();
        sock->close();

        // 删除过期的monitor
        removeMonitor();

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

    template <typename T>
    void CommandHandler<T>::saveToFile()
    {
        const std::string filename = "dump.rdb";
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
                    file << "DB|" << db << "|STR|" << key << "|" << value;

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
    void CommandHandler<T>::loadFromFile()
    {
        BLUE_LOG_INFO(xx::g_logger) << "loadFromFile";
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
                shard.store[key] = value;

                if (parts.size() >= 6)
                {
                    int64_t expire_time = std::stoll(parts[5]);
                    shard.expire[key] = TimePoint(std::chrono::nanoseconds(expire_time));
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
                shard.hash[key][field] = value;
            }
            else if (type == "LIST" && parts.size() >= 5)
            {
                std::string key = parts[3];
                std::string value = parts[4];

                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                shard.lists[key].push_back(value);
            }
            else if (type == "SET" && parts.size() >= 5)
            {
                std::string key = parts[3];
                std::string member = parts[4];

                int shard_idx = getShardIndex(key);
                auto &shard = target_db[shard_idx];
                std::unique_lock lock(shard.mutex);
                shard.sets[key].insert(member);
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
    void CommandHandler<T>::initAOF()
    {
        if (!m_aof_config.aof_enabled)
        {
            return;
        }

        if (m_aof_file_idx == 0) 
        {
            m_aof_file_idx = 1;
            m_aof_current_filename = getAOFFilename(1);
        }

        m_aof_file.open(m_aof_current_filename, std::ios::app | std::ios::binary);
        if (!m_aof_file)
        {
            BLUE_LOG_ERROR(xx::g_logger) << "Failed to open AOF file: " << m_aof_current_filename;
            m_aof_config.aof_enabled = false;
            return;
        }

        BLUE_LOG_INFO(xx::g_logger) << "AOF enabled, file: " << m_aof_current_filename;

        // 启动 AOF 同步协程(always 或 everysec)
        IOManager::GetThis()->schedule(aofSyncLoop());
    }

    template <typename T>
    void CommandHandler<T>::appendToAOF(const std::string &cmd)
    {
        if (!m_aof_config.aof_enabled || !m_aof_file.is_open())
        {
            return;
        }

        // 如果处在rotate中这里会阻塞住
        std::unique_lock<std::shared_mutex> lock(m_aof_mutex);

        m_aof_file << cmd;
    
        if (m_aof_config.aof_sync == "always")
        {
            m_aof_file.flush();
        }
        else if (m_aof_config.aof_sync == "everysec")
        {
            auto now = SteadyClock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                                now - m_last_aof_sync)
                                .count();
            if (duration >= 1)
            {
                m_aof_file.flush();
                m_last_aof_sync = now;
            }
        }

        size_t curr_size = m_aof_file.tellp();
        if (curr_size > m_aof_config.max_file_size && !m_aof_rotating.load(std::memory_order_acquire))
        {
            lock.unlock();
            rotateAOF();
        }
    }

    template <typename T>
    void CommandHandler<T>::loadAOF()
    {
        // 从所有现有的AOF文件加载
        std::vector<std::string> files;

        // 提前分配
        files.reserve(m_aof_config.max_file_number);

        int goodidx = 1;
        int idx = 1;
        // 遍历1-max_file_number之间的文件索引
        for (; idx <= m_aof_config.max_file_number; idx++)
        {
            std::string file_name = getAOFFilename(idx);
            std::ifstream test(file_name);
            if (test && test.good())
            {
                files.push_back(file_name);
                goodidx = idx;
                test.close();
            }
        }

        if (files.empty())
        {
            BLUE_LOG_INFO(xx::g_logger) << "No existing AOF file";
            m_aof_file_idx = 1;
            m_aof_current_filename = getAOFFilename(1);
            return;
        }

        m_aof_file_idx = goodidx;       // 必然有效(1-max_file_number)
        m_aof_current_filename = getAOFFilename(m_aof_file_idx);

        // 创建临时的socket对象(没有真正调用系统API创建socket fd)来载入
        auto temp_sock = MSocket::CreateTcpSocket();
        temp_sock->setClientlevel(1); // 跳过认证检查
        temp_sock->setClientId(0);    // 默认数据库 0

        int total_cmds = 0;
        int errors = 0;

        for (const auto& file_name : files)
        {
            std::ifstream file(file_name);
            if (!file)
            {
                BLUE_LOG_INFO(xx::g_logger) << "No existing AOF file";
                continue;
            }

            BLUE_LOG_INFO(xx::g_logger) << "Loading AOF: " << file_name;

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            file.close();

            if (content.empty())
            {
                BLUE_LOG_INFO(xx::g_logger) << "AOF file is empty";
                continue;
            }

            RespStreamParser parser;

            if (!parser.feed(content))
            {
                BLUE_LOG_INFO(xx::g_logger) << "Failed to parser AOF file";
                continue;
            }

            RespValue cmd_Resp;
            int count = 0;
            while (parser.next(cmd_Resp))
            {
                auto copy_arr = cmd_Resp.arr;
                if (copy_arr.empty() || cmd_Resp.type != RespValue::Type::ARRAY)
                {
                    continue;
                }
                try
                {
                    // 只载入数据,不计入AOF文件
                    execute(std::move(copy_arr), temp_sock, false);
                    count++;
                }
                catch (const std::exception &e)
                {
                    BLUE_LOG_ERROR(xx::g_logger) << "Failed to execute AOF command: " << e.what();
                    errors++;
                }
            }
            total_cmds += count;
            BLUE_LOG_INFO(xx::g_logger) << "Loaded " << count << " commands from AOF " << file_name;
        }

        BLUE_LOG_INFO(xx::g_logger) << "AOF loaded: " << total_cmds 
                                << " commands, errors: " << errors;
    }

    template <typename T>
    bool CommandHandler<T>::isWriteCommand(const std::string &cmd)
    {
        static const std::unordered_set<std::string> write_commands = {
            "SET", "SETNX", "MSET", "APPEND", "GETSET", "INCR", "INCRBY",
            "HSET", "HDEL",
            "LPUSH", "RPUSH", "LPOP", "RPOP", "LSET", "LINSERT", "LPOPRPUSH", "RPOPLPUSH",
            "SADD", "SREM", "SMOVE", "SPOP",
            "ZADD", "ZREM", "ZINCRBY", "ZREMRANGEBYSCORE", "ZINCRBYFLOAT",
            "DEL", "EXPIRE", "PEXPIRE", "PERSIST", "RENAME", "RENAMENX",
            "FLUSHDB", "FLUSHDBALL",
            // 处理select 来保证数据库会被正确初始化
            "SELECT"};
        // BLUE_LOG_INFO(xx::g_logger) << "cmd: " << cmd << "inwrite_commands: " << (int)(write_commands.find(cmd) != write_commands.end());
        return (bool)(write_commands.find(cmd) != write_commands.end());
    }

    template <typename T>
    std::string CommandHandler<T>::formatCommand(const std::vector<RespValue> &args)
    {
        std::string result;
        result += "*" + std::to_string(args.size()) + "\r\n";
        for (const auto &arg : args)
        {
            result += "$" + std::to_string(arg.str.size()) + "\r\n";
            result += arg.str + "\r\n";
        }
        return result;
    }

    template <typename T>
    Task<void> CommandHandler<T>::aofSyncLoop()
    {
        if (!m_aof_config.aof_enabled)
        {
            co_return;
        }

        while (!m_shutdown.load(std::memory_order_acquire))
        {
            co_await sleepFor(2);

            if (m_aof_config.aof_sync == "everysec") // 每秒刷新
            {
                std::unique_lock<std::shared_mutex> lock(m_aof_mutex);
                auto now = SteadyClock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_aof_sync).count();

                if (duration >= 1 && m_aof_file.is_open())
                {
                    m_aof_file.flush();
                    m_last_aof_sync = now;
                }
            }
            else if (m_aof_config.aof_sync == "always")
            {
                if (m_aof_file.is_open())
                {
                    m_aof_file.flush();
                }
            }
        }
        co_return;
    }

    template <typename T>
    void CommandHandler<T>::rotateAOF()
    {
        if (m_aof_config.max_file_number == 0) 
        {
            BLUE_LOG_ERROR(xx::g_logger) << "max_file_number is 0, cannot rotate";
            return;
        }
        // 处在轮转中
        if (m_aof_rotating.load(std::memory_order_acquire))
        {
            BLUE_LOG_DEBUGE(xx::g_logger) << "AOF rotation already in progress";
            return;
        }

        // 开始轮转
        m_aof_rotating.store(true, std::memory_order_release);

        BLUE_LOG_INFO(xx::g_logger) << "AOF rotation started, current file: "
                                    << m_aof_current_filename
                                    << ", idx: " << m_aof_file_idx
                                    << ", max_file_number: " << m_aof_config.max_file_number;

        std::unique_lock<std::shared_mutex> lock(m_aof_mutex);
        if (m_aof_file.is_open())
        {
            m_aof_file.flush();
            m_aof_file.close();
        }

        // 让索引落在1-max_file_number之间
        int next_idx = (m_aof_file_idx % m_aof_config.max_file_number) + 1;

        // 索引文件会在1-max_file_number之间循环,所以当返回一个已经存在的文件名，表示需要删除了
        std::string new_file = getAOFFilename(next_idx);

        // 调用后,new_file文件名一定可以使用了,因为要么是旧的被删除了(可能删除失败),要么是新的
        bool can_used = cleanupOldAOFs(new_file);

        // 如果删除文件失败,截断文件内容后以app方式打开
        if (!can_used)
        {
            std::ofstream truncate_file(new_file, std::ios::trunc | std::ios::binary);
            if (truncate_file) 
            {
                truncate_file.close();
                BLUE_LOG_INFO(xx::g_logger) << "Truncated old AOF file: " << new_file;
            } 
            else 
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Failed to truncate old AOF file: " << new_file;
                m_aof_config.aof_enabled = false;
                m_aof_rotating.store(false,std::memory_order_release);
                return;
            }
        }

        m_aof_file.open(new_file, std::ios::app | std::ios::binary);
        if (!m_aof_file)
        {
            BLUE_LOG_ERROR(xx::g_logger) << "Failed to open new AOF file: " << new_file;
            m_aof_config.aof_enabled = false;
            m_aof_rotating.store(false,std::memory_order_release);
            return;
        }

        m_aof_file_idx = next_idx;          // 更新当前的文件索引
        m_aof_current_filename = new_file;  // 更新当前的文件名
        m_aof_file.flush();
        lock.unlock();

        // 轮转结束
        m_aof_rotating.store(false,std::memory_order_release);
        BLUE_LOG_INFO(xx::g_logger) << "AOF rotation completed, new file: " << new_file;
    }

    template <typename T>
    std::string CommandHandler<T>::getAOFFilename(int index)
    {
        if (index == 1)
        {
            return m_aof_config.aof_filename;
        }
        return m_aof_config.aof_filename + "." + std::to_string(index);
    }

    template <typename T>
    bool CommandHandler<T>::cleanupOldAOFs(const std::string &filename)
    {
        // 检查文件是否存在
        std::ifstream test(filename);
        if (test.good()) 
        {
            test.close();
            if (remove(filename.c_str()) == 0) 
            {
                BLUE_LOG_INFO(xx::g_logger) << "Removed old AOF file: " << filename;
            } 
            else 
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Failed to remove old AOF file: " << filename;
                return false;
            }
        }
        return true;
    }
}
