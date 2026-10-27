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
 * @file command_handler_table.h
 * @brief redis server 响应客户端命令的派生类，编译期构建命令表实现，每个命令单独的处理函数，相对好维护和管理
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.28
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <memory>
#include <chrono>
#include <list>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include "blue/config.h"
#include "blue/msocket.h"
#include "blue/skiplist.h"
#include "blue/resp_parser.h"
#include "blue/tcpServer.h"
#include "blue/configinit.h"
#include "redis_command/server_data.h"
#include "redis_command/command_table.h"
#include "redis_command/command_register.h"

namespace blue
{
    class CommandHandlerTable
    {
    public:
        using SteadyClock = std::chrono::steady_clock;
        using TimePoint = SteadyClock::time_point;

    public:
        /**
         * @brief 命令表处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        RespValue executeTable(std::vector<RespValue> args,
                               MSocket::MSocketPtr sock, std::shared_ptr<ServerData> self,
                               bool RecordAOF = true);

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

        static constexpr auto ONLY_MORE_ONE = [](size_t argc) -> bool
        { return argc >= 1; };
        static constexpr auto ONLY_MORE_TWO = [](size_t argc) -> bool
        { return argc >= 2; };
        static constexpr auto ONLY_MORE_THREE = [](size_t argc) -> bool
        { return argc >= 3; };

    public:
        using CommandHandlerFunc = blue::AutoRespValue (*)(std::vector<RespValue> &,
                                                           MSocket::MSocketPtr,
                                                           bool,
                                                           std::shared_ptr<ServerData>);
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
            CMD_ENTRY_T(INFO, handleINFO, false, ONLY_MORE_ONE);
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

//     inline RespValue CommandHandlerTable::executeTable(std::vector<RespValue> args, MSocket::MSocketPtr sock,
//                                                           std::shared_ptr<ServerData> self,
//                                                           bool RecordAOF)
//     {
//         RespValue result;
//         auto start = SteadyClock::now();
//         if (args.empty())
//         {
//             return *RespValue::error("ERR empty command");
//         }
//         std::string cmd = args[0].str;
//         std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
// #define HOT_COMMANDS(XX) \
//     XX(GET)              \
//     XX(SET)              \
//     XX(DEL)              \
//     XX(HSET)             \
//     XX(HGET)             \
//     XX(LPUSH)            \
//     XX(LPOP)             \
//     XX(SADD)             \
//     XX(ZADD)

// #define IF_CMD(name)                                                   \
//     if (cmd == #name)                                                  \
//     {                                                                  \
//         if (self->getAOF().isWriteCommand(cmd) && RecordAOF)           \
//         {                                                              \
//             std::string aof_cmds = self->getAOF().formatCommand(args); \
//             self->getAOF().appendToAOF(aof_cmds);                      \
//             if (s_repl_is_master.load(std::memory_order_acquire) &&    \
//                 !self->getReplication().slavesEmpty())                 \
//             {                                                          \
//                 self->getReplication().broadcastToSlaves(aof_cmds);    \
//             }                                                          \
//         }                                                              \
//         result = *handle##name(args, sock, RecordAOF, self);           \
//         /* 慢查询记录*/                                                \
//         auto end = SteadyClock::now();                                 \
//         std::string cmd_str;                                           \
//         for (size_t i = 0; i < args.size(); ++i)                       \
//         {                                                              \
//             if (i > 0)                                                 \
//             {                                                          \
//                 cmd_str += " ";                                        \
//             }                                                          \
//             cmd_str += args[i].str;                                    \
//         }                                                              \
//         self->getSlowLog().pushEntry(cmd_str, sock, start, end);       \
//         return result;                                                 \
//     }

//         // 高频命令不走命令表
//         HOT_COMMANDS(IF_CMD);

// #undef IF_CMD
// #undef HOT_COMMANDS

//         const auto &table = getCommandTable();
//         auto *entry = table.find_lowerbound(fnv1a_hash(cmd.c_str()));
//         if (!entry)
//         {
//             return *RespValue::error("ERR unknown command");
//         }

//         // 参数验证
//         if (entry->argV && !entry->argV(args.size()))
//         {
//             return *RespValue::error("ERR wrong number of arguments for '" + cmd + "'");
//         }

//         if (RecordAOF && entry->is_write)
//         {
//             std::string aof_cmds = self->getAOF().formatCommand(args);
//             self->getAOF().appendToAOF(aof_cmds);

//             // 如果是主节点，广播给从节点
//             if (s_repl_is_master.load(std::memory_order_acquire) && !(self->getReplication().slavesEmpty()))
//             {
//                 self->getReplication().broadcastToSlaves(aof_cmds);
//             }
//         }

//         // 执行命令
//         auto handler = entry->handler;
//         result = *handler(args, sock, RecordAOF, self);

//         // 慢查询记录
//         auto end = SteadyClock::now();
//         std::string cmd_str;
//         for (size_t i = 0; i < args.size(); ++i)
//         {
//             if (i > 0)
//             {
//                 cmd_str += " ";
//             }
//             cmd_str += args[i].str;
//         }
//         self->getSlowLog().pushEntry(cmd_str, sock, start, end);

//         return result;
//     }
}