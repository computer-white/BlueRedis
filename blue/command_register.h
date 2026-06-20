#pragma once
#include "command_table.h"

namespace blue
{
    template <typename T>
    using CommandHandler = RespValue(*)(
        std::vector<RespValue>&,
        MSocket::MSocketPtr,
        bool,
        T*
    );

    // 注册命令的宏
    #define REGISTER_COMMAND(Name, Handler, IsWrite, MinArgs, MaxArgs) \
        static RespValue Handler(std::vector<RespValue>& args, \
                                MSocket::MSocketPtr sock, \
                                bool aof, \
                                CommandHandler<int>* self); \
        consteval auto reg_##Handler() {   \
            return blue::CommandInfo(#Name,IsWrite, MinArgs, MaxArgs); \
        }

    // 构建命令表的宏（在类的静态方法中使用）
    #define BUILD_COMMAND_TABLE(...) \
        static consteval auto buildCommandTable() { \
            blue::CommandTableBuilder<256> builder; \
            __VA_ARGS__ \
            return builder.build(); \
        }

    // 插入命令的宏
    #define CMD_ENTRY(Name, Handler, IsWrite, MinArgs, MaxArgs) \
        builder.insert(blue::fnv1a_hash(#Name), #Name, \
                    reinterpret_cast<void*>(Handler), \
                    IsWrite, MinArgs, MaxArgs);
}