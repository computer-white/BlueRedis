#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "msocket.h"
#include "resp_parser.h"
#include "command_hash.h"

namespace blue
{
    // 编译期命令信息
    template <size_t N>
    struct CommandInfo
    {
        char name[N];
        uint32_t hash;
        bool is_write;
        uint8_t min_args;
        uint8_t max_args;   // 255 表示无限制

        // 编译器构造
        consteval CommandInfo(const char (&str)[N], bool write, uint8_t min, uint8_t max)
        : is_write(write), min_args(min), max_args(max)
        {
            for (size_t i = 0; i < N; i++)
            {
                name[i] = str[i];
            }
            hash = fnva1_hash(name, N - 1);
        }

        consteval CommandInfo(const ConstString<N>& str, bool write, uint8_t min, uint8_t max)
        : CommandInfo(str.data,write, min, max) {}
    };

}