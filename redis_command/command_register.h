#pragma once
#ifdef COMMAND_TABLE
#include "command_table.h"

namespace blue
{
    // 注册自定义检测的宏
    #define REGISTER_COMMAND_T(Name, Handler) \
        static RespValue Handler(std::vector<RespValue>& args, \
                                MSocket::MSocketPtr sock, \
                                bool aof, \
                                CommandHandler<int>* self);
    
    #define REGISTER_COMMAND_T(Name, Handler) \
        static RespValue Handler(std::vector<RespValue>& args, \
                                MSocket::MSocketPtr sock, \
                                bool aof, \
                                ServerData<int>& self, \
                                CommandHandler<int> *comm);
    // 插入自定义检测命令的宏
    #define CMD_ENTRY(Name, Handler, IsWrite, Argv) \
        builder.insert(#Name, \
                    Handler, \
                    blue::fnv1a_hash(#Name), \
                    IsWrite, Argv);
    // 插入自定义检测命令的宏
    #define CMD_ENTRY_T(Name, Handler, IsWrite, Argv) \
        builder.insert(#Name, \
                    Handler, \
                    blue::fnv1a_hash(#Name), \
                    IsWrite, Argv);
}
#else
#endif