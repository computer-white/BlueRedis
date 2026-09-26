/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <iostream>
#include <cassert>
#include "blue/resp_parser.h"

using namespace blue;

TEST(TestRespParser, SimpleString)
{
    std::string data = "+OK\r\n";
    auto [val, consumed] = RespValue::parse(data);
    EXPECT_EQ(val.type, RespValue::Type::SIMPLE_STRING);
    EXPECT_EQ(val.str, "OK");
    EXPECT_EQ(consumed, data.size());
}

TEST(TestRespParser, Error)
{
    std::string data = "-ERR unknown command\r\n";
    auto [val, comsumed] = RespValue::parse(data);
    EXPECT_EQ(val.type, RespValue::Type::ERROR);
    EXPECT_EQ(val.str, "ERR unknown command");
    EXPECT_EQ(comsumed, data.size());
}

TEST(TestRespParser, Integer)
{
    std::string data = ":1000\r\n";
    auto [val, consumed] = RespValue::parse(data);
    EXPECT_EQ(val.type, RespValue::Type::INTEGER);
    EXPECT_EQ(val.integ, 1000);
    EXPECT_EQ(consumed, data.size());
}

TEST(TestRespParser, BlukString)
{
    std::string data = "$5\r\nhello\r\n";
    auto [val, consumed] = RespValue::parse(data);
    EXPECT_EQ(val.type, RespValue::Type::BULK_STRING);
    EXPECT_EQ(val.str, "hello");
    EXPECT_EQ(consumed, data.size());
}

TEST(TestRespParser, NullBluk)
{
    std::string data = "$-1\r\n";
    auto [val, consumed] = RespValue::parse(data);
    EXPECT_EQ(val.type, RespValue::Type::NULL_VAL);
    EXPECT_EQ(consumed, data.size());
}

TEST(TestRespParser, Array)
{
    std::string data = "*2\r\n$3\r\nGET\r\n$4\r\nkey1\r\n";
    auto [val, consumed] = RespValue::parse(data);
    EXPECT_EQ(val.type, RespValue::Type::ARRAY);
    EXPECT_EQ(val.arr.size(), 2);
    EXPECT_EQ(val.arr[0].type, RespValue::Type::BULK_STRING);
    EXPECT_EQ(val.arr[0].str, "GET");
    EXPECT_EQ(val.arr[1].type, RespValue::Type::BULK_STRING);
    EXPECT_EQ(val.arr[1].str, "key1");
    EXPECT_EQ(consumed, data.size());
}

TEST(TestRespParser, IncompleteData)
{
    RespStreamParser parser;
    
    // 发送不完整的数据
    parser.feed("*2\r\n$3\r\nGET\r\n$4\r\nke");
    RespValue cmd;
    EXPECT_FALSE(parser.next(cmd));

    // 发送剩余数据
    parser.feed("y1\r\n");
    EXPECT_TRUE(parser.next(cmd));
    EXPECT_EQ(cmd.arr[1].str, "key1");
}

TEST(TestRespParser, MultipleCommands)
{
    RespStreamParser parser;

    // 一次发送多个命令
    parser.feed("*1\r\n$4\r\nPING\r\n*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n");

    RespValue cmd;
    EXPECT_TRUE(parser.next(cmd));
    EXPECT_EQ(cmd.type, RespValue::Type::ARRAY);
    EXPECT_EQ(cmd.arr[0].str, "PING");

    EXPECT_TRUE(parser.next(cmd));

    EXPECT_EQ(cmd.type, RespValue::Type::ARRAY);
    EXPECT_EQ(cmd.arr[0].str, "GET");
    EXPECT_EQ(cmd.arr[1].str, "key");
}

TEST(TestRespParser, BufferOverflowProtection)
{
    RespStreamParser parser(100);  // 小缓冲区用于测试
    
    std::string large_data(200, 'a');
    EXPECT_FALSE(parser.feed(large_data));
}

TEST(TestRespParser, Encoding)
{
    // 测试编码和解码的一致性
    AutoRespValue original = RespValue::array({
        *RespValue::bulk_string("SET"),
        *RespValue::bulk_string("key"),
        *RespValue::bulk_string("value")
    });
    
    std::string encoded = RespValue::encode(*original);
    auto [decoded, consumed] = RespValue::parse(encoded);

    EXPECT_EQ(decoded.type, RespValue::Type::ARRAY);
    EXPECT_EQ(decoded.arr.size(), 3);
    EXPECT_EQ(decoded.arr[0].str, "SET");
    EXPECT_EQ(decoded.arr[1].str, "key");
    EXPECT_EQ(decoded.arr[2].str, "value");
}