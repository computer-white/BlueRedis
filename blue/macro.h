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
 * @file macro.h
 * @brief 自定义宏模块
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.4.15
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef __BLUE_MACRO_H__
#define __BLUE_MACRO_H__
// 自定义宏模块
#include <string>
#include <assert.h>
#include "util.h"
#include "log.h"

#if defined (__GNUC__ ) && __GNUC__ >= 4  || defined (__llvm__)
// 告诉编译器优化,条件大概成立
#   define BLUE_LIKELY(x)            __builtin_expect(!!(x), 1)
// 告诉编译器优化，条件大概率不成立
#   define BLUE_UNLIKELY(x)          __builtin_expect(!!(x), 0)
#else
#   define BLUE_LIKELY(x)            (x)
#   define BLUE_UNLIKELY(x)          (x)
#endif

// x : 条件
#define BLUE_ASSERT(x)                                                                      \
    if (BLUE_UNLIKELY(!(x)))                                                                               \
    {                                                                                       \
        BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ASSERTION : " #x                        \
                                                << "\nbacktrace\n"                          \
                                                << blue::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                          \
    }
// x : 条件 w : 额外打印的信息
#define BLUE_ASSERT2(x, w)                                                                  \
    if (BLUE_UNLIKELY(!(x)))                                                                               \
    {                                                                                       \
        BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ASSERTION : " #x                        \
                                                << "\n"                                     \
                                                << w                                        \
                                                << "\nbacktrace\n"                          \
                                                << blue::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                          \
    }
#endif // __BLUE_MACRO_H__