#pragma once
#include <array>
#include <memory>
#include <regex>
#include <chrono>
#include <list>
#include <iomanip>
#include <unordered_set>
#include "blue/config.h"
#include "blue/task.h"
#include "blue/tcpServer.h"
#include "blue/resp_parser.h"
#include "blue/asyncio.h"
#include "blue/await.h"
#include "modules/subscription.h"
#include "modules/slowlog.h"
#include "modules/monitor.h"
#include "modules/AOF.h"
#include "blue/skiplist.h"
#ifdef COMMAND_TABLE
#include "command_table.h"
#include "command_register.h"
#else
#endif

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

    // 32个分片
    constexpr int SHARD_COUNT = 32;
    // 16个数据库
    constexpr int DB_COUNT = 16;

    // 单个分片的结构
    struct DataShard
    {
        using TimePoint = std::chrono::steady_clock::time_point;
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

    /**
     * @brief redis 服务器
     */
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
#ifdef COMMAND_TABLE
    private:
        static constexpr auto EVEN_VALIDATOR = [](size_t argc) -> bool
        {
            return argc >= 4 && (argc & 1) == 0; // HSET, ZADD: 4, 6, 8, ...
        };

        static constexpr auto ODD_VALIDATOR = [](size_t argc) -> bool
        {
            return argc >= 3 && (argc & 1) == 1; // MSET: 3, 5, 7, ...
        };

        static constexpr auto ONLY_ONE = [](size_t argc) -> bool
        { return argc == 1; };
        static constexpr auto ONLY_TWO = [](size_t argc) -> bool
        { return argc == 2; };
        static constexpr auto ONLY_THREE = [](size_t argc) -> bool
        { return argc == 3; };
        static constexpr auto ONLY_FOUR = [](size_t argc) -> bool
        { return argc == 4; };
        static constexpr auto ONLY_FIVE = [](size_t argc) -> bool
        { return argc == 5; };

        static constexpr auto ONLY_ONE_TWO = [](size_t argc) -> bool
        { return argc >= 1 && argc <= 2; };
        static constexpr auto ONLY_TWO_THREE = [](size_t argc) -> bool
        { return argc >= 2 && argc <= 3; };
        static constexpr auto ONLY_THREE_FOUR = [](size_t argc) -> bool
        { return argc >= 3 && argc <= 4; };
        static constexpr auto ONLY_THREE_SIX = [](size_t argc) -> bool
        { return argc >= 3 && argc <= 6; };
        static constexpr auto ONLY_FOUR_FIVE = [](size_t argc) -> bool
        { return argc >= 4 && argc <= 5; };

        static constexpr auto ONLY_MORE_TWO = [](size_t argc) -> bool
        { return argc >= 2; };
        static constexpr auto ONLY_MORE_THREE = [](size_t argc) -> bool
        { return argc >= 3; };

    public:
        using CommandHandlerFunc = blue::RespValue (*)(std::vector<RespValue> &,
                                                       MSocket::MSocketPtr,
                                                       bool,
                                                       CommandHandler<int> *);
        // 声明所有命令
        // connect
        REGISTER_COMMAND(PING, handlePING);
        REGISTER_COMMAND(AUTH, handleAUTH);
        REGISTER_COMMAND(SELECT, handleSELECT);
        REGISTER_COMMAND(CLIENT, handleCLIENT);
        REGISTER_COMMAND(CONFIG, handleCONFIG);

        // string
        REGISTER_COMMAND(SET, handleSET);
        REGISTER_COMMAND(GET, handleGET);
        REGISTER_COMMAND(MSET, handleMSET);
        REGISTER_COMMAND(MGET, handleMGET);
        REGISTER_COMMAND(GETSET, handleGETSET);
        REGISTER_COMMAND(APPEND, handleAPPEND);
        REGISTER_COMMAND(SETNX, handleSETNX);
        REGISTER_COMMAND(EXISTS, handleEXISTS);
        REGISTER_COMMAND(DEL, handleDEL);
        REGISTER_COMMAND(INCR, handleINCR);
        REGISTER_COMMAND(INCRBY, handleINCRBY);
        REGISTER_COMMAND(STRLEN, handleSTRLEN);
        REGISTER_COMMAND(TYPE, handleTYPE);

        // hash
        REGISTER_COMMAND(HSET, handleHSET);
        REGISTER_COMMAND(HGET, handleHGET);
        REGISTER_COMMAND(HGETALL, handleHGETALL);
        REGISTER_COMMAND(HDEL, handleHDEL);
        REGISTER_COMMAND(HLEN, handleHLEN);
        REGISTER_COMMAND(HEXISTS, handleHEXISTS);
        REGISTER_COMMAND(HKEYS, handleHKEYS);
        REGISTER_COMMAND(HVALS, handleHVALS);
        REGISTER_COMMAND(KEYS, handleKEYS);

        // list
        REGISTER_COMMAND(LPUSH, handleLPUSH);
        REGISTER_COMMAND(RPUSH, handleRPUSH);
        REGISTER_COMMAND(LPOP, handleLPOP);
        REGISTER_COMMAND(RPOP, handleRPOP);
        REGISTER_COMMAND(LLEN, handleLLEN);
        REGISTER_COMMAND(LINSERT, handleLINSERT);
        REGISTER_COMMAND(LINDEX, handleLINDEX);
        REGISTER_COMMAND(LSET, handleLSET);
        REGISTER_COMMAND(RPOPLPUSH, handleRPOPLPUSH);
        REGISTER_COMMAND(LPOPRPUSH, handleLPOPRPUSH);
        REGISTER_COMMAND(LRANGE, handleLRANGE);

        // zset
        REGISTER_COMMAND(ZADD, handleZADD);
        REGISTER_COMMAND(ZRANGE, handleZRANGE);
        REGISTER_COMMAND(ZREM, handleZREM);
        REGISTER_COMMAND(ZSCORE, handleZSCORE);
        REGISTER_COMMAND(ZRANK, handleZRANK);
        REGISTER_COMMAND(ZINCRBY, handleZINCRBY);
        REGISTER_COMMAND(ZINCRBYFLOAT, handleZINCRBYFLOAT);
        REGISTER_COMMAND(ZCOUNT, handleZCOUNT);
        REGISTER_COMMAND(ZRANGEBYSCORE, handleZRANGEBYSCORE);
        REGISTER_COMMAND(ZREMRANGEBYSCORE, handleZREMRANGEBYSCORE);

        // set
        REGISTER_COMMAND(SADD, handleSADD);
        REGISTER_COMMAND(SMEMBERS, handleSMEMBERS);
        REGISTER_COMMAND(SREM, handleSREM);
        REGISTER_COMMAND(SISMEMBER, handleSISMEMBER);
        REGISTER_COMMAND(SCARD, handleSCARD);
        REGISTER_COMMAND(SRANDMEMBER, handleSRANDMEMBER);
        REGISTER_COMMAND(SPOP, handleSPOP);
        REGISTER_COMMAND(SDIFF, handleSDIFF);
        REGISTER_COMMAND(SINTER, handleSINTER);
        REGISTER_COMMAND(SUNION, handleSUNION);
        REGISTER_COMMAND(SMOVE, handleSMOVE);

        // server
        REGISTER_COMMAND(FLUSHDB, handleFLUSHDB);
        REGISTER_COMMAND(FLUSHDBALL, handleFLUSHDBALL);
        REGISTER_COMMAND(DBSIZE, handleDBSIZE);
        REGISTER_COMMAND(EXPIRE, handleEXPIRE);
        REGISTER_COMMAND(TTL, handleTTL);
        REGISTER_COMMAND(PEXPIRE, handlePEXPIRE);
        REGISTER_COMMAND(PTTL, handlePTTL);
        REGISTER_COMMAND(PERSIST, handlePERSIST);
        REGISTER_COMMAND(RENAME, handleRENAME);
        REGISTER_COMMAND(RENAMENX, handleRENAMENX);
        REGISTER_COMMAND(RANDOMKEY, handleRANDOMKEY);
        REGISTER_COMMAND(INFO, handleINFO);
        REGISTER_COMMAND(SAVE, handleSAVE);
        REGISTER_COMMAND(BGSAVE, handleBGSAVE);
        REGISTER_COMMAND(LASTSAVE, handleLASTSAVE);
        REGISTER_COMMAND(LASTSAVE1, handleLASTSAVE1);
        REGISTER_COMMAND(COMMAND, handleCOMMAND);
        REGISTER_COMMAND(ECHO, handleECHO);
        REGISTER_COMMAND(TIME, handleTIME);
        REGISTER_COMMAND(LOCALTIME, handleLOCALTIME);
        REGISTER_COMMAND(WATCH, handleWATCH);
        REGISTER_COMMAND(UNWATCH, handleUNWATCH);
        REGISTER_COMMAND(SLOWLOG, handleSLOWLOG);
        REGISTER_COMMAND(MONITOR, handleMONITOR);
        REGISTER_COMMAND(AOFROTATE, handleAOFROTATE);
        REGISTER_COMMAND(SHUTDOWN, handleSHUTDOWN);

        // 插入所有命令
        static consteval auto buildCommandTable()
        {
            // 注释的走if-else直接判断
            blue::CommandTableBuilder<256> builder;
            // connect
            CMD_ENTRY(PING, handlePING, false, ONLY_ONE_TWO);
            CMD_ENTRY(AUTH, handleAUTH, false, ONLY_TWO);
            CMD_ENTRY(SELECT, handleSELECT, true, ONLY_TWO);
            CMD_ENTRY(CLIENT, handleCLIENT, false, ONLY_TWO_THREE);
            CMD_ENTRY(CONFIG, handleCONFIG, false, ONLY_THREE_FOUR);

            // string
            // CMD_ENTRY(SET, handleSET, true, ONLY_THREE_SIX);
            // CMD_ENTRY(GET, handleGET, false, ONLY_TWO);
            CMD_ENTRY(MSET, handleMSET, true, ODD_VALIDATOR);
            CMD_ENTRY(MGET, handleMGET, false, ONLY_MORE_TWO);
            CMD_ENTRY(GETSET, handleGETSET, true, ONLY_THREE);
            CMD_ENTRY(APPEND, handleAPPEND, true, ONLY_THREE);
            CMD_ENTRY(SETNX, handleSETNX, true, ONLY_THREE);
            CMD_ENTRY(EXISTS, handleEXISTS, false, ONLY_MORE_TWO);
            // CMD_ENTRY(DEL, handleDEL, true, ONLY_MORE_TWO);
            CMD_ENTRY(INCR, handleINCR, false, ONLY_TWO);
            CMD_ENTRY(INCRBY, handleINCRBY, false, ONLY_THREE);
            CMD_ENTRY(STRLEN, handleSTRLEN, false, ONLY_TWO);
            CMD_ENTRY(TYPE, handleTYPE, false, ONLY_TWO);

            // hash
            // CMD_ENTRY(HSET, handleHSET, true, EVEN_VALIDATOR);
            // CMD_ENTRY(HGET, handleHGET, false, ONLY_THREE);
            CMD_ENTRY(HGETALL, handleHGETALL, false, ONLY_TWO);
            CMD_ENTRY(HDEL, handleHDEL, true, ONLY_MORE_THREE);
            CMD_ENTRY(HLEN, handleHLEN, false, ONLY_TWO);
            CMD_ENTRY(HEXISTS, handleHEXISTS, false, ONLY_THREE);
            CMD_ENTRY(HKEYS, handleHKEYS, false, ONLY_TWO);
            CMD_ENTRY(HVALS, handleHVALS, false, ONLY_TWO);
            CMD_ENTRY(KEYS, handleKEYS, false, ONLY_TWO);

            // list
            // CMD_ENTRY(LPUSH, handleLPUSH, true, ONLY_MORE_THREE);
            CMD_ENTRY(RPUSH, handleRPUSH, true, ONLY_MORE_THREE);
            // CMD_ENTRY(LPOP, handleLPOP, true, ONLY_TWO_THREE);
            CMD_ENTRY(RPOP, handleRPOP, true, ONLY_TWO_THREE);
            CMD_ENTRY(LLEN, handleLLEN, false, ONLY_TWO);
            CMD_ENTRY(LINSERT, handleLINSERT, true, ONLY_FIVE);
            CMD_ENTRY(LINDEX, handleLINDEX, false, ONLY_THREE);
            CMD_ENTRY(LSET, handleLSET, true, ONLY_FOUR);
            CMD_ENTRY(RPOPLPUSH, handleRPOPLPUSH, true, ONLY_THREE);
            CMD_ENTRY(LPOPRPUSH, handleLPOPRPUSH, true, ONLY_THREE);
            CMD_ENTRY(LRANGE, handleLRANGE, false, ONLY_FOUR);

            // zset
            // CMD_ENTRY(ZADD, handleZADD, true, EVEN_VALIDATOR);
            CMD_ENTRY(ZRANGE, handleZRANGE, false, ONLY_FOUR_FIVE);
            CMD_ENTRY(ZREM, handleZREM, true, ONLY_MORE_THREE);
            CMD_ENTRY(ZSCORE, handleZSCORE, false, ONLY_THREE);
            CMD_ENTRY(ZRANK, handleZRANK, false, ONLY_THREE);
            CMD_ENTRY(ZINCRBY, handleZINCRBY, false, ONLY_FOUR);
            CMD_ENTRY(ZINCRBYFLOAT, handleZINCRBYFLOAT, false, ONLY_FOUR);
            CMD_ENTRY(ZCOUNT, handleZCOUNT, false, ONLY_FOUR);
            CMD_ENTRY(ZRANGEBYSCORE, handleZRANGEBYSCORE, false, ONLY_FOUR_FIVE);
            CMD_ENTRY(ZREMRANGEBYSCORE, handleZREMRANGEBYSCORE, true, ONLY_FOUR);

            // set
            // CMD_ENTRY(SADD, handleSADD, true, ONLY_MORE_THREE);
            CMD_ENTRY(SMEMBERS, handleSMEMBERS, false, ONLY_TWO);
            CMD_ENTRY(SREM, handleSREM, true, ONLY_MORE_THREE);
            CMD_ENTRY(SISMEMBER, handleSISMEMBER, false, ONLY_THREE);
            CMD_ENTRY(SCARD, handleSCARD, false, ONLY_TWO);
            CMD_ENTRY(SRANDMEMBER, handleSRANDMEMBER, false, ONLY_TWO_THREE);
            CMD_ENTRY(SPOP, handleSPOP, true, ONLY_TWO_THREE);
            CMD_ENTRY(SDIFF, handleSDIFF, false, ONLY_MORE_TWO);
            CMD_ENTRY(SINTER, handleSINTER, false, ONLY_MORE_TWO);
            CMD_ENTRY(SUNION, handleSUNION, false, ONLY_MORE_TWO);
            CMD_ENTRY(SMOVE, handleSMOVE, true, ONLY_FOUR);

            // server
            CMD_ENTRY(FLUSHDB, handleFLUSHDB, true, ONLY_ONE_TWO);
            CMD_ENTRY(FLUSHDBALL, handleFLUSHDBALL, true, ONLY_ONE_TWO);
            CMD_ENTRY(DBSIZE, handleDBSIZE, false, ONLY_ONE);
            CMD_ENTRY(EXPIRE, handleEXPIRE, false, ONLY_THREE);
            CMD_ENTRY(TTL, handleTTL, false, ONLY_TWO);
            CMD_ENTRY(PEXPIRE, handlePEXPIRE, false, ONLY_THREE);
            CMD_ENTRY(PTTL, handlePTTL, false, ONLY_TWO);
            CMD_ENTRY(PERSIST, handlePERSIST, false, ONLY_TWO);
            CMD_ENTRY(RENAME, handleRENAME, false, ONLY_THREE);
            CMD_ENTRY(RENAMENX, handleRENAMENX, false, ONLY_THREE);
            CMD_ENTRY(RANDOMKEY, handleRANDOMKEY, false, ONLY_ONE);
            CMD_ENTRY(INFO, handleINFO, false, ONLY_ONE);
            CMD_ENTRY(SAVE, handleSAVE, false, ONLY_ONE);
            CMD_ENTRY(BGSAVE, handleBGSAVE, false, ONLY_ONE);
            CMD_ENTRY(LASTSAVE, handleLASTSAVE, false, ONLY_ONE);
            CMD_ENTRY(LASTSAVE1, handleLASTSAVE1, false, ONLY_ONE);
            CMD_ENTRY(COMMAND, handleCOMMAND, false, ONLY_ONE);
            CMD_ENTRY(ECHO, handleECHO, false, ONLY_TWO);
            CMD_ENTRY(TIME, handleTIME, false, ONLY_ONE);
            CMD_ENTRY(LOCALTIME, handleLOCALTIME, false, ONLY_ONE);
            CMD_ENTRY(WATCH, handleWATCH, false, ONLY_MORE_TWO);
            CMD_ENTRY(UNWATCH, handleUNWATCH, false, ONLY_ONE);
            CMD_ENTRY(SLOWLOG, handleSLOWLOG, false, ONLY_TWO_THREE);
            CMD_ENTRY(MONITOR, handleMONITOR, false, ONLY_ONE);
            CMD_ENTRY(AOFROTATE, handleAOFROTATE, false, ONLY_ONE);
            CMD_ENTRY(SHUTDOWN, handleSHUTDOWN, false, ONLY_ONE);

            return builder.build();
        }
        RespValue execute1(std::vector<RespValue> args, MSocket::MSocketPtr sock, bool RecordAOF = true);

    private:
        static const auto &getCommandTable()
        {
            static const auto table = buildCommandTable();
            return table;
        }
#else
        RespValue execute(std::vector<RespValue> args, MSocket::MSocketPtr sock, bool RecordAOF = true);
#endif

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
         * @note Watch + Transaction 模式使用
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

    private:
        /* REDIS SERVER CONFIG */
        struct CommConfig
        {
            int32_t maxClients = 1000;  // 最大客户端数量
            int32_t timeout_s = 0;      // 客户端超时(s)
            std::string save;           // 保存策略
        };

        CommConfig m_config;

    private:
        /* REDIS SERVER */
        std::array<std::array<DataShard, SHARD_COUNT>, DB_COUNT> m_dbs; // 数据库
        std::atomic<uint32_t> m_commands{0};                            // 总共命令数量
        std::atomic<bool> m_shutdown{false};                            // 服务器关闭标识
        std::string m_password = "";                                    // 管理员密码
        std::atomic<time_t> m_last_time{0};                             // 上一次保存rdb文件时间
        std::atomic<bool> m_bgsave_running{false};                      // 后台保存rdb
        MSocket::MSocketWPtr m_admin_sock;                              // 用于同一时间只能一个管理员上线
    private:
        /* SUBSCRIPTION */
        SubscriptionModule m_subscription; // subscription

    private:
        /* SLOWLOG */
        SlowLogModule m_slowLog;                // monitor
        std::atomic<bool> m_push_monitor{true}; // 是否推送给monitor
    private:
        /* MONITOR */
        MonitorModule m_monitor; // monitor模式

    private:
        /* AOF */
        AOFModule m_aof; // AOF
    };

    template <typename T>
    CommandHandler<T>::CommandHandler(int level, int option_name, T option, IOManager *manager,
                                      IOManager *acceptmanager)
        : TcpServer<T>(level, option_name, option, manager, acceptmanager)
    {
#ifdef COMMAND_TABLE
        // 设置 AOF 执行器
        m_aof.setExecutor([this](std::vector<RespValue> args,
                                 MSocket::MSocketPtr sock,
                                 bool record) -> RespValue
                          { return this->execute1(args, sock, record); });
#else
        // 设置 AOF 执行器
        m_aof.setExecutor([this](std::vector<RespValue> args,
                                 MSocket::MSocketPtr sock,
                                 bool record) -> RespValue
                          { return this->execute(args, sock, record); });
#endif
        loadFromFile();
        m_aof.loadAOF();
        m_aof.initAOF(); // 初始化AOF,追加打开AOF文件,并开启AOF同步协程
        IOManager::GetThis()->schedule(expireTime());
        if (s_admin_password.empty())
        {
            s_admin_password = "admin123";
        }
        m_password = s_admin_password;
        m_aof.setLastAOFSync(SteadyClock::now());
    }

    template <typename T>
    CommandHandler<T>::~CommandHandler()
    {
        m_shutdown.store(true, std::memory_order_release);

        // 停止AOF的循环写入
        m_aof.stop();

        // 等待其他协程退出
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // 停止 AOF 刷新线程
        m_aof.stopAOFFlushThread();

        saveToFile();
        m_aof.closeAOFWithFlush();
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

#ifdef COMMAND_TABLE
    template <typename T>
    RespValue CommandHandler<T>::execute1(std::vector<RespValue> args, MSocket::MSocketPtr sock, bool RecordAOF)
    {
        auto start = SteadyClock::now();
        if (args.empty())
        {
            return RespValue::error("ERR empty command");
        }
        std::string cmd = args[0].str;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
#define HOT_COMMANDS(XX) \
    XX(GET)              \
    XX(SET)              \
    XX(DEL)              \
    XX(HSET)             \
    XX(HGET)             \
    XX(LPUSH)            \
    XX(LPOP)             \
    XX(SADD)             \
    XX(ZADD)

#define IF_CMD(name)                                          \
    if (cmd == #name)                                         \
    {                                                         \
        if (m_aof.isWriteCommand(cmd) && RecordAOF)           \
        {                                                     \
            std::string aof_cmds = m_aof.formatCommand(args); \
            m_aof.appendToAOF(aof_cmds);                      \
        }                                                     \
        return handle##name(args, sock, RecordAOF, this);     \
    }

        // 高频命令不走命令表
        HOT_COMMANDS(IF_CMD);

#undef IF_CMD
#undef HOT_COMMANDS

        const auto &table = getCommandTable();
        auto *entry = table.find_lowerbound(fnv1a_hash(cmd.c_str()));
        if (!entry)
        {
            return RespValue::error("ERR unknown command");
        }

        // 参数验证
        if (entry->argV && !entry->argV(args.size()))
        {
            return RespValue::error("ERR wrong number of arguments for '" + cmd + "'");
        }

        if (RecordAOF && entry->is_write)
        {
            std::string aof_cmds = m_aof.formatCommand(args);
            m_aof.appendToAOF(aof_cmds);
        }

        // 执行命令
        auto handler = entry->handler;
        RespValue result = handler(args, sock, RecordAOF, this);

        // 慢查询记录
        auto end = SteadyClock::now();
        std::string cmd_str;
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0)
            {
                cmd_str += " ";
            }
            cmd_str += args[i].str;
        }
        m_slowLog.pushEntry(cmd_str, sock, start, end);

        return result;
    }
#else
    template <typename T>
    RespValue CommandHandler<T>::execute(std::vector<RespValue> args, MSocket::MSocketPtr sock, bool RecordAOF)
    {
        auto start = SteadyClock::now();

        auto return_with_slowlog = [&](RespValue resp) -> RespValue
        {
            auto end = std::chrono::steady_clock::now();
            std::string cmd_str;
            for (size_t i = 0; i < args.size(); i++)
            {
                if (i > 0)
                {
                    cmd_str += " ";
                }
                cmd_str += args[i].str;
            }
            m_slowLog.pushEntry(cmd_str, sock, start, end);

            // 写命令记录进入AOF(异步)
            if (args.empty())
            {
                return resp;
            }
            std::string cmd = args[0].str;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            if (RecordAOF && m_aof.isWriteCommand(cmd))
            {
                std::string aof_cmds = m_aof.formatCommand(args);
                m_aof.appendToAOF(aof_cmds);
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
            m_push_monitor.store(false, std::memory_order_release);
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
            BLUE_LOG_INFO(xx::g_logger) << "if-else模式";
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
                if (sock->getClientlevel() != 0)
                {
                    return return_with_slowlog(RespValue::error("ERR this connection already have been logged by others"));
                }
                sock->setClientlevel(1);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            if (args[1].str == m_password)
            {
                if (!m_admin_sock.expired())
                {
                    return return_with_slowlog(RespValue::error("ERR admin already logged in elsewhere"));
                }
                if (sock->getClientlevel() == 1)
                {
                    return return_with_slowlog(RespValue::error("ERR this connection already have been logged by client"));
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
                    result.push_back(RespValue::bulk_string(std::to_string(m_slowLog.getSlowLogThan())));
                }
                if (pattern == "*" || pattern == "slowlog-max-len" || pattern == "slowlog-*")
                {
                    result.push_back(RespValue::bulk_string("slowlog-max-len"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_slowLog.getSlowMaxLen())));
                }
                if (pattern == "*" || pattern == "aof-enabled" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-enabled"));
                    result.push_back(RespValue::bulk_string(m_aof.getConfig_AOFEnabled() ? "yes" : "no"));
                }
                if (pattern == "*" || pattern == "aof-filename" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-filename"));
                    result.push_back(RespValue::bulk_string(m_aof.getConfig_AOFFilename()));
                }
                if (pattern == "*" || pattern == "aof-sync" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-sync"));
                    result.push_back(RespValue::bulk_string(m_aof.getConfig_AOFSync()));
                }
                if (pattern == "*" || pattern == "aof-max_file_size" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_file_size"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_aof.getConfig_AOFMaxFileSize())));
                }
                if (pattern == "*" || pattern == "aof-max_file_number" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_file_number"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_aof.getConfig_AOFMaxFileNumber())));
                }
                if (pattern == "*" || pattern == "aof-max_buffer_size" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_buffer_size"));
                    result.push_back(RespValue::bulk_string(std::to_string(m_aof.getMaxAOFBufferSize())));
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

                if (param == "clientpass") // 客户端密码
                {
                    sock->setClientPassword(value);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "slowlog-log-slower-than") // 慢查询的限制时长(超过这个时长记录慢查询)
                {
                    try
                    {
                        int64_t val = std::stoll(value);
                        if (val < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value must be >= 0"));
                        }
                        m_slowLog.setSlowLogThan(val);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "slowlog-max-len") // 慢查询记录最大条数
                {
                    try
                    {
                        size_t val = std::stoul(value);
                        if (val <= 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value must be > 0"));
                        }
                        m_slowLog.setSlowMaxLen(val);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "aof-enabled") // 开启aof记录
                {
                    if (value == "yes" || value == "1")
                    {
                        m_aof.setConfig_AOFEnabled(true);
                        m_aof.initAOF();
                    }
                    else if (value == "no" || value == "0")
                    {
                        m_aof.setConfig_AOFEnabled(false);
                        m_aof.closeAOF();
                    }
                    else
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid value"));
                    }
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-sync") // aof策略
                {
                    if (value == "always" || value == "everysec" || value == "no")
                    {
                        m_aof.setConfig_AOFSync(value);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    return return_with_slowlog(RespValue::error("ERR invalid sync mode"));
                }
                if (param == "aof-filename") // aof文件名(模板文件名)
                {
                    if (value.empty())
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid filename"));
                    }
                    m_aof.setConfig_AOFFilename(value);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_file_size") // 每个aof文件大小
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
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }

                    m_aof.setConfig_AOFMaxFileSize(val);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_file_number") // 最多保留多少aof文件
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
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                    m_aof.setConfig_AOFMaxFileNumber(val);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_buffer_size") // aof缓冲区大小
                {
                    int64_t val;
                    try
                    {
                        val = std::stoll(value);
                        if (val < 1024 * 1024)
                        {
                            return return_with_slowlog(RespValue::error("ERR max_file_number value too small"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                    m_aof.setMaxAOFBufferSize(val);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                // 以下只允许管理员设置
                if (!isAdmin(sock))
                {
                    return return_with_slowlog(RespValue::error("ERR authentication required"));
                }
                if (param == "maxclients") // 服务器最大支持的客户端数量
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
                if (param == "timeout") // 每个客户端会话的超时时长
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
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
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
            if (count == 0)
            {
                return RespValue::array({});
            }
            if (members.empty())
            {
                return RespValue::null_bulk();
            }
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                set.erase(members[idx]);
                if (set.empty())
                {
                    shards.sets.erase(it);
                }
                return return_with_slowlog(RespValue::bulk_string(members[idx]));
            }

            if (count > 0)
            {
                // 正数：不重复
                int num = std::min(count, (int32_t)(members.size()));
                std::shuffle(members.begin(), members.end(), std::mt19937(std::random_device()()));
                for (int i = 0; i < num; i++)
                {
                    set.erase(members[i]);
                    results.push_back(RespValue::bulk_string(members[i]));
                }
                if (set.empty())
                {
                    shards.sets.erase(it);
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
            info += "aof_enabled:" + std::string(m_aof.getConfig_AOFEnabled() ? "1" : "0") + "\r\n";
            info += "aof_sync:" + m_aof.getConfig_AOFSync() + "\r\n";
            info += "aof_current_file:" + m_aof.getCurrentFileName() + "\r\n";
            info += "aof_file_index:" + std::to_string(m_aof.getCurrentFileIdx()) + "\r\n";
            info += "aof_current_size:" + std::to_string(m_aof.getCurrentFileSize()) + "\r\n";
            info += "aof_max_file_size:" + std::to_string(m_aof.getConfig_AOFMaxFileSize()) + "\r\n";
            info += "aof_max_files:" + std::to_string(m_aof.getConfig_AOFMaxFileNumber()) + "\r\n";
            info += "aof_max_buffer_size" + std::to_string(m_aof.getMaxAOFBufferSize()) + "\r\n";
            info += "\r\n";

            // Monitor
            info += "# Monitor\r\n";
            info += "monitor_clients:" + std::to_string(m_monitor.size()) + "\r\n";
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
                m_slowLog.syncSlowLogs();

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

                std::vector<RespValue> results = m_slowLog.getSlowLogs(count);
                return return_with_slowlog(RespValue::array(std::move(results)));
            }

            else if (sub_cmd == "LEN")
            {
                m_slowLog.syncSlowLogs();
                return return_with_slowlog(RespValue::integer(m_slowLog.len()));
            }
            else if (sub_cmd == "RESET")
            {
                m_slowLog.reset();
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

            // 添加monitor client
            m_monitor.addMonitorClient(sock);

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
            if (!m_aof.getConfig_AOFEnabled())
            {
                return return_with_slowlog(RespValue::error("ERR AOF is disabled"));
            }
            if (m_aof.getAOFRotating())
            {
                return return_with_slowlog(RespValue::error("ERR AOF rotation already in progress"));
            }
            std::thread([this]
                        {
                // rotateAOF();
                m_aof.rotateAOF();
                BLUE_LOG_INFO(xx::g_logger) << "AOF rotation finished"; })
                .detach();
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
#endif
    template <typename T>
    Task<void> CommandHandler<T>::handleClient(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(xx::g_logger) << "handleClient begin, fd=" << sock->getSocketfd();
        // BLUE_LOG_INFO(xx::g_logger) << "remote address: " <<  sock->getRemoteAddress()->toString();
        // BLUE_LOG_INFO(xx::g_logger) << "local address : " <<  sock->getLocalAddress()->toString();

        RespStreamParser parser;                     // 解析器
        const size_t MAX_COMMAND_SIZE = 1024 * 1024; // 解析缓冲区最大大小
        const size_t BATCH_SIZE = 8192;              // 批量响应大小阈值

        const uint64_t timeout_ms = static_cast<uint64_t>(m_config.timeout_s) * 1000ul;

        do
        {
            if (m_shutdown.load(std::memory_order_acquire) || TcpServer<T>::getIsStop())
            {
                break;
            }
            char tmp[8192];
            ssize_t ret;
            if (timeout_ms > 0)
            {
                BLUE_LOG_INFO(xx::g_logger) << "超时recv";
                ret = co_await sock->recvT(tmp, sizeof(tmp),0,timeout_ms);
            }
            else
            {
                BLUE_LOG_INFO(xx::g_logger) << "recv";
                ret = co_await sock->recv(tmp, sizeof(tmp));
            }
            if (ret <= 0)
            {
                if (ret == 0)
                {
                    BLUE_LOG_INFO(xx::g_logger) << "[client " << sock->getSocketfd() << "] 正常关闭";
                    break;
                }
                if (errno == ETIMEDOUT)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "[client " << sock->getSocketfd() 
                                            << "] timeout (" << m_config.timeout_s << "s), closing";
                    break;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                BLUE_LOG_ERROR(xx::g_logger) << "[client " << sock->getSocketfd()
                                            << "] recv error: " << strerror(errno);
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
#ifdef COMMAND_TABLE
                                    auto response = execute1(transaction, sock, true);
#else
                                    auto response = execute(transaction, sock, true);
#endif
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
                            m_subscription.removeSubscriber(channel, sock);

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
                                m_subscription.addSubscriber(channel, sock);

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

                            // 获取订阅者并发送消息给所有订阅者
                            int receiver_count = m_subscription.publishMessage(channel, message);

                            response = RespValue::integer(receiver_count);
                        }
                        end = SteadyClock::now();
                    }
                    else
                    {
#ifdef COMMAND_TABLE
                        // 执行普通命令
                        response = execute1(std::move(copy_arr), sock, true); // move后copy_arr为空
#else
                        response = execute(std::move(copy_arr), sock, true); // move后copy_arr为空
#endif
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
                    m_slowLog.pushEntry(cmd_str, sock, start, end);
                }

                if (response.str != "QUEUED" && m_push_monitor.load(std::memory_order_acquire))
                {
                    // 推送消息给监控客户端
                    m_monitor.pushToMonitor(cmd_str, sock);
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
        m_subscription.removeAllSubscribers(sock);

        sock->clearSubScription();
        // BLUE_LOG_INFO(xx::g_logger) << "one Client exit, fd:" << sock->getSocketfd();
        sock->close();

        // 删除过期的monitor
        m_monitor.removeMonitor();

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

#ifdef COMMAND_TABLE
    // ========== 连接命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handlePING(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        BLUE_LOG_INFO(xx::g_logger) << "commandTable 模式";
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() == 1)
            return RespValue::simple_string("PONG");
        else
            return RespValue::bulk_string(args[1].str);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleAUTH(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (args[1].str == sock->getClientPassword())
        {
            if (sock->getClientlevel() != 0)
            {
                return RespValue::error("ERR this connection already have been logged by others");
            }
            sock->setClientlevel(1);
            return RespValue::simple_string("OK");
        }
        if (args[1].str == self->m_password)
        {
            if (!self->m_admin_sock.expired())
            {
                return RespValue::error("ERR admin already logged in elsewhere");
            }
            if (sock->getClientlevel() == 1)
            {
                return RespValue::error("ERR this connection already have been logged by client");
            }
            self->m_admin_sock = sock;
            sock->setClientlevel(2);
            return RespValue::simple_string("OK");
        }
        return RespValue::error("ERR invalid password");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSELECT(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int db;
        try
        {
            db = std::stoi(args[1].str);
            if (db < 0 || db >= DB_COUNT)
            {
                return RespValue::error("ERR DB index is out of range");
            }
        }
        catch (...)
        {
            return RespValue::error("ERR invalid DB index");
        }
        sock->setClientId(db);
        return RespValue::simple_string("OK");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleCLIENT(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::string subcmd = args[1].str;
        std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
        if (subcmd == "SETNAME")
        {
            if (args.size() != 3)
            {
                return RespValue::error("ERR wrong number of arguments for 'CLIENT SETNAME'");
            }
            sock->setClientName(args[2].str);
            return RespValue::simple_string("OK");
        }
        else if (subcmd == "GETNAME")
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'CLIENT GETNAME'");
            }
            if (sock->getClientName().empty())
            {
                return RespValue::null_bulk();
            }
            return RespValue::bulk_string(sock->getClientName());
        }
        else if (subcmd == "LIST")
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'CLIENT LIST'");
            }
            std::string result;
            result += "name=" + sock->getClientName() + " ";
            result += "addr=" + sock->getRemoteAddress()->toString() + " ";
            result += "subScription=" + std::to_string((int)(sock->inSubScription())) + " ";
            result += "transaction=" + std::to_string((int)(sock->inTransaction())) + " ";
            result += "monitor=" + std::to_string((int)(sock->inMonitorMode())) + "\r\n";
            return RespValue::bulk_string(result);
        }
        return RespValue::error("ERR wrong arguments for 'CLIENT'");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleCONFIG(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        // 同一时刻只能存在一个管理员，并且由于CommandHandler只有一个实例化，所以修改和获取不需要锁
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

            if (pattern == "*" || pattern == "clientpass")
            {
                result.push_back(RespValue::bulk_string("clientpass"));
                result.push_back(RespValue::bulk_string(sock->getClientPassword()));
            }
            if (pattern == "*" || pattern == "maxclients")
            {
                result.push_back(RespValue::bulk_string("maxclients"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_config.maxClients)));
            }
            if (pattern == "*" || pattern == "timeout")
            {
                result.push_back(RespValue::bulk_string("timeout"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_config.timeout_s)));
            }
            if (pattern == "*" || pattern == "slowlog-log-slower-than" || pattern == "slowlog-*")
            {
                result.push_back(RespValue::bulk_string("slowlog-log-slower-than"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_slowLog.getSlowLogThan())));
            }
            if (pattern == "*" || pattern == "slowlog-max-len" || pattern == "slowlog-*")
            {
                result.push_back(RespValue::bulk_string("slowlog-max-len"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_slowLog.getSlowMaxLen())));
            }
            if (pattern == "*" || pattern == "aof-enabled" || pattern == "aof-*")
            {
                result.push_back(RespValue::bulk_string("aof-enabled"));
                result.push_back(RespValue::bulk_string(self->m_aof.getConfig_AOFEnabled() ? "yes" : "no"));
            }
            if (pattern == "*" || pattern == "aof-filename" || pattern == "aof-*")
            {
                result.push_back(RespValue::bulk_string("aof-filename"));
                result.push_back(RespValue::bulk_string(self->m_aof.getConfig_AOFFilename()));
            }
            if (pattern == "*" || pattern == "aof-sync" || pattern == "aof-*")
            {
                result.push_back(RespValue::bulk_string("aof-sync"));
                result.push_back(RespValue::bulk_string(self->m_aof.getConfig_AOFSync()));
            }
            if (pattern == "*" || pattern == "aof-max_file_size" || pattern == "aof-*")
            {
                result.push_back(RespValue::bulk_string("aof-max_file_size"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_aof.getConfig_AOFMaxFileSize())));
            }
            if (pattern == "*" || pattern == "aof-max_file_number" || pattern == "aof-*")
            {
                result.push_back(RespValue::bulk_string("aof-max_file_number"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_aof.getConfig_AOFMaxFileNumber())));
            }
            if (pattern == "*" || pattern == "aof-max_buffer_size" || pattern == "aof-*")
            {
                result.push_back(RespValue::bulk_string("aof-max_buffer_size"));
                result.push_back(RespValue::bulk_string(std::to_string(self->m_aof.getMaxAOFBufferSize())));
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

            if (param == "clientpass") // 客户端密码
            {
                sock->setClientPassword(value);
                return RespValue::simple_string("OK");
            }
            if (param == "slowlog-log-slower-than") // 慢查询的限制时长(超过这个时长记录慢查询)
            {
                try
                {
                    int64_t val = std::stoll(value);
                    if (val < 0)
                    {
                        return RespValue::error("ERR value must be >= 0");
                    }
                    self->m_slowLog.setSlowLogThan(val);
                    return RespValue::simple_string("OK");
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid integer value");
                }
            }
            if (param == "slowlog-max-len") // 慢查询记录最大条数
            {
                try
                {
                    size_t val = std::stoul(value);
                    if (val <= 0)
                    {
                        return RespValue::error("ERR value must be > 0");
                    }
                    self->m_slowLog.setSlowMaxLen(val);
                    return RespValue::simple_string("OK");
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid integer value");
                }
            }
            if (param == "aof-enabled") // 开启aof记录
            {
                if (value == "yes" || value == "1")
                {
                    self->m_aof.setConfig_AOFEnabled(true);
                    self->m_aof.initAOF();
                }
                else if (value == "no" || value == "0")
                {
                    self->m_aof.setConfig_AOFEnabled(false);
                    self->m_aof.closeAOF();
                }
                else
                {
                    return RespValue::error("ERR invalid value");
                }
                return RespValue::simple_string("OK");
            }
            if (param == "aof-sync") // aof策略
            {
                if (value == "always" || value == "everysec" || value == "no")
                {
                    self->m_aof.setConfig_AOFSync(value);
                    return RespValue::simple_string("OK");
                }
                return RespValue::error("ERR invalid sync mode");
            }
            if (param == "aof-filename") // aof文件名(模板文件名)
            {
                if (value.empty())
                {
                    return RespValue::error("ERR invalid filename");
                }
                self->m_aof.setConfig_AOFFilename(value);
                return RespValue::simple_string("OK");
            }
            if (param == "aof-max_file_size") // 每个aof文件大小
            {
                int64_t val;
                try
                {
                    val = std::stoi(value);
                    if (val < 1024 * 1024)
                    {
                        return RespValue::error("ERR max_file_size value too small");
                    }
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid integer value");
                }

                self->m_aof.setConfig_AOFMaxFileSize(val);
                return RespValue::simple_string("OK");
            }
            if (param == "aof-max_file_number") // 最多保留多少aof文件
            {
                int val;
                try
                {
                    val = std::stoi(value);
                    if (val < 5)
                    {
                        return RespValue::error("ERR max_file_number value too small");
                    }
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid integer value");
                }
                self->m_aof.setConfig_AOFMaxFileNumber(val);
                return RespValue::simple_string("OK");
            }
            if (param == "aof-max_buffer_size") // aof缓冲区大小
            {
                int64_t val;
                try
                {
                    val = std::stoll(value);
                    if (val < 1024 * 1024)
                    {
                        return RespValue::error("ERR max_file_number value too small");
                    }
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid integer value");
                }
                self->m_aof.setMaxAOFBufferSize(val);
                return RespValue::simple_string("OK");
            }
            // 以下只允许管理员设置
            if (!self->isAdmin(sock))
            {
                return RespValue::error("ERR authentication required");
            }
            if (param == "maxclients") // 服务器最大支持的客户端数量
            {
                try
                {
                    int newmax = std::stoi(value);
                    if (newmax <= 0)
                    {
                        return RespValue::error("ERR invalid maxclients value");
                    }
                    if (newmax < self->getConnection())
                    {
                        return RespValue::error("ERR maxclients can't be less than current connections");
                    }
                    self->m_config.maxClients = newmax;
                    return RespValue::simple_string("OK");
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid integer value");
                }
            }
            if (param == "timeout") // 每个客户端会话的超时时长
            {
                try
                {
                    int timeout = std::stoi(value);
                    if (timeout < 0)
                    {
                        return RespValue::error("ERR invalid timeout value");
                    }
                    self->m_config.timeout_s = timeout;
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
    }

    // ========== String 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleSET(std::vector<RespValue> &args,
                                           MSocket::MSocketPtr sock,
                                           bool aof,
                                           CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'SET'");
        }
        const std::string key = args[1].str;
        const std::string val = args[2].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleGET(std::vector<RespValue> &args,
                                           MSocket::MSocketPtr sock,
                                           bool aof,
                                           CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::null_bulk();
        }
        if (args.size() < 2)
        {
            return RespValue::error("ERR wrong number of arguments for 'GET'");
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

    template <typename T>
    RespValue CommandHandler<T>::handleMSET(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        for (size_t i = 1; i < args.size(); i += 2)
        {
            const std::string key = args[i].str;
            const std::string val = args[i + 1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            shards.store[key] = val;
        }
        return RespValue::simple_string("OK");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleMGET(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::vector<RespValue> results;

        for (size_t i = 1; i < args.size(); i++)
        {
            auto &shards = self->getShard(args[i].str, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleGETSET(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string val = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            shards.store[key] = val;
            return RespValue::bulk_string(val);
        }
        std::string ans = it->second;
        it->second = val;
        return RespValue::bulk_string(ans);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleAPPEND(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string val = args[2].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleSETNX(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string val = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            shards.store[key] = val;
            return RespValue::integer(1);
        }
        return RespValue::integer(0);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleEXISTS(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int64_t count = 0;
        for (size_t i = 1; i < args.size(); i++)
        {
            const std::string key = args[i].str;
            auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleDEL(std::vector<RespValue> &args,
                                           MSocket::MSocketPtr sock,
                                           bool aof,
                                           CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 2)
        {
            return RespValue::error("ERR wrong number of arguments for 'DEL'");
        }
        int count = 0;
        for (size_t i = 1; i < args.size(); i++)
        {
            const std::string key = args[i].str;
            auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleINCR(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        int64_t val = 0;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleINCRBY(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        int64_t increment;
        try
        {
            increment = std::stoll(args[2].str);
        }
        catch (...)
        {
            return RespValue::error("ERR value is not an integer or out of range");
        }
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleSTRLEN(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleTYPE(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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
    }

    template <typename T>
    RespValue CommandHandler<T>::handleKEYS(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
        for (auto &shard : self->m_dbs[sock->getClientId()])
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

        return RespValue::array(std::move(result));
    }

    // ========== Hash 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleHSET(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 4 || args.size() % 2 != 0)
        {
            return RespValue::error("ERR wrong number of arguments for 'HSET'");
        }
        int count = 0;
        std::string key = args[1].str;
        for (size_t i = 2; i < args.size(); i += 2)
        {
            auto &shards = self->getShard(key, sock);
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
    }

    template <typename T>
    RespValue CommandHandler<T>::handleHGET(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'HGET'");
        }
        const std::string key = args[1].str;
        const std::string field = args[2].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleHGETALL(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::vector<RespValue> result;
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleHDEL(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int count = 0;
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleHLEN(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleHEXISTS(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string field = args[2].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleHKEYS(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        std::vector<RespValue> results;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleHVALS(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        std::vector<RespValue> results;
        auto &shards = self->getShard(key, sock);
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

    // ========== List 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleLPUSH(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'LPUSH'");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto &lhs = shards.lists[key];
        for (size_t i = 2; i < args.size(); i++)
        {
            lhs.push_front(args[i].str);
        }
        return RespValue::integer(lhs.size());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleRPUSH(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto &lhs = shards.lists[key];
        for (size_t i = 2; i < args.size(); i++)
        {
            lhs.push_back(args[i].str);
        }
        return RespValue::integer(lhs.size());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLPOP(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
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
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleRPOP(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleLLEN(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.lists.find(key);
        if (it == shard.lists.end())
        {
            return RespValue::integer(0);
        }
        return RespValue::integer(it->second.size());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLINSERT(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string pos = args[2].str;
        const std::string pivot = args[3].str;
        const std::string val = args[4].str;

        auto &shard = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.lists.find(key);
        if (it == shard.lists.end())
        {
            return RespValue::null_bulk();
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
            return RespValue::integer(-1); // pivot不存在
        }
        if (pos == "AFTER")
        {
            list_it++;
        }
        lists.insert(list_it, val);
        return RespValue::integer(lists.size());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLINDEX(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        int64_t idx = 0;
        try
        {
            idx = std::stoll(args[2].str);
        }
        catch (...)
        {
            return RespValue::error("ERR value is not a integer or out of range");
        }
        auto &shard = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.lists.find(key);
        if (it == shard.lists.end())
        {
            return RespValue::null_bulk();
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
        return RespValue::bulk_string(*list_it);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLSET(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
            return RespValue::error("ERR value is not a integer or out of range");
        }
        auto &shard = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.lists.find(key);
        if (it == shard.lists.end())
        {
            return RespValue::null_bulk();
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
        return RespValue::simple_string("OK");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleRPOPLPUSH(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string source_key = args[1].str;
        const std::string dest_key = args[2].str;
        if (source_key == dest_key)
        {
            return RespValue::integer(1);
        }
        int src_idx = self->getShardIndex(source_key);
        int dest_idx = self->getShardIndex(dest_key);
        if (src_idx > dest_idx)
        {
            std::swap(src_idx, dest_idx);
        }
        auto &src_shard = self->getShard(source_key, sock);
        auto &dest_shard = self->getShard(dest_key, sock);
        std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
        std::unique_lock<std::shared_mutex> lock2;
        if (src_idx != dest_idx)
        {
            lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
        }
        auto src_it = src_shard.lists.find(source_key);
        if (src_it == src_shard.lists.end())
        {
            return RespValue::integer(0);
        }

        auto dest_it = dest_shard.lists.find(dest_key);
        if (dest_it == dest_shard.lists.end())
        {
            dest_shard.lists[dest_key] = std::list<std::string>();
            dest_it = dest_shard.lists.find(dest_key);
        }
        const std::string tem = src_it->second.back();
        src_it->second.pop_back();
        dest_it->second.push_front(tem);
        return RespValue::integer(1);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLPOPRPUSH(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string source_key = args[1].str;
        const std::string dest_key = args[2].str;
        if (source_key == dest_key)
        {
            return RespValue::integer(1);
        }
        int src_idx = self->getShardIndex(source_key);
        int dest_idx = self->getShardIndex(dest_key);
        if (src_idx > dest_idx)
        {
            std::swap(src_idx, dest_idx);
        }
        auto &src_shard = self->getShard(source_key, sock);
        auto &dest_shard = self->getShard(dest_key, sock);
        std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
        std::unique_lock<std::shared_mutex> lock2;
        if (src_idx != dest_idx)
        {
            lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
        }
        auto src_it = src_shard.lists.find(source_key);
        if (src_it == src_shard.lists.end())
        {
            return RespValue::integer(0);
        }

        auto dest_it = dest_shard.lists.find(dest_key);
        if (dest_it == dest_shard.lists.end())
        {
            dest_shard.lists[dest_key] = std::list<std::string>();
            dest_it = dest_shard.lists.find(dest_key);
        }
        const std::string tem = src_it->second.front();
        src_it->second.pop_front();
        dest_it->second.push_back(tem);
        return RespValue::integer(1);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLRANGE(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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
    }

    // ========== Set 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleSADD(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of argument for 'SADD'");
        }
        const std::string key = args[1].str;
        int32_t count = 0;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleSMEMBERS(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        std::vector<RespValue> results;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleSREM(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        int32_t count = 0;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleSISMEMBER(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string member = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.sets.find(key);
        if (it == shards.sets.end())
        {
            return RespValue::integer(0);
        }
        return RespValue::integer(it->second.count(member));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSCARD(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.sets.find(key);
        if (it == shards.sets.end())
        {
            return RespValue::integer(0);
        }
        return RespValue::integer(it->second.size());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSRANDMEMBER(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleSPOP(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int32_t count;
        try
        {
            count = args.size() == 3 ? std::stoi(args[2].str) : 1;
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
        auto &shards = self->getShard(key, sock);
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
        if (count == 0)
        {
            return RespValue::array({});
        }
        if (members.empty())
        {
            return RespValue::null_bulk();
        }
        if (args.size() == 2)
        {
            int idx = rand() % members.size();
            set.erase(members[idx]);
            if (set.empty())
            {
                shards.sets.erase(it);
            }
            return RespValue::bulk_string(members[idx]);
        }

        if (count > 0)
        {
            // 正数：不重复
            int num = std::min(count, (int32_t)(members.size()));
            std::shuffle(members.begin(), members.end(), std::mt19937(std::random_device()()));
            for (int i = 0; i < num; i++)
            {
                set.erase(members[i]);
                results.push_back(RespValue::bulk_string(members[i]));
            }
            if (set.empty())
            {
                shards.sets.erase(it);
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSDIFF(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::vector<RespValue> results;
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto &member : shard.sets[key])
        {
            bool ok = true;
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string tem_key = args[i].str;
                auto &tem_shard = self->getShard(tem_key, sock);
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
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSINTER(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::vector<RespValue> results;
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto &member : shard.sets[key])
        {
            bool ok = true;
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string tem_key = args[i].str;
                auto &tem_shard = self->getShard(tem_key, sock);
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
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSUNION(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::unordered_set<std::string> results_set;
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto &member : shard.sets[key])
        {
            results_set.insert(member);
        }
        for (size_t i = 2; i < args.size(); i++)
        {
            const std::string tem_key = args[i].str;
            auto &tem_shard = self->getShard(tem_key, sock);
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
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSMOVE(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string source_key = args[1].str;
        const std::string destination_key = args[2].str;
        if (source_key == destination_key)
        {
            return RespValue::integer(1);
        }
        const std::string member = args[3].str;
        int src_shard_idx = self->getShardIndex(source_key);
        int dest_shard_idx = self->getShardIndex(destination_key);
        if (src_shard_idx > dest_shard_idx)
        {
            std::swap(src_shard_idx, dest_shard_idx);
        }
        auto &src_shard = self->getShard(source_key, sock);
        auto &dest_shard = self->getShard(destination_key, sock);

        std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
        std::unique_lock<std::shared_mutex> lock2;
        if (src_shard_idx != dest_shard_idx)
        {
            lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
        }

        auto src_it = src_shard.sets.find(source_key);
        if (src_it == src_shard.sets.end())
        {
            return RespValue::integer(0);
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
            return RespValue::integer(0);
        }
        src_it->second.erase(member);
        dest_it->second.insert(member);
        if (src_it->second.empty())
        {
            src_shard.sets.erase(src_it);
        }
        return RespValue::integer(1);
    }

    // ========== ZSet 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleZADD(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 4 || args.size() % 2 != 0)
        {
            return RespValue::error("ERR wrong number of arguments for 'ZADD'");
        }
        int count = 0;
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleZRANGE(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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
                results.push_back(RespValue::bulk_string(self->format_score(node->key.score)));
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleZREM(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int count = 0;
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleZSCORE(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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
            return RespValue::bulk_string(self->format_score(sit->second));
        }
        return RespValue::null_bulk();
    }

    template <typename T>
    RespValue CommandHandler<T>::handleZRANK(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str, member = args[2].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleZINCRBY(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
            return RespValue::error("ERR value is not a integer or out of range");
        }
        auto &shard = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.zset.find(key);
        if (it == shard.zset.end())
        {
            return RespValue::null_bulk();
        }
        auto sit = shard.zset_score.find(key);
        if (sit == shard.zset_score.end())
        {
            return RespValue::null_bulk();
        }
        auto &skiplist = it->second;
        auto &scores_map = sit->second;
        if (scores_map.find(member) != scores_map.end())
        {
            ZSetKey old_val(scores_map[member], member);
            scores_map[member] += incr;
            skiplist.remove(old_val);
            skiplist.insert({scores_map[member], member}, member);
            return RespValue::bulk_string(self->format_score(scores_map[member]));
        }
        return RespValue::null_bulk();
    }

    template <typename T>
    RespValue CommandHandler<T>::handleZINCRBYFLOAT(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
            return RespValue::error("ERR value is not a float or out of range");
        }
        auto &shard = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.zset.find(key);
        if (it == shard.zset.end())
        {
            return RespValue::null_bulk();
        }
        auto sit = shard.zset_score.find(key);
        if (sit == shard.zset_score.end())
        {
            return RespValue::null_bulk();
        }
        auto &skiplist = it->second;
        auto &scores_map = sit->second;
        if (scores_map.find(member) != scores_map.end())
        {
            ZSetKey old_val(scores_map[member], member);
            scores_map[member] += incr;
            skiplist.remove(old_val);
            skiplist.insert({scores_map[member], member}, member);
            return RespValue::bulk_string(self->format_score(scores_map[member]));
        }
        return RespValue::null_bulk();
    }

    template <typename T>
    RespValue CommandHandler<T>::handleZCOUNT(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        double min = 0, max = 0;
        try
        {
            min = std::stod(args[2].str);
            max = std::stod(args[3].str);
            if (min > max)
            {
                return RespValue::error("ERR min can't greater than max");
            }
        }
        catch (...)
        {
            return RespValue::error("ERR value is not a double or out of range");
        }
        int64_t count = 0;
        auto &shard = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.zset_score.find(key);
        if (it == shard.zset_score.end())
        {
            return RespValue::integer(0);
        }
        for (const auto &[_, score] : it->second)
        {
            if (score >= min && score <= max)
            {
                count++;
            }
        }
        return RespValue::integer(count);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleZRANGEBYSCORE(std::vector<RespValue> &args,
                                                     MSocket::MSocketPtr sock,
                                                     bool aof,
                                                     CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        double min = 0, max = 0;
        try
        {
            min = std::stod(args[2].str);
            max = std::stod(args[3].str);
            if (min > max)
            {
                return RespValue::error("ERR min can't greater than max");
            }
        }
        catch (...)
        {
            return RespValue::error("ERR value is not a double or out of range");
        }
        std::vector<RespValue> results;
        bool withscore = (args.size() == 5 && (args[4].str == "WITHSCORE" || args[4].str == "withscore"));
        auto &shard = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.zset_score.find(key);
        if (it == shard.zset_score.end())
        {
            return RespValue::array({});
        }
        for (const auto &[member, score] : it->second)
        {
            if (score >= min && score <= max)
            {
                results.push_back(RespValue::bulk_string(member));
                if (withscore)
                {
                    results.push_back(RespValue::bulk_string((self->format_score(score))));
                }
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleZREMRANGEBYSCORE(std::vector<RespValue> &args,
                                                        MSocket::MSocketPtr sock,
                                                        bool aof,
                                                        CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        double min = 0, max = 0;
        try
        {
            min = std::stod(args[2].str);
            max = std::stod(args[3].str);
            if (min > max)
            {
                return RespValue::error("ERR min can't greater than max");
            }
        }
        catch (...)
        {
            return RespValue::error("ERR value is not a double or out of range");
        }
        int64_t count = 0;
        auto &shard = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.zset.find(key);
        if (it == shard.zset.end())
        {
            return RespValue::integer(-1);
        }
        auto sit = shard.zset_score.find(key);
        if (sit == shard.zset_score.end())
        {
            return RespValue::integer(-1);
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
        return RespValue::integer(count);
    }

    // ========== DB 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleFLUSHDB(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (self->isAdmin(sock))
        {
            if (args.size() < 1 || args.size() > 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'FLUSHDB'");
            }
            for (auto &shards : self->m_dbs[sock->getClientId()])
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
        else if (sock->getClientlevel() == 1)
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR maybe need 'FLUSHAD CONFIRM");
            }
            std::string confirm = args[1].str;
            std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::toupper);
            if (confirm == "CONFIRM")
            {
                for (auto &shards : self->m_dbs[sock->getClientId()])
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
            return RespValue::error("ERR maybe need 'FLUSHDB CONFIRM'");
        }
        return RespValue::error("ERR authentication required");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleFLUSHDBALL(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (self->isAdmin(sock))
        {
            if (args.size() < 1 || args.size() > 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'FLUSHDBALL'");
            }
            for (int db = 0; db < DB_COUNT; db++)
            {
                for (auto &shards : self->m_dbs[db])
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
            return RespValue::simple_string("OK");
        }
        else if (sock->getClientlevel() == 1)
        {
            if (args.size() != 2)
            {
                return RespValue::error("ERR maybe need 'FLUSHADALL CONFIRM");
            }
            std::string confirm = args[1].str;
            std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::toupper);
            if (confirm == "CONFIRM")
            {
                for (int db = 0; db < DB_COUNT; db++)
                {
                    for (auto &shards : self->m_dbs[db])
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
                return RespValue::simple_string("OK");
            }
            return RespValue::error("ERR authentication required, maybe need 'FLUSHADALL CONFIRM");
        }
        return RespValue::error("ERR authentication required");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleDBSIZE(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int64_t count = 0;
        for (auto &shards : self->m_dbs[sock->getClientId()])
        {
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            count += shards.store.size() + shards.lists.size() + shards.hash.size() + shards.zset.size();
        }
        return RespValue::integer(count);
    }

    // ========== Key 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleEXPIRE(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::integer(0);
        }
        shards.expire[key] = SteadyClock::now() + std::chrono::seconds(second);
        return RespValue::integer(1);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleTTL(std::vector<RespValue> &args,
                                           MSocket::MSocketPtr sock,
                                           bool aof,
                                           CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handlePEXPIRE(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::integer(0);
        }
        shards.expire[key] = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
        return RespValue::integer(1);
    }

    template <typename T>
    RespValue CommandHandler<T>::handlePTTL(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handlePERSIST(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        auto &shards = self->getShard(key, sock);
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

    template <typename T>
    RespValue CommandHandler<T>::handleRENAME(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string newkey = args[2].str;

        if (key == newkey)
        {
            return RespValue::simple_string("OK");
        }

        int old_shard_idx = self->getShardIndex(key);
        int new_shard_idx = self->getShardIndex(newkey);

        // 按顺序锁，避免死锁
        int first = old_shard_idx;
        int second = new_shard_idx;
        if (first > second)
        {
            std::swap(first, second);
        }
        auto &m_shards = self->m_dbs[sock->getClientId()];
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

    template <typename T>
    RespValue CommandHandler<T>::handleRENAMENX(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string key = args[1].str;
        const std::string newkey = args[2].str;

        if (key == newkey)
        {
            return RespValue::integer(1);
        }

        int old_shard_idx = self->getShardIndex(key);
        int new_shard_idx = self->getShardIndex(newkey);

        // 按顺序锁，避免死锁
        int first = old_shard_idx;
        int second = new_shard_idx;
        if (first > second)
        {
            std::swap(first, second);
        }
        auto &m_shards = self->m_dbs[sock->getClientId()];
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

    template <typename T>
    RespValue CommandHandler<T>::handleRANDOMKEY(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::vector<std::string> all_keys;
        for (auto &shards : self->m_dbs[sock->getClientId()])
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

    // ========== Server 命令 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleINFO(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::string info;
        // Server
        info += "# Server\r\n";
        info += "redis_version:1.0.0\r\n";
        info += "tcp_port:6666\r\n";
        info += "\r\n";

        // Client
        info += "# Client\r\n";
        info += "connections:" + std::to_string(self->getConnection()) + "\r\n";
        info += "maxclient:" + std::to_string(self->m_config.maxClients) + "\r\n";
        info += "reject_connections:" + std::to_string(self->getRejectConnection()) + "\r\n";
        info += "\r\n";

        // AOF
        info += "# AOF\r\n";
        info += "aof_enabled:" + std::string(self->m_aof.getConfig_AOFEnabled() ? "1" : "0") + "\r\n";
        info += "aof_sync:" + self->m_aof.getConfig_AOFSync() + "\r\n";
        info += "aof_current_file:" + self->m_aof.getCurrentFileName() + "\r\n";
        info += "aof_file_index:" + std::to_string(self->m_aof.getCurrentFileIdx()) + "\r\n";
        info += "aof_current_size:" + std::to_string(self->m_aof.getCurrentFileSize()) + "\r\n";
        info += "aof_max_file_size:" + std::to_string(self->m_aof.getConfig_AOFMaxFileSize()) + "\r\n";
        info += "aof_max_files:" + std::to_string(self->m_aof.getConfig_AOFMaxFileNumber()) + "\r\n";
        info += "aof_max_buffer_size" + std::to_string(self->m_aof.getMaxAOFBufferSize()) + "\r\n";
        info += "\r\n";

        // Monitor
        info += "# Monitor\r\n";
        info += "monitor_clients:" + std::to_string(self->m_monitor.size()) + "\r\n";
        info += "\r\n";

        // Stats
        info += "# Stats\r\n";
        info += "total_connections_received:" + std::to_string(self->getConnection()) + "\r\n";
        info += "total_commands_processed:" + std::to_string(self->m_commands.load(std::memory_order_acquire)) + "\r\n";
        info += "\r\n";

        // Memory
        info += "# Memory\r\n";
        size_t total_keys = 0;
        for (auto &shard : self->m_dbs[sock->getClientId()])
        {
            std::shared_lock lock(shard.mutex);
            total_keys += shard.store.size() + shard.hash.size() + shard.lists.size() + shard.sets.size() + shard.zset.size();
        }
        info += "total_keys:" + std::to_string(total_keys) + "\r\n";
        return RespValue::bulk_string(info);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSAVE(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        self->saveToFile();
        return RespValue::simple_string("OK");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleBGSAVE(std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              bool aof,
                                              CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (self->m_bgsave_running.load(std::memory_order_acquire))
        {
            return RespValue::error("ERR Background save already in progress");
        }
        self->m_bgsave_running.store(true, std::memory_order_release);
        std::thread([self]
                    {
            self->saveToFile();
            self->m_bgsave_running.store(false,std::memory_order_release);
            BLUE_LOG_INFO(xx::g_logger) << "BGSAVE completed"; })
            .detach();
        return RespValue::simple_string("Background saving started");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLASTSAVE(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        return RespValue::integer(self->m_last_time.load(std::memory_order_acquire));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLASTSAVE1(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::time_t beijing_t = self->m_last_time.load(std::memory_order_acquire) + 8 * 3600;
        std::tm time_local;
#ifdef _WIN32
        gmtime_s(&time_local, &beijing_t);
#else
        gmtime_r(&beijing_t, &time_local);
#endif
        std::ostringstream os;
        os << std::put_time(&time_local, "%Y-%m-%d %H:%M:%S");
        return RespValue::bulk_string(os.str());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleCOMMAND(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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

        return RespValue::array(std::move(commands));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleECHO(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        return RespValue::bulk_string(args[1].str);
    }

    template <typename T>
    RespValue CommandHandler<T>::handleTIME(std::vector<RespValue> &args,
                                            MSocket::MSocketPtr sock,
                                            bool aof,
                                            CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        auto now = SteadyClock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
        std::vector<RespValue> results;
        results.push_back(RespValue::bulk_string(std::to_string(seconds)));
        results.push_back(RespValue::bulk_string(std::to_string(microseconds)));
        return RespValue::array(std::move(results));
    }

    template <typename T>
    RespValue CommandHandler<T>::handleLOCALTIME(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
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
        return RespValue::bulk_string(os.str());
    }

    template <typename T>
    RespValue CommandHandler<T>::handleSHUTDOWN(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                CommandHandler<int> *self)
    {
        if (!self->isAdmin(sock))
        {
            return RespValue::error("ERR permission denied");
        }
        self->m_shutdown.store(true, std::memory_order_release);
        return RespValue::bulk_string("OK - waiting for clients to disconnect");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleWATCH(std::vector<RespValue> &args,
                                             MSocket::MSocketPtr sock,
                                             bool aof,
                                             CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        sock->clearWatchedKey();

        for (size_t i = 1; i < args.size(); i++)
        {
            const std::string key = args[i].str;
            sock->addWatchKey(key, self->getKeyVersion(key, sock));
        }
        return RespValue::simple_string("OK");
    }

    template <typename T>
    RespValue CommandHandler<T>::handleUNWATCH(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        sock->clearWatchedKey();
        return RespValue::simple_string("OK");
    }

    // ========== 慢查询 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleSLOWLOG(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::string sub_cmd = args[1].str;
        std::transform(sub_cmd.begin(), sub_cmd.end(), sub_cmd.begin(), ::toupper);
        if (sub_cmd == "GET")
        {
            // 同步数据
            self->m_slowLog.syncSlowLogs();

            int64_t count = 10;
            if (args.size() >= 3)
            {
                try
                {
                    count = std::stoll(args[2].str);
                    if (count < 0)
                    {
                        return RespValue::error("ERR count must be >= 0");
                    }
                }
                catch (...)
                {
                    return RespValue::error("ERR value is not an integer or out of range");
                }
            }

            std::vector<RespValue> results = self->m_slowLog.getSlowLogs(count);
            return RespValue::array(std::move(results));
        }

        else if (sub_cmd == "LEN")
        {
            self->m_slowLog.syncSlowLogs();
            return RespValue::integer(self->m_slowLog.len());
        }
        else if (sub_cmd == "RESET")
        {
            self->m_slowLog.reset();
            return RespValue::simple_string("OK");
        }
        else
        {
            return RespValue::error("ERR unknown SLOWLOG subcommand");
        }
    }

    // ========== 监控 ==========
    template <typename T>
    RespValue CommandHandler<T>::handleMONITOR(std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               bool aof,
                                               CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }

        // 添加monitor client
        self->m_monitor.addMonitorClient(sock);

        sock->setMonitorMode(true);

        return RespValue::simple_string("OK");
    }

    // ========== AOF ==========
    template <typename T>
    RespValue CommandHandler<T>::handleAOFROTATE(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 CommandHandler<int> *self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (!self->m_aof.getConfig_AOFEnabled())
        {
            return RespValue::error("ERR AOF is disabled");
        }
        if (self->m_aof.getAOFRotating())
        {
            return RespValue::error("ERR AOF rotation already in progress");
        }
        std::thread([self]
                    {
            // rotateAOF();
            self->m_aof.rotateAOF();
            BLUE_LOG_INFO(xx::g_logger) << "AOF rotation finished"; })
            .detach();
        return RespValue::simple_string("AOF rotation started");
    }
#else
#endif
}
