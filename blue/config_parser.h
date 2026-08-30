/**
 * @file config_parser.h
 * @brief 对于从配置读取的内容做解析(10M/MB -> 1024 * 1024 * 10 -> 10'485'760)
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.8.30
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include <vector>

namespace blue
{
    namespace util
    {
        struct SizeUnit
        {
            std::string suffix;
            size_t multiplier;
        };

        struct TimeUnit
        {
            std::string suffix;
            size_t multiplier;  // 毫秒倍数
        };

        class ConfigParser
        {
        public:
            // 大小解析, 支持k,M,G,T

            /**
             * @brief 字符串 -> size_t
             */
            static std::optional<size_t> ParseSize(const std::string &val);

            /**
             * @brief size_t -> 字符串
             */
            static std::string FormatSize(size_t val, int precision = 2);

            // 时间解析, ms,s,m,h

            /**
             * @brief 字符串 -> ms
             */
            static std::optional<std::chrono::milliseconds> ParseTime(const std::string &val);

            /**
             * @brief ms -> 字符串
             */
            static std::string FormatTime(std::chrono::microseconds ms);

            // 布尔解析, yes/no, true/false, on/off, 1/0
            
            /**
             * @brief 字符串 -> bool
             */
            static bool ParseBool(const std::string &val);

        };
    }
}
