#ifndef BLUE_ENDIAN_H
#define BLUE_ENDIAN_H

#define BLUE_LITTLE_ENDIAN 1
#define BLUE_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

// 自定义主机字节与自定义字节转化
namespace blue
{
#if __cplusplus >= 202002L
    template <typename T>
    T bitswap(T val)
    {
        if constexpr (sizeof(T) == 8)
            return (T)__bswap_64((uint64_t)val);
        else if constexpr (sizeof(T) == 4)
            return (T)__bswap_32((uint32_t)val);
        else if constexpr (sizeof(T) == 2)
            return (T)__bswap_16((uint16_t)val);
        else
            static_assert(sizeof(T) == 0, "bitswap only supports 2/4/8 byte types");
    }
#else
    template <typename T>
    std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint64_t), T>
    bitswap(T val)
    {
        return (T)__bswap_64((uint64_t)val);
    }

    template <typename T>
    std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint32_t), T>
    bitswap(T val)
    {
        return (T)__bswap_32((uint32_t)val);
    }

    template <typename T>
    std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint16_t), T>
    bitswap(T val)
    {
        return (T)__bswap_16((uint16_t)val);
    }
#endif

// 检测系统字节序
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BLUE_BYTE_ORDER BLUE_BIG_ENDIAN
#else
#define BLUE_BYTE_ORDER BLUE_LITTLE_ENDIAN
#endif

    // 根据系统字节序提供转换函数
#if BLUE_BYTE_ORDER == BLUE_BIG_ENDIAN
    // 网络转主机
    template <typename T>
    T bitswapnToh(T val)
    {
        return val;
    }

    // 主机转网络
    template <typename T>
    T bitswaphToh(T val)
    {
        return val;
    }
#else
    // 网络转主机
    template <typename T>
    T bitswapnToh(T val)
    {
        return bitswap(val);
    }

    // 主机转网络
    template <typename T>
    T bitswaphTon(T val)
    {
        return bitswap(val);
    }
#endif

} // namespace blue

#endif