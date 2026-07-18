/**
 * @file command_handler_table.h
 * @brief redis server 响应客户端命令的派生类，编译期构建命令表实现，每个命令单独的处理函数，相对好维护和管理
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.28
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#ifdef COMMAND_TABLE
#include <memory>
#include "command_handler_base.h"
#include "blue/tcpServer.h"
#include "redis_command/command_table.h"
#include "redis_command/command_register.h"

namespace blue
{
    template <typename T>
    class CommandHandlerTable : public CommandHandlerBased<T>
    {
    public:
        using SteadyClock = std::chrono::steady_clock;
        using TimePoint = SteadyClock::time_point;

    public:
        /**
         * @brief ifelse处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeIfelse(std::vector<RespValue> args,
                                        MSocket::MSocketPtr sock, std::shared_ptr<ServerData<T>> self,
                                        bool RecordAOF = true) override { return RespValue{}; }

        /**
         * @brief 命令表处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeTable(std::vector<RespValue> args,
                                       MSocket::MSocketPtr sock, std::shared_ptr<ServerData<T>> self,
                                       bool RecordAOF = true) override;

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
        using CommandHandlerFunc = blue::AutoRespValue (*)(std::vector<RespValue> &,
                                                       MSocket::MSocketPtr,
                                                       bool,
                                                       std::shared_ptr<ServerData<int>>);
        // 声明所有命令
        // connect
        REGISTER_COMMAND_T(PING, handlePING);
        REGISTER_COMMAND_T(AUTH, handleAUTH);
        REGISTER_COMMAND_T(SELECT, handleSELECT);
        REGISTER_COMMAND_T(CLIENT, handleCLIENT);
        REGISTER_COMMAND_T(CONFIG, handleCONFIG);

        // string
        REGISTER_COMMAND_T(SET, handleSET);
        REGISTER_COMMAND_T(GET, handleGET);
        REGISTER_COMMAND_T(MSET, handleMSET);
        REGISTER_COMMAND_T(MGET, handleMGET);
        REGISTER_COMMAND_T(GETSET, handleGETSET);
        REGISTER_COMMAND_T(APPEND, handleAPPEND);
        REGISTER_COMMAND_T(SETNX, handleSETNX);
        REGISTER_COMMAND_T(EXISTS, handleEXISTS);
        REGISTER_COMMAND_T(DEL, handleDEL);
        REGISTER_COMMAND_T(INCR, handleINCR);
        REGISTER_COMMAND_T(INCRBY, handleINCRBY);
        REGISTER_COMMAND_T(STRLEN, handleSTRLEN);
        REGISTER_COMMAND_T(TYPE, handleTYPE);

        // hash
        REGISTER_COMMAND_T(HSET, handleHSET);
        REGISTER_COMMAND_T(HGET, handleHGET);
        REGISTER_COMMAND_T(HGETALL, handleHGETALL);
        REGISTER_COMMAND_T(HDEL, handleHDEL);
        REGISTER_COMMAND_T(HLEN, handleHLEN);
        REGISTER_COMMAND_T(HEXISTS, handleHEXISTS);
        REGISTER_COMMAND_T(HKEYS, handleHKEYS);
        REGISTER_COMMAND_T(HVALS, handleHVALS);
        REGISTER_COMMAND_T(KEYS, handleKEYS);
        REGISTER_COMMAND_T(SCAN, handleSCAN);

        // list
        REGISTER_COMMAND_T(LPUSH, handleLPUSH);
        REGISTER_COMMAND_T(RPUSH, handleRPUSH);
        REGISTER_COMMAND_T(LPOP, handleLPOP);
        REGISTER_COMMAND_T(RPOP, handleRPOP);
        REGISTER_COMMAND_T(LLEN, handleLLEN);
        REGISTER_COMMAND_T(LINSERT, handleLINSERT);
        REGISTER_COMMAND_T(LINDEX, handleLINDEX);
        REGISTER_COMMAND_T(LSET, handleLSET);
        REGISTER_COMMAND_T(RPOPLPUSH, handleRPOPLPUSH);
        REGISTER_COMMAND_T(LPOPRPUSH, handleLPOPRPUSH);
        REGISTER_COMMAND_T(LRANGE, handleLRANGE);

        // zset
        REGISTER_COMMAND_T(ZADD, handleZADD);
        REGISTER_COMMAND_T(ZRANGE, handleZRANGE);
        REGISTER_COMMAND_T(ZREM, handleZREM);
        REGISTER_COMMAND_T(ZSCORE, handleZSCORE);
        REGISTER_COMMAND_T(ZRANK, handleZRANK);
        REGISTER_COMMAND_T(ZINCRBY, handleZINCRBY);
        REGISTER_COMMAND_T(ZINCRBYFLOAT, handleZINCRBYFLOAT);
        REGISTER_COMMAND_T(ZCOUNT, handleZCOUNT);
        REGISTER_COMMAND_T(ZRANGEBYSCORE, handleZRANGEBYSCORE);
        REGISTER_COMMAND_T(ZREMRANGEBYSCORE, handleZREMRANGEBYSCORE);

        // set
        REGISTER_COMMAND_T(SADD, handleSADD);
        REGISTER_COMMAND_T(SMEMBERS, handleSMEMBERS);
        REGISTER_COMMAND_T(SREM, handleSREM);
        REGISTER_COMMAND_T(SISMEMBER, handleSISMEMBER);
        REGISTER_COMMAND_T(SCARD, handleSCARD);
        REGISTER_COMMAND_T(SRANDMEMBER, handleSRANDMEMBER);
        REGISTER_COMMAND_T(SPOP, handleSPOP);
        REGISTER_COMMAND_T(SDIFF, handleSDIFF);
        REGISTER_COMMAND_T(SINTER, handleSINTER);
        REGISTER_COMMAND_T(SUNION, handleSUNION);
        REGISTER_COMMAND_T(SMOVE, handleSMOVE);

        // server
        REGISTER_COMMAND_T(FLUSHDB, handleFLUSHDB);
        REGISTER_COMMAND_T(FLUSHDBALL, handleFLUSHDBALL);
        REGISTER_COMMAND_T(DBSIZE, handleDBSIZE);
        REGISTER_COMMAND_T(EXPIRE, handleEXPIRE);
        REGISTER_COMMAND_T(TTL, handleTTL);
        REGISTER_COMMAND_T(PEXPIRE, handlePEXPIRE);
        REGISTER_COMMAND_T(PTTL, handlePTTL);
        REGISTER_COMMAND_T(PERSIST, handlePERSIST);
        REGISTER_COMMAND_T(RENAME, handleRENAME);
        REGISTER_COMMAND_T(RENAMENX, handleRENAMENX);
        REGISTER_COMMAND_T(RANDOMKEY, handleRANDOMKEY);
        REGISTER_COMMAND_T(INFO, handleINFO);
        REGISTER_COMMAND_T(SAVE, handleSAVE);
        REGISTER_COMMAND_T(BGSAVE, handleBGSAVE);
        REGISTER_COMMAND_T(LASTSAVE, handleLASTSAVE);
        REGISTER_COMMAND_T(LASTSAVE1, handleLASTSAVE1);
        REGISTER_COMMAND_T(COMMAND, handleCOMMAND);
        REGISTER_COMMAND_T(ECHO, handleECHO);
        REGISTER_COMMAND_T(TIME, handleTIME);
        REGISTER_COMMAND_T(LOCALTIME, handleLOCALTIME);
        REGISTER_COMMAND_T(WATCH, handleWATCH);
        REGISTER_COMMAND_T(UNWATCH, handleUNWATCH);
        REGISTER_COMMAND_T(SLOWLOG, handleSLOWLOG);
        REGISTER_COMMAND_T(MONITOR, handleMONITOR);
        REGISTER_COMMAND_T(AOFROTATE, handleAOFROTATE);
        REGISTER_COMMAND_T(SHUTDOWN, handleSHUTDOWN);
        REGISTER_COMMAND_T(QUIT, handleQUIT);

        // replication
        REGISTER_COMMAND_T(REPLICAOF, handleREPLICAOF);
        REGISTER_COMMAND_T(SLAVEOF, handleSLAVEOF);
        REGISTER_COMMAND_T(SYNC, handleSYNC);

        // 插入所有命令
        static consteval auto buildCommandTable()
        {
            // 注释的走if-else直接判断
            blue::CommandTableBuilder<256> builder;
            // connect
            CMD_ENTRY_T(PING, handlePING, false, ONLY_ONE_TWO);
            CMD_ENTRY_T(AUTH, handleAUTH, false, ONLY_TWO);
            CMD_ENTRY_T(SELECT, handleSELECT, true, ONLY_TWO);
            CMD_ENTRY_T(CLIENT, handleCLIENT, false, ONLY_TWO_THREE);
            CMD_ENTRY_T(CONFIG, handleCONFIG, false, ONLY_THREE_FOUR);

            // string
            // CMD_ENTRY_T(SET, handleSET, true, ONLY_THREE_SIX);
            // CMD_ENTRY_T(GET, handleGET, false, ONLY_TWO);
            CMD_ENTRY_T(MSET, handleMSET, true, ODD_VALIDATOR);
            CMD_ENTRY_T(MGET, handleMGET, false, ONLY_MORE_TWO);
            CMD_ENTRY_T(GETSET, handleGETSET, true, ONLY_THREE);
            CMD_ENTRY_T(APPEND, handleAPPEND, true, ONLY_THREE);
            CMD_ENTRY_T(SETNX, handleSETNX, true, ONLY_THREE);
            CMD_ENTRY_T(EXISTS, handleEXISTS, false, ONLY_MORE_TWO);
            // CMD_ENTRY_T(DEL, handleDEL, true, ONLY_MORE_TWO);
            CMD_ENTRY_T(INCR, handleINCR, false, ONLY_TWO);
            CMD_ENTRY_T(INCRBY, handleINCRBY, false, ONLY_THREE);
            CMD_ENTRY_T(STRLEN, handleSTRLEN, false, ONLY_TWO);
            CMD_ENTRY_T(TYPE, handleTYPE, false, ONLY_TWO);

            // hash
            // CMD_ENTRY_T(HSET, handleHSET, true, EVEN_VALIDATOR);
            // CMD_ENTRY_T(HGET, handleHGET, false, ONLY_THREE);
            CMD_ENTRY_T(HGETALL, handleHGETALL, false, ONLY_TWO);
            CMD_ENTRY_T(HDEL, handleHDEL, true, ONLY_MORE_THREE);
            CMD_ENTRY_T(HLEN, handleHLEN, false, ONLY_TWO);
            CMD_ENTRY_T(HEXISTS, handleHEXISTS, false, ONLY_THREE);
            CMD_ENTRY_T(HKEYS, handleHKEYS, false, ONLY_TWO);
            CMD_ENTRY_T(HVALS, handleHVALS, false, ONLY_TWO);
            CMD_ENTRY_T(KEYS, handleKEYS, false, ONLY_TWO);
            CMD_ENTRY_T(SCAN, handleSCAN, false, ONLY_MORE_TWO);

            // list
            // CMD_ENTRY_T(LPUSH, handleLPUSH, true, ONLY_MORE_THREE);
            CMD_ENTRY_T(RPUSH, handleRPUSH, true, ONLY_MORE_THREE);
            // CMD_ENTRY_T(LPOP, handleLPOP, true, ONLY_TWO_THREE);
            CMD_ENTRY_T(RPOP, handleRPOP, true, ONLY_TWO_THREE);
            CMD_ENTRY_T(LLEN, handleLLEN, false, ONLY_TWO);
            CMD_ENTRY_T(LINSERT, handleLINSERT, true, ONLY_FIVE);
            CMD_ENTRY_T(LINDEX, handleLINDEX, false, ONLY_THREE);
            CMD_ENTRY_T(LSET, handleLSET, true, ONLY_FOUR);
            CMD_ENTRY_T(RPOPLPUSH, handleRPOPLPUSH, true, ONLY_THREE);
            CMD_ENTRY_T(LPOPRPUSH, handleLPOPRPUSH, true, ONLY_THREE);
            CMD_ENTRY_T(LRANGE, handleLRANGE, false, ONLY_FOUR);

            // zset
            // CMD_ENTRY_T(ZADD, handleZADD, true, EVEN_VALIDATOR);
            CMD_ENTRY_T(ZRANGE, handleZRANGE, false, ONLY_FOUR_FIVE);
            CMD_ENTRY_T(ZREM, handleZREM, true, ONLY_MORE_THREE);
            CMD_ENTRY_T(ZSCORE, handleZSCORE, false, ONLY_THREE);
            CMD_ENTRY_T(ZRANK, handleZRANK, false, ONLY_THREE);
            CMD_ENTRY_T(ZINCRBY, handleZINCRBY, false, ONLY_FOUR);
            CMD_ENTRY_T(ZINCRBYFLOAT, handleZINCRBYFLOAT, false, ONLY_FOUR);
            CMD_ENTRY_T(ZCOUNT, handleZCOUNT, false, ONLY_FOUR);
            CMD_ENTRY_T(ZRANGEBYSCORE, handleZRANGEBYSCORE, false, ONLY_FOUR_FIVE);
            CMD_ENTRY_T(ZREMRANGEBYSCORE, handleZREMRANGEBYSCORE, true, ONLY_FOUR);

            // set
            // CMD_ENTRY_T(SADD, handleSADD, true, ONLY_MORE_THREE);
            CMD_ENTRY_T(SMEMBERS, handleSMEMBERS, false, ONLY_TWO);
            CMD_ENTRY_T(SREM, handleSREM, true, ONLY_MORE_THREE);
            CMD_ENTRY_T(SISMEMBER, handleSISMEMBER, false, ONLY_THREE);
            CMD_ENTRY_T(SCARD, handleSCARD, false, ONLY_TWO);
            CMD_ENTRY_T(SRANDMEMBER, handleSRANDMEMBER, false, ONLY_TWO_THREE);
            CMD_ENTRY_T(SPOP, handleSPOP, true, ONLY_TWO_THREE);
            CMD_ENTRY_T(SDIFF, handleSDIFF, false, ONLY_MORE_TWO);
            CMD_ENTRY_T(SINTER, handleSINTER, false, ONLY_MORE_TWO);
            CMD_ENTRY_T(SUNION, handleSUNION, false, ONLY_MORE_TWO);
            CMD_ENTRY_T(SMOVE, handleSMOVE, true, ONLY_FOUR);

            // server
            CMD_ENTRY_T(FLUSHDB, handleFLUSHDB, true, ONLY_ONE_TWO);
            CMD_ENTRY_T(FLUSHDBALL, handleFLUSHDBALL, true, ONLY_ONE_TWO);
            CMD_ENTRY_T(DBSIZE, handleDBSIZE, false, ONLY_ONE);
            CMD_ENTRY_T(EXPIRE, handleEXPIRE, false, ONLY_THREE);
            CMD_ENTRY_T(TTL, handleTTL, false, ONLY_TWO);
            CMD_ENTRY_T(PEXPIRE, handlePEXPIRE, false, ONLY_THREE);
            CMD_ENTRY_T(PTTL, handlePTTL, false, ONLY_TWO);
            CMD_ENTRY_T(PERSIST, handlePERSIST, false, ONLY_TWO);
            CMD_ENTRY_T(RENAME, handleRENAME, false, ONLY_THREE);
            CMD_ENTRY_T(RENAMENX, handleRENAMENX, false, ONLY_THREE);
            CMD_ENTRY_T(RANDOMKEY, handleRANDOMKEY, false, ONLY_ONE);
            CMD_ENTRY_T(INFO, handleINFO, false, ONLY_ONE);
            CMD_ENTRY_T(SAVE, handleSAVE, false, ONLY_ONE);
            CMD_ENTRY_T(BGSAVE, handleBGSAVE, false, ONLY_ONE);
            CMD_ENTRY_T(LASTSAVE, handleLASTSAVE, false, ONLY_ONE);
            CMD_ENTRY_T(LASTSAVE1, handleLASTSAVE1, false, ONLY_ONE);
            CMD_ENTRY_T(COMMAND, handleCOMMAND, false, ONLY_ONE);
            CMD_ENTRY_T(ECHO, handleECHO, false, ONLY_TWO);
            CMD_ENTRY_T(TIME, handleTIME, false, ONLY_ONE);
            CMD_ENTRY_T(LOCALTIME, handleLOCALTIME, false, ONLY_ONE);
            CMD_ENTRY_T(WATCH, handleWATCH, false, ONLY_MORE_TWO);
            CMD_ENTRY_T(UNWATCH, handleUNWATCH, false, ONLY_ONE);
            CMD_ENTRY_T(SLOWLOG, handleSLOWLOG, false, ONLY_TWO_THREE);
            CMD_ENTRY_T(MONITOR, handleMONITOR, false, ONLY_ONE);
            CMD_ENTRY_T(AOFROTATE, handleAOFROTATE, false, ONLY_ONE);
            CMD_ENTRY_T(SHUTDOWN, handleSHUTDOWN, false, ONLY_ONE);
            CMD_ENTRY_T(QUIT, handleQUIT, false, ONLY_ONE);

            // replication
            CMD_ENTRY_T(REPLICAOF, handleREPLICAOF, false, ONLY_THREE);
            CMD_ENTRY_T(SLAVEOF, handleSLAVEOF, false, ONLY_THREE);
            CMD_ENTRY_T(SYNC, handleSYNC, false, ONLY_ONE);

            return builder.build();
        }
        static const auto &getCommandTable()
        {
            static const auto table = buildCommandTable();
            return table;
        }
    };
    template <typename T>
    inline RespValue CommandHandlerTable<T>::executeTable(std::vector<RespValue> args, MSocket::MSocketPtr sock,
                                                          std::shared_ptr<ServerData<T>> self,
                                                          bool RecordAOF)
    {
        RespValue result;
        auto start = SteadyClock::now();
        if (args.empty())
        {
            return *RespValue::error("ERR empty command");
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

#define IF_CMD(name)                                                   \
    if (cmd == #name)                                                  \
    {                                                                  \
        if (self->getAOF().isWriteCommand(cmd) && RecordAOF)           \
        {                                                              \
            std::string aof_cmds = self->getAOF().formatCommand(args); \
            self->getAOF().appendToAOF(aof_cmds);                      \
            if (self->getReplication().getisMaster() &&                \
                !self->getReplication().slavesEmpty())                 \
            {                                                          \
                self->getReplication().broadcastToSlaves(aof_cmds);    \
            }                                                          \
        }                                                              \
        result = *handle##name(args, sock, RecordAOF, self);                      \
        /* 慢查询记录*/                                                \
        auto end = SteadyClock::now();                                 \
        std::string cmd_str;                                           \
        for (size_t i = 0; i < args.size(); ++i)                       \
        {                                                              \
            if (i > 0)                                                 \
            {                                                          \
                cmd_str += " ";                                        \
            }                                                          \
            cmd_str += args[i].str;                                    \
        }                                                              \
        self->getSlowLog().pushEntry(cmd_str, sock, start, end);       \
        return result;                                                 \
    }

        // 高频命令不走命令表
        HOT_COMMANDS(IF_CMD);

#undef IF_CMD
#undef HOT_COMMANDS

        const auto &table = getCommandTable();
        auto *entry = table.find_lowerbound(fnv1a_hash(cmd.c_str()));
        if (!entry)
        {
            return *RespValue::error("ERR unknown command");
        }

        // 参数验证
        if (entry->argV && !entry->argV(args.size()))
        {
            return *RespValue::error("ERR wrong number of arguments for '" + cmd + "'");
        }

        if (RecordAOF && entry->is_write)
        {
            std::string aof_cmds = self->getAOF().formatCommand(args);
            self->getAOF().appendToAOF(aof_cmds);

            // 如果是主节点，广播给从节点
            if (self->getReplication().getisMaster() && !(self->getReplication().slavesEmpty()))
            {
                self->getReplication().broadcastToSlaves(aof_cmds);
            }
        }

        // 执行命令
        auto handler = entry->handler;
        result = *handler(args, sock, RecordAOF, self);

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
        self->getSlowLog().pushEntry(cmd_str, sock, start, end);

        return result;
    }

    // ========== 连接命令 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handlePING(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
    AutoRespValue CommandHandlerTable<T>::handleAUTH(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        if (args[1].str == self->getPassword())
        {
            if (!self->getAdminSocket().expired())
            {
                return RespValue::error("ERR admin already logged in elsewhere");
            }
            if (sock->getClientlevel() == 1)
            {
                return RespValue::error("ERR this connection already have been logged by client");
            }
            self->setAdminSocket(sock);
            sock->setClientlevel(2);
            return RespValue::simple_string("OK");
        }
        return RespValue::error("ERR invalid password");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSELECT(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
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
    AutoRespValue CommandHandlerTable<T>::handleCLIENT(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
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
        else if (subcmd == "SETINFO")
        {
            if (args.size() != 4)
            {
                return RespValue::error("ERR wrong number of arguments for 'CLIENT SETINFO'");
            }
            return RespValue::simple_string("OK");
        }
        return RespValue::error("ERR wrong arguments for 'CLIENT'");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleCONFIG(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
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
                result.push_back(*RespValue::bulk_string("clientpass"));
                result.push_back(*RespValue::bulk_string(sock->getClientPassword()));
            }
            if (pattern == "*" || pattern == "maxclients")
            {
                result.push_back(*RespValue::bulk_string("maxclients"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getMaxClientCount())));
            }
            if (pattern == "*" || pattern == "timeout")
            {
                result.push_back(*RespValue::bulk_string("timeout"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getTimeoutS())));
            }
            if (pattern == "*" || pattern == "slowlog-log-slower-than" || pattern == "slowlog-*")
            {
                result.push_back(*RespValue::bulk_string("slowlog-log-slower-than"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getSlowLog().getSlowLogThan())));
            }
            if (pattern == "*" || pattern == "slowlog-max-len" || pattern == "slowlog-*")
            {
                result.push_back(*RespValue::bulk_string("slowlog-max-len"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getSlowLog().getSlowMaxLen())));
            }
            if (pattern == "*" || pattern == "aof-enabled" || pattern == "aof-*")
            {
                result.push_back(*RespValue::bulk_string("aof-enabled"));
                result.push_back(*RespValue::bulk_string(self->getAOF().getConfig_AOFEnabled() ? "yes" : "no"));
            }
            if (pattern == "*" || pattern == "aof-filename" || pattern == "aof-*")
            {
                result.push_back(*RespValue::bulk_string("aof-filename"));
                result.push_back(*RespValue::bulk_string(self->getAOF().getConfig_AOFFilename()));
            }
            if (pattern == "*" || pattern == "aof-sync" || pattern == "aof-*")
            {
                result.push_back(*RespValue::bulk_string("aof-sync"));
                result.push_back(*RespValue::bulk_string(self->getAOF().getConfig_AOFSync()));
            }
            if (pattern == "*" || pattern == "aof-max_file_size" || pattern == "aof-*")
            {
                result.push_back(*RespValue::bulk_string("aof-max_file_size"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getAOF().getConfig_AOFMaxFileSize())));
            }
            if (pattern == "*" || pattern == "aof-max_file_number" || pattern == "aof-*")
            {
                result.push_back(*RespValue::bulk_string("aof-max_file_number"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getAOF().getConfig_AOFMaxFileNumber())));
            }
            if (pattern == "*" || pattern == "aof-max_buffer_size" || pattern == "aof-*")
            {
                result.push_back(*RespValue::bulk_string("aof-max_buffer_size"));
                result.push_back(*RespValue::bulk_string(std::to_string(self->getAOF().getMaxAOFBufferSize())));
            }
            if (pattern == "*" || pattern == "database")
            {
                result.push_back(*RespValue::bulk_string("database"));
                result.push_back(*RespValue::bulk_string("16"));
            }
            return RespValue::array(std::move(result));
        }
        else if (subcmd == "SET")
        {
            if (args.size() != 4)
            {
                return RespValue::error("ERR wrong number of arguments for 'CONFIG SET'");
            }

            const std::string &param = args[2].str;
            const std::string &value = args[3].str;

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
                    self->getSlowLog().setSlowLogThan(val);
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
                    self->getSlowLog().setSlowMaxLen(val);
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
                    self->getAOF().setConfig_AOFEnabled(true);
                    self->getAOF().initAOF();
                }
                else if (value == "no" || value == "0")
                {
                    self->getAOF().setConfig_AOFEnabled(false);
                    self->getAOF().closeAOF();
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
                    self->getAOF().setConfig_AOFSync(value);
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
                self->getAOF().setConfig_AOFFilename(value);
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

                self->getAOF().setConfig_AOFMaxFileSize(val);
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
                self->getAOF().setConfig_AOFMaxFileNumber(val);
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
                self->getAOF().setMaxAOFBufferSize(val);
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
                    self->setMaxClientCount(newmax);
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
                    self->setTimeoutS(timeout);
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
    AutoRespValue CommandHandlerTable<T>::handleSET(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'SET'");
        }
        const std::string &key = args[1].str;
        const std::string &val = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        std::optional<TimePoint> timepoint;
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
                timepoint = SteadyClock::now() + std::chrono::seconds(seconds);
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
                timepoint = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
            }
        }
        // shards.store[key] = DataShard::StoreData(val, timepoint);
        shards.store.insert_or_assign(key, DataShard::StoreData(val, timepoint));
        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleGET(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 2)
        {
            return RespValue::error("ERR wrong number of arguments for 'GET'");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::null_bulk();
        }
        if (it->second.is_expired())
        {
            lock.unlock();
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            shards.store.erase(key);
            return RespValue::null_bulk();
        }

        const std::string &val = it->second.val;
        return RespValue::bulk_string(val);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleMSET(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        for (size_t i = 1; i < args.size(); i += 2)
        {
            const std::string &key = args[i].str;
            const std::string &val = args[i + 1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            // shards.store[key] = DataShard::StoreData(val);
            shards.store.insert_or_assign(key, DataShard::StoreData(val));
        }
        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleMGET(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::vector<RespValue> results;

        for (size_t i = 1; i < args.size(); i++)
        {
            const std::string &key = args[i].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                results.push_back(*RespValue::null_bulk());
            }
            else
            {
                if (it->second.is_expired())
                {
                    lock.unlock();
                    std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
                    shards.store.erase(key);
                    results.push_back(*RespValue::null_bulk());
                }
                else
                {
                    results.push_back(*RespValue::bulk_string(it->second.val));
                }
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleGETSET(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &val = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            // shards.store[key] = DataShard::StoreData(val);
            shards.store.insert_or_assign(key, DataShard::StoreData(val));
            return RespValue::bulk_string(val);
        }
        std::string ans = it->second.val;
        shards.store.insert_or_assign(key, DataShard::StoreData(val, it->second.expire));
        return RespValue::bulk_string(ans);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleAPPEND(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &val = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            // shards.store[key] = DataShard::StoreData(val);
            shards.store.insert_or_assign(key, DataShard::StoreData(val));
        }
        else
        {
            std::string new_val = it->second.val + val;
            shards.store.insert_or_assign(key, DataShard::StoreData(new_val, it->second.expire));
        }
        return RespValue::integer(shards.store[key].val.size());
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSETNX(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &val = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            // shards.store[key] = DataShard::StoreData(val);
            shards.store.insert_or_assign(key, DataShard::StoreData(val));
            return RespValue::integer(1);
        }
        return RespValue::integer(0);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleEXISTS(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int64_t count = 0;
        for (size_t i = 1; i < args.size(); i++)
        {
            const std::string &key = args[i].str;
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
    AutoRespValue CommandHandlerTable<T>::handleDEL(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                std::shared_ptr<ServerData<int>> self)
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
            const std::string &key = args[i].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it != shards.store.end())
            {
                shards.store.erase(it);
                count++;
            }
        }
        return RespValue::integer(count);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleINCR(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        int64_t val = 0;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> rdlock(shards.mutex);
        auto it = shards.store.find(key);
        if (it != shards.store.end())
        {
            try
            {
                val = std::stoll(it->second.val);
            }
            catch (...)
            {
                return RespValue::error("ERR value is not a integer or out of range");
            }
            val++;
            // it->second.val = std::to_string(val);
            shards.store.insert_or_assign(key, DataShard::StoreData(std::to_string(val), it->second.expire));
        }
        else
        {
            val++;
            // shards.store[key] = DataShard::StoreData(std::to_string(val));
            shards.store.insert_or_assign(key, DataShard::StoreData(std::to_string(val)));
        }
        return RespValue::integer(val);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleINCRBY(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
                val = std::stoll(it->second.val);
            }
            catch (...)
            {
                return RespValue::error("ERR value is not a integer or out of range");
            }
            val += increment;
            // it->second.val = std::to_string(val);
            shards.store.insert_or_assign(key, DataShard::StoreData(std::to_string(val), it->second.expire));
        }
        else
        {
            val += increment;
            // shards.store[key] = DataShard::StoreData(std::to_string(val));
            shards.store.insert_or_assign(key, DataShard::StoreData(std::to_string(val)));
        }
        return RespValue::integer(val);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSTRLEN(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
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
        if (it->second.is_expired())
        {
            // lock.unlock();
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            shards.store.erase(key);
            return RespValue::integer(0);
        }
        return RespValue::integer(it->second.val.size());
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleTYPE(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleKEYS(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }

        // 把 glob 风格转成 regex
        std::string pattern = args[1].str;
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

        constexpr size_t MAX_KEYS = 10000;
        std::regex re(regex_str);
        std::vector<RespValue> result;
        result.reserve(MAX_KEYS);

        // 遍历所有分片
        for (auto &shard : self->getDBs()[sock->getClientId()])
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
                    result.push_back(*RespValue::bulk_string(key));
                }
            }
            // Hash 类型的 key
            for (auto &[key, fields] : shard.hash)
            {
                if (std::regex_match(key, re))
                {
                    result.push_back(*RespValue::bulk_string(key));
                }
            }
            // List 类型的 key
            for (auto &[key, list] : shard.lists)
            {
                if (std::regex_match(key, re))
                {
                    result.push_back(*RespValue::bulk_string(key));
                }
            }
            // Set 类型的 key
            for (auto &[key, set] : shard.sets)
            {
                if (std::regex_match(key, re))
                {
                    result.push_back(*RespValue::bulk_string(key));
                }
            }
            // ZSet 类型的 key
            for (auto &[key, zset] : shard.zset)
            {
                if (std::regex_match(key, re))
                {
                    result.push_back(*RespValue::bulk_string(key));
                }
            }
        }

        return RespValue::array(std::move(result));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSCAN(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 2)
        {
            return RespValue::error("ERR wrong number of arguments for 'SCAN'");
        }

        // 解析游标
        ScanCursor cursor;
        if (sock->hasScanCursor())
        {
            cursor = sock->getScanCursor();
            // 如果游标已完成，检查客户端是否传入了新游标
            if (cursor.completed)
            {
                // 客户端可能传入了 "0" 表示重新开始
                if (args[1].str == "0")
                {
                    cursor = ScanCursor{};
                }
            }
        }
        else
        {
            BLUE_LOG_INFO(xx::g_logger) << "sock have not ScanCursor";
            cursor = ScanCursor{};
        }

        // 如果客户端传入了游标，用客户端的值覆盖
        if (args[1].str != "0")
        {
            // 解析客户端传来的游标
            auto clientCursor = ScanCursor::deserialize(args[1].str);
            if (!clientCursor.completed)
            {
                cursor = clientCursor;
            }
        }

        // 解析可选参数
        std::string pattern = "*";
        int count = 10;

        for (size_t i = 2; i < args.size(); i++)
        {
            std::string arg = args[i].str;
            std::transform(arg.begin(), arg.end(), arg.begin(), ::toupper);

            if (arg == "MATCH" && i + 1 < args.size())
            {
                pattern = args[++i].str;
            }
            else if (arg == "COUNT" && i + 1 < args.size())
            {
                try
                {
                    count = std::stoi(args[++i].str);
                    if (count <= 0)
                        count = 1;
                }
                catch (...)
                {
                    return RespValue::error("ERR invalid COUNT");
                }
            }
        }

        // 获取当前数据库编号
        int dbIndex = sock->getClientId() % DB_COUNT;
        cursor.db = dbIndex;

        // 执行扫描
        std::vector<std::string> keys;
        bool completed = self->scanKeys(dbIndex, cursor, pattern, count, keys);

        // 保存游标状态
        if (!completed)
        {
            sock->setScanCursor(cursor);
        }
        else
        {
            sock->clearScanCursor();
        }

        // 构造响应
        std::vector<RespValue> result;
        if (completed)
        {
            result.push_back(*RespValue::bulk_string("0"));
        }
        else
        {
            // 返回类型为 ShardIndex:DataType:OffSet
            result.push_back(*RespValue::bulk_string(cursor.serialize()));
        }

        std::vector<RespValue> keysArray;
        keysArray.reserve(keys.size());
        for (const auto &key : keys)
        {
            keysArray.push_back(*RespValue::bulk_string(key));
        }
        result.push_back(*RespValue::array(keysArray));

        return RespValue::array(result);
    }

    // ========== Hash 命令 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHSET(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
        for (size_t i = 2; i < args.size(); i += 2)
        {
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &field = args[i].str;
            auto &value = args[i + 1].str; // size 是偶数所以不会出界
            auto it = shards.hash.find(key);
            if (it != shards.hash.end())
            {
                auto &inner_map = const_cast<absl::flat_hash_map<std::string, std::string> &>(it->second);
                if (inner_map.find(field) == inner_map.end())
                {
                    count++;
                }
                inner_map.insert_or_assign(field, value);
            }
            else
            {
                // key 不存在，创建新 map
                absl::flat_hash_map<std::string, std::string> new_map;
                new_map.insert_or_assign(field, value);
                shards.hash.insert_or_assign(key, std::move(new_map));
                count++;
            }
        }
        return RespValue::integer(count);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHGET(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'HGET'");
        }
        const std::string &key = args[1].str;
        const std::string &field = args[2].str;
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
        const std::string &val = fit->second;
        return RespValue::bulk_string(val);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHGETALL(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::vector<RespValue> result;
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.hash.find(key);
        if (it == shards.hash.end())
        {
            return RespValue::array(std::move(result));
        }
        for (const auto &[field, value] : it->second)
        {
            result.push_back(*RespValue::bulk_string(field));
            result.push_back(*RespValue::bulk_string(value));
        }
        return RespValue::array(std::move(result));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHDEL(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int count = 0;
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
        auto it = shards.hash.find(key);
        if (it == shards.hash.end())
        {
            return RespValue::integer(count);
        }
        auto &inner_map = const_cast<absl::flat_hash_map<std::string, std::string> &>(it->second);
        for (size_t i = 2; i < args.size(); i++)
        {
            count += inner_map.erase(args[i].str);
        }
        if (inner_map.empty())
        {
            shards.hash.erase(key);
        }
        return RespValue::integer(count);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHLEN(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleHEXISTS(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &field = args[2].str;
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
        auto field_it = it->second.find(field);
        if (field_it == it->second.end())
        {
            return RespValue::integer(0);
        }
        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHKEYS(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
        for (const auto &[field, _] : it->second)
        {
            results.push_back(*RespValue::bulk_string(field));
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleHVALS(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
        for (const auto &[_, val] : it->second)
        {
            results.push_back(*RespValue::bulk_string(val));
        }
        return RespValue::array(std::move(results));
    }

    // ========== List 命令 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLPUSH(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'LPUSH'");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.lists.find(key);
        if (it == shards.lists.end())
        {
            std::list<std::string> new_list;
            for (size_t i = 2; i < args.size(); i++)
            {
                new_list.push_front(args[i].str);
            }
            shards.lists.insert_or_assign(key, std::move(new_list));
            return RespValue::integer(args.size() - 2);
        }
        auto &list = const_cast<std::list<std::string> &>(it->second);
        for (size_t i = 2; i < args.size(); i++)
        {
            list.push_front(args[i].str);
        }
        return RespValue::integer(list.size());
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleRPUSH(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.lists.find(key);
        if (it == shards.lists.end())
        {
            std::list<std::string> new_list;
            for (size_t i = 2; i < args.size(); i++)
            {
                new_list.push_back(args[i].str);
            }
            shards.lists.insert_or_assign(key, std::move(new_list));
            return RespValue::integer(args.size() - 2);
        }
        auto &lhs = const_cast<std::list<std::string> &>(it->second);
        for (size_t i = 2; i < args.size(); i++)
        {
            lhs.push_back(args[i].str);
        }
        return RespValue::integer(lhs.size());
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLPOP(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
        auto it = shards.lists.find(key);
        if (it == shards.lists.end() || it->second.empty())
        {
            return RespValue::null_bulk();
        }
        std::vector<RespValue> results;
        auto &list = const_cast<std::list<std::string> &>(it->second);
        for (int i = 0; i < count && !it->second.empty(); i++)
        {
            results.push_back(*RespValue::bulk_string(it->second.front()));
            list.pop_front();
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
    AutoRespValue CommandHandlerTable<T>::handleRPOP(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
        auto it = shards.lists.find(key);
        if (it == shards.lists.end() || it->second.empty())
        {
            return RespValue::null_bulk();
        }
        std::vector<RespValue> results;
        auto &list = const_cast<std::list<std::string> &>(it->second);
        for (int i = 0; i < count && !it->second.empty(); i++)
        {
            results.push_back(*RespValue::bulk_string(it->second.back()));
            list.pop_back();
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
    AutoRespValue CommandHandlerTable<T>::handleLLEN(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleLINSERT(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &pos = args[2].str;
        const std::string &pivot = args[3].str;
        const std::string &val = args[4].str;

        auto &shard = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.lists.find(key);
        if (it == shard.lists.end())
        {
            return RespValue::null_bulk();
        }
        std::list<std::string> &lists = const_cast<std::list<std::string> &>(it->second);
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
    AutoRespValue CommandHandlerTable<T>::handleLINDEX(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
        auto &list = const_cast<std::list<std::string> &>(it->second);
        auto list_it = list.begin();
        while (idx--)
        {
            list_it++;
        }
        return RespValue::bulk_string(*list_it);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLSET(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &val = args[3].str;
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
        std::unique_lock<std::shared_mutex> lock(shard.mutex);
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
        auto &list = const_cast<std::list<std::string> &>(it->second);
        auto list_it = list.begin();
        while (idx--)
        {
            list_it++;
        }
        *list_it = val;
        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleRPOPLPUSH(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &source_key = args[1].str;
        const std::string &dest_key = args[2].str;
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
            // dest_shard.lists[dest_key] = std::list<std::string>();
            dest_shard.lists.insert_or_assign(dest_key, std::list<std::string>());
            dest_it = dest_shard.lists.find(dest_key);
        }
        const std::string &tem = src_it->second.back();
        auto &src = const_cast<std::list<std::string> &>(src_it->second);
        auto &dest = const_cast<std::list<std::string> &>(dest_it->second);
        src.pop_back();
        dest.push_front(tem);
        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLPOPRPUSH(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &source_key = args[1].str;
        const std::string &dest_key = args[2].str;
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
            // dest_shard.lists[dest_key] = std::list<std::string>();
            dest_shard.lists.insert_or_assign(dest_key, std::list<std::string>());
            dest_it = dest_shard.lists.find(dest_key);
        }
        const std::string &tem = src_it->second.front();
        auto &src = const_cast<std::list<std::string> &>(src_it->second);
        auto &dest = const_cast<std::list<std::string> &>(dest_it->second);
        src.pop_front();
        dest.push_back(tem);
        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLRANGE(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
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
        auto &list = const_cast<std::list<std::string> &>(it->second);
        auto iter = list.begin();
        std::advance(iter, start);
        for (int i = start; i <= stop && iter != list.end(); i++, iter++)
        {
            result.push_back(*RespValue::bulk_string(*iter));
        }
        return RespValue::array(std::move(result));
    }

    // ========== Set 命令 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSADD(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() < 3)
        {
            return RespValue::error("ERR wrong number of argument for 'SADD'");
        }
        const std::string &key = args[1].str;
        int32_t count = 0;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        for (size_t i = 2; i < args.size(); i++)
        {
            const std::string &member = args[i].str;
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
    AutoRespValue CommandHandlerTable<T>::handleSMEMBERS(std::vector<RespValue> &args,
                                                     MSocket::MSocketPtr sock,
                                                     bool aof,
                                                     std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        std::vector<RespValue> results;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.sets.find(key);
        if (it == shards.sets.end())
        {
            return RespValue::array(std::move(results));
        }
        for (const auto &member : it->second)
        {
            results.push_back(*RespValue::bulk_string(member));
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSREM(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleSISMEMBER(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleSCARD(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleSRANDMEMBER(std::vector<RespValue> &args,
                                                        MSocket::MSocketPtr sock,
                                                        bool aof,
                                                        std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
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
                results.push_back(*RespValue::bulk_string(members[i]));
            }
        }
        else
        {
            // 负数：可重复
            int num = -count;
            for (int i = 0; i < num; i++)
            {
                int idx = rand() % members.size();
                results.push_back(*RespValue::bulk_string(members[idx]));
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSPOP(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
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
        auto &set = const_cast<std::unordered_set<std::string> &>(it->second);
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
                results.push_back(*RespValue::bulk_string(members[i]));
            }
            if (set.empty())
            {
                shards.sets.erase(it);
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSDIFF(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::vector<RespValue> results;
        auto it = shard.sets.find(key);
        if (it == shard.sets.end())
        {
            return RespValue::array({});
        }
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto &member : it->second)
        {
            bool ok = true;
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string tem_key = args[i].str;
                auto &tem_shard = self->getShard(tem_key, sock);
                auto tem_it = tem_shard.sets.find(tem_key);
                if (tem_it == tem_shard.sets.end())
                {
                    continue;
                }
                std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                auto &tem_members = tem_it->second;
                if (tem_members.contains(member))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
            {
                results.push_back(*RespValue::bulk_string(member));
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSINTER(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::vector<RespValue> results;
        auto it = shard.sets.find(key);
        if (it == shard.sets.end())
        {
            return RespValue::array({});
        }
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto &member : it->second)
        {
            bool ok = true;
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string tem_key = args[i].str;
                auto &tem_shard = self->getShard(tem_key, sock);
                auto tem_it = tem_shard.sets.find(tem_key);
                if (tem_it == tem_shard.sets.end())
                {
                    return RespValue::array({});
                }
                std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                auto &tem_members = tem_it->second;
                if (!tem_members.contains(member))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
            {
                results.push_back(*RespValue::bulk_string(member));
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSUNION(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shard = self->getShard(key, sock);
        std::unordered_set<std::string> results_set;
        auto it = shard.sets.find(key);
        if (it == shard.sets.end())
        {
            return RespValue::array({});
        }
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto &member : it->second)
        {
            results_set.insert(member);
        }
        for (size_t i = 2; i < args.size(); i++)
        {
            const std::string tem_key = args[i].str;
            auto &tem_shard = self->getShard(tem_key, sock);
            auto tem_it = tem_shard.sets.find(tem_key);
            if (tem_it == tem_shard.sets.end())
            {
                continue;
            }
            std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
            auto &tem_members = tem_it->second;
            for (const auto &tem_member : tem_members)
            {
                results_set.insert(tem_member);
            }
        }
        // lock.unlock();
        std::vector<RespValue> results;
        for (const auto &member : results_set)
        {
            results.push_back(*RespValue::bulk_string(member));
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSMOVE(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &source_key = args[1].str;
        const std::string &destination_key = args[2].str;
        if (source_key == destination_key)
        {
            return RespValue::integer(1);
        }
        const std::string &member = args[3].str;
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
            // dest_shard.sets[destination_key] = std::unordered_set<std::string>();
            dest_shard.sets.insert_or_assign(destination_key, std::unordered_set<std::string>());
            dest_it = dest_shard.sets.find(destination_key);
        }
        // 检查member是否在源集合中
        if (src_it->second.find(member) == src_it->second.end())
        {
            return RespValue::integer(0);
        }
        auto &src = const_cast<std::unordered_set<std::string> &>(src_it->second);
        auto &dest = const_cast<std::unordered_set<std::string> &>(dest_it->second);
        src.erase(member);
        dest.insert(member);
        if (src.empty())
        {
            src_shard.sets.erase(src_it);
        }
        return RespValue::integer(1);
    }

    // ========== ZSet 命令 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleZADD(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
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
            const std::string &member = args[i + 1].str;

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
    AutoRespValue CommandHandlerTable<T>::handleZRANGE(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
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
            results.push_back(*RespValue::bulk_string(node->val));
            if (withscores)
            {
                results.push_back(*RespValue::bulk_string(self->format_score(node->key.score)));
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleZREM(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int count = 0;
        const std::string &key = args[1].str;
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
                const std::string &member = args[i].str;
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
    AutoRespValue CommandHandlerTable<T>::handleZSCORE(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
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
    AutoRespValue CommandHandlerTable<T>::handleZRANK(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str, member = args[2].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
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
    AutoRespValue CommandHandlerTable<T>::handleZINCRBY(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &member = args[3].str;
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
    AutoRespValue CommandHandlerTable<T>::handleZINCRBYFLOAT(std::vector<RespValue> &args,
                                                         MSocket::MSocketPtr sock,
                                                         bool aof,
                                                         std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &member = args[3].str;
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
    AutoRespValue CommandHandlerTable<T>::handleZCOUNT(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleZRANGEBYSCORE(std::vector<RespValue> &args,
                                                          MSocket::MSocketPtr sock,
                                                          bool aof,
                                                          std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
                results.push_back(*RespValue::bulk_string(member));
                if (withscore)
                {
                    results.push_back(*RespValue::bulk_string((self->format_score(score))));
                }
            }
        }
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleZREMRANGEBYSCORE(std::vector<RespValue> &args,
                                                             MSocket::MSocketPtr sock,
                                                             bool aof,
                                                             std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
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
    AutoRespValue CommandHandlerTable<T>::handleFLUSHDB(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
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
            for (auto &shards : self->getDBs()[sock->getClientId()])
            {
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                shards.store.clear();
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
                for (auto &shards : self->getDBs()[sock->getClientId()])
                {
                    std::unique_lock<std::shared_mutex> lock(shards.mutex);
                    shards.store.clear();
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
    AutoRespValue CommandHandlerTable<T>::handleFLUSHDBALL(std::vector<RespValue> &args,
                                                       MSocket::MSocketPtr sock,
                                                       bool aof,
                                                       std::shared_ptr<ServerData<int>> self)
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
                for (auto &shards : self->getDBs()[db])
                {
                    std::unique_lock<std::shared_mutex> lock(shards.mutex);
                    shards.store.clear();
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
                    for (auto &shards : self->getDBs()[db])
                    {
                        std::unique_lock<std::shared_mutex> lock(shards.mutex);
                        shards.store.clear();
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
    AutoRespValue CommandHandlerTable<T>::handleDBSIZE(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        int64_t count = 0;
        for (auto &shards : self->getDBs()[sock->getClientId()])
        {
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            count += shards.store.size() + shards.lists.size() + shards.hash.size() + shards.zset.size();
        }
        return RespValue::integer(count);
    }

    // ========== Key 命令 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleEXPIRE(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::integer(0);
        }
        shards.store.insert_or_assign(key, DataShard::StoreData(it->second.val, SteadyClock::now() + std::chrono::seconds(second)));
        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleTTL(std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                bool aof,
                                                std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::integer(-2);
        }
        else if (!(it->second.expire.has_value()))
        {
            return RespValue::integer(-1);
        }
        auto &data = it->second;
        // 计算剩余秒数
        auto now = SteadyClock::now();
        if (now >= data.expire.value())
        {
            return RespValue::integer(-2);
        }

        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                             data.expire.value() - now)
                             .count();

        return RespValue::integer(remaining);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handlePEXPIRE(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
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
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::integer(0);
        }
        shards.store.insert_or_assign(key, DataShard::StoreData(it->second.val, SteadyClock::now() + std::chrono::milliseconds(milliseconds)));
        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handlePTTL(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::shared_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        if (it == shards.store.end())
        {
            return RespValue::integer(-2);
        }
        else if (!(it->second.expire.has_value()))
        {
            return RespValue::integer(-1);
        }
        auto &data = it->second;
        // 计算剩余秒数
        auto now = SteadyClock::now();
        if (now >= data.expire.value())
        {
            return RespValue::integer(-2);
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                             data.expire.value() - now)
                             .count();

        return RespValue::integer(remaining);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handlePERSIST(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        auto &shards = self->getShard(key, sock);
        std::unique_lock<std::shared_mutex> lock(shards.mutex);
        auto it = shards.store.find(key);
        // 不存在或没有过期时间
        if (it == shards.store.end() || !(it->second.expire.has_value()))
        {
            return RespValue::integer(0);
        }
        shards.store.insert_or_assign(key, DataShard::StoreData(it->second.val, std::nullopt));
        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleRENAME(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &newkey = args[2].str;

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
        auto &m_shards = self->getDBs()[sock->getClientId()];
        // 锁住zset
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
        int type = -1;

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

        // 移动数据
        if (type == 0)
        {
            auto it = old_shard.store.find(key);
            if (it != old_shard.store.end())
            {
                new_shard.store.insert_or_assign(newkey, it->second);
                old_shard.store.erase(key);
            }
        }
        else if (type == 1)
        {
            auto it = old_shard.hash.find(key);
            if (it != old_shard.hash.end())
            {
                absl::flat_hash_map<std::string, std::string> new_hash;
                for (auto &[field, value] : it->second)
                {
                    new_hash.insert_or_assign(field, value);
                }
                new_shard.hash.insert_or_assign(newkey, std::move(new_hash));
                old_shard.hash.erase(key);
            }
        }
        else if (type == 2)
        {
            auto it = old_shard.lists.find(key);
            if (it != old_shard.lists.end())
            {
                new_shard.lists.insert_or_assign(newkey, std::move(it->second));
                old_shard.lists.erase(key);
            }
        }
        else if (type == 3)
        {
            auto it = old_shard.sets.find(key);
            if (it != old_shard.sets.end())
            {
                new_shard.sets.insert_or_assign(newkey, std::move(it->second));
                old_shard.sets.erase(key);
            }
        }
        else if (type == 4)
        {
            if (old_shard_idx == new_shard_idx)
            {
                new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                old_shard.zset.erase(key);
                old_shard.zset_score.erase(key);
            }
            else
            {
                auto it = old_shard.zset.find(key);
                if (it != old_shard.zset.end())
                {
                    new_shard.zset.emplace(newkey, std::move(it->second));
                    old_shard.zset.erase(key);
                }
                auto score_it = old_shard.zset_score.find(key);
                if (score_it != old_shard.zset_score.end())
                {
                    new_shard.zset_score.emplace(newkey, std::move(score_it->second));
                    old_shard.zset_score.erase(key);
                }
            }
        }

        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleRENAMENX(std::vector<RespValue> &args,
                                                     MSocket::MSocketPtr sock,
                                                     bool aof,
                                                     std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        const std::string &key = args[1].str;
        const std::string &newkey = args[2].str;

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
        auto &m_shards = self->getDBs()[sock->getClientId()];
        // 锁住zset
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
        int type = -1;

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

        // 查看 newkey 是否存在
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
        if (type == 0)
        {
            auto it = old_shard.store.find(key);
            if (it != old_shard.store.end())
            {
                new_shard.store.insert_or_assign(newkey, it->second);
                old_shard.store.erase(key);
            }
        }
        else if (type == 1)
        {
            auto it = old_shard.hash.find(key);
            if (it != old_shard.hash.end())
            {
                absl::flat_hash_map<std::string, std::string> new_hash;
                for (auto &[field, value] : it->second)
                {
                    new_hash.insert_or_assign(field, value);
                }
                new_shard.hash.insert_or_assign(newkey, std::move(new_hash));
                old_shard.hash.erase(key);
            }
        }
        else if (type == 2)
        {
            auto it = old_shard.lists.find(key);
            if (it != old_shard.lists.end())
            {
                new_shard.lists.insert_or_assign(newkey, std::move(it->second));
                old_shard.lists.erase(key);
            }
        }
        else if (type == 3)
        {
            auto it = old_shard.sets.find(key);
            if (it != old_shard.sets.end())
            {
                new_shard.sets.insert_or_assign(newkey, std::move(it->second));
                old_shard.sets.erase(key);
            }
        }
        else if (type == 4)
        {
            if (old_shard_idx == new_shard_idx)
            {
                new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                old_shard.zset.erase(key);
                old_shard.zset_score.erase(key);
            }
            else
            {
                auto it = old_shard.zset.find(key);
                if (it != old_shard.zset.end())
                {
                    new_shard.zset.emplace(newkey, std::move(it->second));
                    old_shard.zset.erase(key);
                }
                auto score_it = old_shard.zset_score.find(key);
                if (score_it != old_shard.zset_score.end())
                {
                    new_shard.zset_score.emplace(newkey, std::move(score_it->second));
                    old_shard.zset_score.erase(key);
                }
            }
        }

        return RespValue::integer(1);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleRANDOMKEY(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::vector<std::string> all_keys;
        for (auto &shards : self->getDBs()[sock->getClientId()])
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
    AutoRespValue CommandHandlerTable<T>::handleINFO(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
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
        info += "maxclient:" + std::to_string(self->getMaxClientCount()) + "\r\n";
        info += "reject_connections:" + std::to_string(self->getRejectConnection()) + "\r\n";
        info += "\r\n";

        // AOF
        info += "# AOF\r\n";
        info += "aof_enabled:" + std::string(self->getAOF().getConfig_AOFEnabled() ? "1" : "0") + "\r\n";
        info += "aof_sync:" + self->getAOF().getConfig_AOFSync() + "\r\n";
        info += "aof_current_file:" + self->getAOF().getCurrentFileName() + "\r\n";
        info += "aof_file_index:" + std::to_string(self->getAOF().getCurrentFileIdx()) + "\r\n";
        info += "aof_current_size:" + std::to_string(self->getAOF().getCurrentFileSize()) + "\r\n";
        info += "aof_max_file_size:" + std::to_string(self->getAOF().getConfig_AOFMaxFileSize()) + "\r\n";
        info += "aof_max_files:" + std::to_string(self->getAOF().getConfig_AOFMaxFileNumber()) + "\r\n";
        info += "aof_max_buffer_size" + std::to_string(self->getAOF().getMaxAOFBufferSize()) + "\r\n";
        info += "\r\n";

        // Replication
        info += "# Replication\r\n";

        if (self->getReplication().getisMaster())
        {
            info += "role:master\r\n";
            info += "connected_slaves:" + std::to_string(self->getReplication().slavesCount()) + "\r\n";

            // 列出所有从节点
            auto slaves_info = self->getReplication().slavesToString();
            if (!slaves_info.empty())
            {
                info += slaves_info;
            }
        }
        else
        {
            info += "role:slave\r\n";
            info += "master_host:" + self->getReplication().getMasterHost() + "\r\n";
            info += "master_port:" + std::to_string(self->getReplication().getMasterPort()) + "\r\n";
            info += "master_link_status:" + std::string(self->getReplication().getReplState() == self->getReplication().getOnline() ? "up" : "down") + "\r\n";
            info += "slave_repl_offset:" + std::to_string(self->getReplication().getReplOffset()) + "\r\n";
        }

        info += "\r\n";

        // Monitor
        info += "# Monitor\r\n";
        info += "monitor_clients:" + std::to_string(self->getMonitor().size()) + "\r\n";
        info += "\r\n";

        // Stats
        info += "# Stats\r\n";
        info += "total_connections_received:" + std::to_string(self->getConnection()) + "\r\n";
        info += "total_commands_processed:" + std::to_string(self->getCommands().load(std::memory_order_acquire)) + "\r\n";
        info += "\r\n";

        // Memory
        info += "# Memory\r\n";
        size_t total_keys = 0;
        for (auto &shard : self->getDBs()[sock->getClientId()])
        {
            // std::shared_lock lock(shard.mutex);
            total_keys += shard.store.size() + shard.hash.size() + shard.lists.size() + shard.sets.size() + shard.zset.size();
        }
        info += "total_keys:" + std::to_string(total_keys) + "\r\n";
        return RespValue::bulk_string(info);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSAVE(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        self->saveToFile();
        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleBGSAVE(std::vector<RespValue> &args,
                                                   MSocket::MSocketPtr sock,
                                                   bool aof,
                                                   std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (self->getBgSaveRunning())
        {
            return RespValue::error("ERR Background save already in progress");
        }
        self->setBgSaveRunning(true);
        std::weak_ptr<ServerData<int>> weak_self = self;
        std::thread([weak_self]
                    {
                auto ptr = weak_self.lock();
                if (ptr)
                {
                    ptr->saveToFile();
                    ptr->setBgSaveRunning(false);
                    BLUE_LOG_INFO(xx::g_logger) << "BGSAVE completed";
                }
                else
                {
                    BLUE_LOG_INFO(xx::g_logger) << "BGSAVE: ServerData already destroyed";
                } })
            .detach();
        return RespValue::simple_string("Background saving started");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLASTSAVE(std::vector<RespValue> &args,
                                                     MSocket::MSocketPtr sock,
                                                     bool aof,
                                                     std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        return RespValue::integer(self->getLastSaveTime().load(std::memory_order_acquire));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLASTSAVE1(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        std::time_t beijing_t = self->getLastSaveTime() + 8 * 3600;
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
    AutoRespValue CommandHandlerTable<T>::handleCOMMAND(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
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
            "PERSIST", "QUIT", "SCAN",
            // 事务模式
            "MULTI", "EXEC", "DISCARD", "WATCH", "UNWATCH",
            // 订阅模式
            "SUBSCRIBE", "PUBLISH", "UNSUBSCRIBE",
            // 慢查询
            "SLOWLOG",
            // 监控模式
            "MONITOR",
            // 主从复制
            "REPLICAOF", "SLAVEOF", "SYNC"};

        for (const auto &name : cmd_list)
        {
            commands.push_back(*RespValue::bulk_string(name));
        }

        return RespValue::array(std::move(commands));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleECHO(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        return RespValue::bulk_string(args[1].str);
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleTIME(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        auto now = SteadyClock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
        std::vector<RespValue> results;
        results.push_back(*RespValue::bulk_string(std::to_string(seconds)));
        results.push_back(*RespValue::bulk_string(std::to_string(microseconds)));
        return RespValue::array(std::move(results));
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleLOCALTIME(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
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
    AutoRespValue CommandHandlerTable<T>::handleSHUTDOWN(std::vector<RespValue> &args,
                                                     MSocket::MSocketPtr sock,
                                                     bool aof,
                                                     std::shared_ptr<ServerData<int>> self)
    {
        if (!self->isAdmin(sock))
        {
            return RespValue::error("ERR permission denied");
        }
        self->setShutdown(true);
        return RespValue::bulk_string("OK - waiting for clients to disconnect");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleQUIT(std::vector<RespValue> &args,
                                                     MSocket::MSocketPtr sock,
                                                     bool aof,
                                                     std::shared_ptr<ServerData<int>> self)
    {
        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleWATCH(std::vector<RespValue> &args,
                                                  MSocket::MSocketPtr sock,
                                                  bool aof,
                                                  std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        sock->clearWatchedKey();

        for (size_t i = 1; i < args.size(); i++)
        {
            const std::string &key = args[i].str;
            sock->addWatchKey(key, self->getKeyVersion(key, sock));
        }
        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleUNWATCH(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
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
    AutoRespValue CommandHandlerTable<T>::handleSLOWLOG(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
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
            self->getSlowLog().syncSlowLogs();

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

            std::vector<RespValue> results = self->getSlowLog().getSlowLogs(count);
            return RespValue::array(std::move(results));
        }

        else if (sub_cmd == "LEN")
        {
            self->getSlowLog().syncSlowLogs();
            return RespValue::integer(self->getSlowLog().len());
        }
        else if (sub_cmd == "RESET")
        {
            self->getSlowLog().reset();
            return RespValue::simple_string("OK");
        }
        else
        {
            return RespValue::error("ERR unknown SLOWLOG subcommand");
        }
    }

    // ========== 监控 ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleMONITOR(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }

        // 添加monitor client
        self->getMonitor().addMonitorClient(sock);

        sock->setMonitorMode(true);

        return RespValue::simple_string("OK");
    }

    // ========== AOF ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleAOFROTATE(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (!self->getAOF().getConfig_AOFEnabled())
        {
            return RespValue::error("ERR AOF is disabled");
        }
        if (self->getAOF().getAOFRotating())
        {
            return RespValue::error("ERR AOF rotation already in progress");
        }
        std::weak_ptr<ServerData<int>> weak_self = self;
        std::thread([weak_self]
                    {
                auto ptr = weak_self.lock();
                if (ptr)
                {
                    ptr->getAOF().rotateAOF();
                    BLUE_LOG_INFO(xx::g_logger) << "AOF rotation finished"; 
                } })
            .detach();
        return RespValue::simple_string("AOF rotation started");
    }

    // ========== Replication ==========
    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleREPLICAOF(std::vector<RespValue> &args,
                                                      MSocket::MSocketPtr sock,
                                                      bool aof,
                                                      std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() != 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'REPLICAOF'");
        }
        const std::string &host = args[1].str;
        const std::string &port_str = args[2].str;

        // REPLICAOF NO ONE 取消复制
        if (host == "NO" && port_str == "ONE")
        {
            if (!self->getReplication().getisMaster())
            {
                self->getReplication().stopReplication();
                self->getReplication().setisMaster(true);
                BLUE_LOG_INFO(xx::g_logger) << "Replication stopped, now master";
            }
            return RespValue::simple_string("OK");
        }

        int32_t port;
        try
        {
            port = std::stoi(port_str);
            if (port < 0 || port > UINT16_MAX)
            {
                return RespValue::error("ERR value is invalid");
            }
        }
        catch (...)
        {
            return RespValue::error("ERR value is not an integer or out of range");
        }

        // 如果已经是从节点且连接到同一个主节点，忽略
        if (!self->getReplication().getisMaster() &&
            self->getReplication().getMasterHost() == host &&
            self->getReplication().getMasterPort() == port)
        {
            return RespValue::simple_string("OK");
        }

        // 停止旧的复制
        if (!self->getReplication().getisMaster())
        {
            self->getReplication().stopReplication();
        }

        // 设置新的配置
        self->getReplication().setisMaster(false);
        self->getReplication().setMasterHost(host);
        self->getReplication().setMasterPort(static_cast<uint16_t>(port));
        self->getReplication().setReplOffset(0);

        // 启动复制
        self->getReplication().startReplication();

        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSLAVEOF(std::vector<RespValue> &args,
                                                    MSocket::MSocketPtr sock,
                                                    bool aof,
                                                    std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        if (args.size() != 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'REPLICAOF'");
        }
        const std::string &host = args[1].str;
        const std::string &port_str = args[2].str;

        // REPLICAOF NO ONE 取消复制
        if (host == "NO" && port_str == "ONE")
        {
            if (!self->getReplication().getisMaster())
            {
                self->getReplication().stopReplication();
                self->getReplication().setisMaster(true);
                BLUE_LOG_INFO(xx::g_logger) << "Replication stopped, now master";
            }
            return RespValue::simple_string("OK");
        }

        int32_t port;
        try
        {
            port = std::stoi(port_str);
            if (port < 0 || port > UINT16_MAX)
            {
                return RespValue::error("ERR value is invalid");
            }
        }
        catch (...)
        {
            return RespValue::error("ERR value is not an integer or out of range");
        }

        // 如果已经是从节点且连接到同一个主节点，忽略
        if (!self->getReplication().getisMaster() &&
            self->getReplication().getMasterHost() == host &&
            self->getReplication().getMasterPort() == port)
        {
            return RespValue::simple_string("OK");
        }

        // 停止旧的复制
        if (!self->getReplication().getisMaster())
        {
            self->getReplication().stopReplication();
        }

        // 设置新的配置
        self->getReplication().setisMaster(false);
        self->getReplication().setMasterHost(host);
        self->getReplication().setMasterPort(static_cast<uint16_t>(port));
        self->getReplication().setReplOffset(0);

        // 启动复制
        self->getReplication().startReplication();

        return RespValue::simple_string("OK");
    }

    template <typename T>
    AutoRespValue CommandHandlerTable<T>::handleSYNC(std::vector<RespValue> &args,
                                                 MSocket::MSocketPtr sock,
                                                 bool aof,
                                                 std::shared_ptr<ServerData<int>> self)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }

        // 只有主节点接收SYNC命令
        if (!(self->getReplication().getisMaster()))
        {
            return RespValue::error("ERR not master");
        }

        BLUE_LOG_INFO(xx::g_logger) << "SYNC request from slave, fd=" << sock->getSocketfd();

        // 主节点保存从节点连接
        self->getReplication().addSlaves(sock);

        // 生成 RDB 数据并发送
        std::string rdb_data = self->generateRDB(); // 拷贝一份

        // 发送 RDB 格式: $<length>\r\n<data>
        std::string response = "$" + std::to_string(rdb_data.size()) + "\r\n" + rdb_data;

        // 非阻塞同步发送(在tcpServer中的startAccept中对sock fd设置了非阻塞)
        ssize_t sent = ::send(sock->getSocketfd(), response.data(), response.size(), MSG_NOSIGNAL);
        if (sent <= 0)
        {
            BLUE_LOG_ERROR(xx::g_logger) << "Failed to send RDB to slave";

            // 从从节点列表中删除从节点
            self->getReplication().remove(sock);

            // 返回失败给从节点
            return RespValue::error("ERR failed to send RDB");
        }

        BLUE_LOG_INFO(xx::g_logger) << "SYNC: RDB sent to slave, " << rdb_data.size() << " bytes";
        return RespValue::simple_string("OK");
    }
}
#else
#endif