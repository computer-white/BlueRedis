/**
 * @file command_handler_base.h
 * @brief redis server 响应客户端命令的基类
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.28
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
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
#include "redis_command/server_data.h"


namespace blue
{   
    template <typename T>
    class CommandHandler;

    template <typename T>
    class CommandHandlerBased
    {
    public:
        CommandHandlerBased() = default;
        ~CommandHandlerBased() = default;
        /**
         * @brief ifelse处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeIfelse(std::vector<RespValue> args, MSocket::MSocketPtr sock, std::shared_ptr<ServerData<T>> self, 
            bool RecordAOF = true) = 0;

        /**
         * @brief 命令表处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeTable(std::vector<RespValue> args, MSocket::MSocketPtr sock, std::shared_ptr<ServerData<T>> self, 
            bool RecordAOF = true) = 0;
    };
}