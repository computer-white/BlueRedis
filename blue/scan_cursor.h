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
 * @file scan_cursor.h
 * @brief SCAN 命令游标定义
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.7.15
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <string>

namespace blue
{

    // SCAN 游标状态
    struct ScanCursor
    {
        int db = 0;             // 数据库编号
        int shardIndex = 0;     // 当前分片索引 (0-127)
        int dataType = 0;       // 0:string, 1:hash, 2:list, 3:set, 4:zset
        size_t offset = 0;      // 在当前数据结构中的偏移量
        bool completed = false; // 是否已完成

        // 序列化为字符串（用于返回给客户端的游标值）
        std::string serialize() const
        {
            if (completed)
            {
                return "0";
            }
            // 格式: shardIndex:dataType:offset
            return std::to_string(shardIndex) + ":" +
                   std::to_string(dataType) + ":" +
                   std::to_string(offset);
        }

        // 从字符串反序列化
        static ScanCursor deserialize(const std::string &str)
        {
            ScanCursor cursor;
            if (str == "0" || str.empty() || str.find(':') == std::string::npos)
            {
                cursor.completed = true;
                return cursor;
            }

            size_t pos1 = str.find(':');
            size_t pos2 = str.find(':', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos)
            {
                try
                {
                    cursor.shardIndex = std::stoi(str.substr(0, pos1));
                    cursor.dataType = std::stoi(str.substr(pos1 + 1, pos2 - pos1 - 1));
                    cursor.offset = std::stoull(str.substr(pos2 + 1));
                    cursor.completed = false;
                }
                catch (...)
                {
                    cursor.completed = true;
                }
            }
            else
            {
                cursor.completed = true;
            }
            return cursor;
        }
    };

} // namespace blue