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
            size_t multiplier;  // 微妙倍数
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

            // 时间解析, us,ms,s,m,h

            /**
             * @brief 字符串 -> us
             */
            static std::optional<std::chrono::microseconds> ParseTime(const std::string &val);

            /**
             * @brief us -> 字符串
             */
            static std::string FormatTime(std::chrono::microseconds us, int precision = 2);

            // 布尔解析, yes/no, true/false, on/off, 1/0
            
            /**
             * @brief 字符串 -> bool
             */
            static bool ParseBool(const std::string &val);

        };
    }
}
