#pragma once
#include "command_table.h"

namespace blue
{
    // 注册自定义检测的宏
    #define REGISTER_COMMAND(Name, Handler, IsWrite, ArgV)\
        static RespValue Handler(std::vector<RespValue>& args, \
                                MSocket::MSocketPtr sock, \
                                bool aof, \
                                CommandHandler<int>* self);

    // 构建命令表的宏（在类的静态方法中使用）
    #define BUILD_COMMAND_TABLE(...) \
        static consteval auto buildCommandTable() { \
            blue::CommandTableBuilder<256> builder; \
            __VA_ARGS__ \
            return builder.build(); \
        }

    // 插入自定义检测命令的宏
    #define CMD_ENTRY(Name, Handler, IsWrite, Argv) \
        builder.insert(#Name, \
                    reinterpret_cast<void*>(Handler), \
                    blue::fnv1a_hash(#Name), \
                    IsWrite, Argv);
}