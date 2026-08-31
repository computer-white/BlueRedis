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
 * @file command_register.h
 * @brief 宏定义redis命令处理函数和插入命令表
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.17
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
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