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