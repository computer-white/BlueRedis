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
 * @file mstream.h
 * @brief 读写操作的流式处理基类
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.5.21
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_MSTREAM_H
#define BLUE_MSTREAM_H
#include <memory>
#include "bytearray.h"
#include "task.h"
// mstream
namespace blue
{
    /* 流式处理的基类 */
    class MStream
    {
    public:
        using MStreamPtr = std::shared_ptr<MStream>;
        /**
         * @brief 流式处理默认析构函数
         */
        virtual ~MStream() {}

        /**
         * @brief 读取len长度内容到buf中
         * @param buf 存放读取内容的地址
         * @param len 想要读取的长度
         * @return 返回实际读取的大小
         * @note 对socket上recv的抽象(出错返回实际读取)
         */
        virtual Task<ssize_t> read(void *buf, size_t len) = 0;

        /**
         * @brief 读取len长度内容到字节链表(byteArray)
         * @param data 字节链表指针
         * @param len 想要读取的长度
         * @return 返回实际读取的大小
         * @note 对socket上recv的抽象(出错返回实际读取)
         */
        virtual Task<ssize_t> read(ByteArray::ByteArrayPtr data, size_t len) = 0;

        /**
         * @brief 循环读取len长度内容到buf中,一次性读完
         * @param buf 存放读取内容的地址
         * @param len 想要读取的长度
         * @return 返回实际读取的大小(出错返回实际读取)
         */
        virtual Task<ssize_t> readFixSize(void *buf, size_t len);

        /**
         * @brief 循环读取len长度内容到data中,一次性读完
         * @param data 字节链表指针
         * @param len 想要读取的长度
         * @return 返回实际读取的大小(出错返回实际读取)
         */
        virtual Task<ssize_t> readFixSize(ByteArray::ByteArrayPtr data, size_t len);

        /**
         * @brief 将buf中len长度内容发送给对方
         * @param buf 存放输出内容的地址
         * @param len 想要输出的长度
         * @return 返回实际写入的大小
         * @note 对socket上send的抽象(出错返回实际写入)
         */
        virtual Task<ssize_t> write(const void *buf, size_t len) = 0;

        /**
         * @brief 将data中len长度内容发送给对方
         * @param data 存放输出内容的地址
         * @param len 想要输出的长度
         * @return 返回实际写入的大小
         * @note 对socket上send的抽象(出错返回实际写入)
         */
        virtual Task<ssize_t> write(ByteArray::ByteArrayPtr data, size_t len) = 0;

        /**
         * @brief 循环将buf中len长度内容发送给对方,一次性写入len
         * @param buf 存放输出内容的地址
         * @param len 想要输出的长度
         * @return 返回实际写入的大小(出错返回实际写入)
         */
        virtual Task<ssize_t> writeFixSize(const void *buf, size_t len);

        /**
         * @brief 循环将data中len长度内容发送给对方,一次性写入len
         * @param data 存放输出内容的地址
         * @param len 想要输出的长度
         * @return 返回实际写入的大小(出错返回实际写入)
         */
        virtual Task<ssize_t> writeFixSize(ByteArray::ByteArrayPtr data, size_t len);

        /**
         * @brief 关闭流
         */
        virtual void close() = 0;
    };
}

#endif