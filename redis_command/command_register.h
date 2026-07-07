 /**
 * @file command_register.h
 * @brief 宏定义redis命令处理函数和插入命令表
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.17
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#ifdef COMMAND_TABLE
#include "command_table.h"

namespace blue
{
    // 注册自定义检测的宏
    #define REGISTER_COMMAND_T(Name, Handler) \
        static AutoRespValue Handler(std::vector<RespValue>& args, \
                                MSocket::MSocketPtr sock, \
                                bool aof, \
                                std::shared_ptr<ServerData<int>> self);
    // 插入自定义检测命令的宏
    #define CMD_ENTRY_T(Name, Handler, IsWrite, Argv) \
        builder.insert(#Name, \
                    Handler, \
                    blue::fnv1a_hash(#Name), \
                    IsWrite, Argv);
}
#else
#endif