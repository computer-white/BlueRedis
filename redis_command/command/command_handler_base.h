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
        virtual RespValue executeIfelse(std::vector<RespValue> args, MSocket::MSocketPtr sock, ServerData<T> &self, 
            bool RecordAOF = true, CommandHandler<int> *comm = nullptr) = 0;

        /**
         * @brief 命令表处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeTable(std::vector<RespValue> args, MSocket::MSocketPtr sock, ServerData<T> &self, 
            bool RecordAOF = true, CommandHandler<int> *comm = nullptr) = 0;
    };
}