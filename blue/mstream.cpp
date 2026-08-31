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
#include "mstream.h"
#include "blue/log.h"

// mstream
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    Task<ssize_t> MStream::readFixSize(void* buf, size_t len)
    {
        size_t offset = 0,l = len;
        while (l > 0)
        {
            ssize_t ret = co_await read((char*)(buf) + offset,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            offset += ret;
            l -= ret;
        }
        co_return len;
    }

    Task<ssize_t> MStream::readFixSize(ByteArray::ByteArrayPtr data, size_t len)
    {
        size_t l = len;
        while (l > 0)
        {
            ssize_t ret = co_await read(data,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            l -= ret;
        }
        co_return len;
    }

    Task<ssize_t> MStream::writeFixSize(const void* buf, size_t len)
    {
        size_t offset = 0,l = len;
        while (l > 0)
        {
            ssize_t ret = co_await write((const char*)(buf) + offset,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            offset += ret;
            l -= ret;
        }
        co_return len;

    }

    Task<ssize_t> MStream::writeFixSize(ByteArray::ByteArrayPtr data, size_t len)
    {
        size_t l = len;
        while (l > 0)
        {
            ssize_t ret = co_await write(data,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            l -= ret;
        }
        co_return len;
    }
}
