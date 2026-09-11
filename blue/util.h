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
 * @file util.h
 * @brief 一系列辅助函数, 关于获取协程id api,已然没用默认返回0，因为我们采用了c++20协程
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.4.3
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_UTIL_H
#define BLUE_UTIL_H
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <vector>
#include <string>

// 辅助函数
namespace blue
{

    // 获取线程Id
    pid_t GetThreadId();

    // 调用堆栈信息 返回到bt size : 调用堆栈行数,skip : 选择跳过多少行
    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);

    // 显示调用堆栈信息 prefix : 支持加上string前缀使信息更明显,size : 调用堆栈行数,skip : 选择跳过多少行
    std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "");

    // 获取当前时间(ms)
    uint64_t GetCurrentMs();
    // 获取当前时间(us)
    uint64_t GetCurrentUs();
    // 获取当前时间(ms)
    uint64_t GetCurrentMsbyc();
    // 获取当前时间(us)
    uint64_t GetCurrentUsbyc();
    // 获取当前时间(ns)
    uint64_t GetCurrentNsbyc();
    // 获取当前北京时间
    std::string GetCurrentBeiJingTime();
}

#endif // __BLUE_UTIL_H__