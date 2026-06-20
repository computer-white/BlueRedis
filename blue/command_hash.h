#pragma once
#include <cstdint>
#include <cstddef>


namespace blue
{
    // constexpr 字符串长度
    constexpr size_t constexpr_strlen(const char* str)
    {
        size_t len = 0;
        while (str[len] != '\0') len++;
        return len;
    }

    // FNV-1a 哈希（编译期计算）
    constexpr uint32_t fnva1_hash(const char* str, size_t len)
    {
        uint32_t hash = 0x811c9dc5u;
        for (size_t i = 0; i < len; ++i) 
        {
            hash ^= static_cast<uint8_t>(str[i]);
            hash *= 0x01000193u;
        }
        return hash;
    }

    // 对字符串字面量的重载
    constexpr uint32_t fnv1a_hash(const char* str) 
    {
        return fnva1_hash(str, constexpr_strlen(str));
    }

    // 编译期字符串包装
    template <size_t N>
    struct ConstString
    {
        char data[N];

        consteval ConstString(const char (&str)[N]) 
        {
            for (size_t i = 0; i < N; ++i)
            {
                data[i] = str[i];
            }
        }
        
        constexpr uint32_t hash() const 
        {
            return fnv1a_hash(data, N - 1);
        }
        
        constexpr size_t size() const { return N - 1; }
    };



}
