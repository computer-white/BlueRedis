/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <random>
#include <memory>
#include <cstring>
#include "blue/bytearray.h"
#include "blue/log.h"
#include "blue/macro.h"

namespace
{
    static blue::Logger::LoggerPtr g_Logger = BLUE_LOG_MASSAGE_ROOT();
    
    // 生成随机数据
    template <typename T, typename Generator>
    std::vector<T> makeData(size_t n, Generator gen)
    {
        std::vector<T> data;
        data.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            data.push_back(gen());
        }
        return data;
    }

    // 固定种子随机数，保证可复现
    std::mt19937 &rng()
    {
        // 2026.9.21
        static std::mt19937 engine(20260925);
        return engine;
    }
}

class TestReadBuffer : public ::testing::Test
{
protected:
    void SetUp() override
    {
        original.clear();
        res.clear();
        total_read = 0;
    }

    void TearDown() override
    {
        ba.reset();
        original.clear();
        res.clear();
        total_read = 0;
    }

    void writeDataUnit8(size_t writesize)
    {
        auto data = makeData<uint8_t>(writesize, [] { return static_cast<uint8_t>(rng()()); });
        for (auto x : data)
        {
            ba->writeFuint8(x);
        }
        original = std::string(data.begin(), data.end());
    }

    uint64_t readFromBuffer(size_t readsize)
    {
        ba->setPosition(0);
        std::vector<iovec> data;
        auto n = ba->getReadBuffers(data, readsize);
        for (auto &iov : data)
        {
            res.append((char*)iov.iov_base, iov.iov_len);
            total_read += iov.iov_len;
        }
        return n;
    }

    blue::ByteArray::ByteArrayPtr ba;
    std::string original;
    std::string res;
    uint64_t total_read = 0;
};

TEST_F(TestReadBuffer, SmallBase)
{
    ba = std::make_shared<blue::ByteArray>(4);
    writeDataUnit8(4);
    auto n = readFromBuffer(10);
    EXPECT_EQ(n, 4);
    EXPECT_EQ(total_read, n);
    EXPECT_EQ(res, original.substr(0, n));
}

TEST_F(TestReadBuffer, ManyBlock)
{
    ba = std::make_shared<blue::ByteArray>(4);
    writeDataUnit8(18); // 5个分区
    auto n = readFromBuffer(18);
    EXPECT_EQ(n, 18);
    EXPECT_EQ(total_read, n);
    EXPECT_EQ(res, original.substr(0, n));
}

TEST_F(TestReadBuffer, ManyBlockAndReadlessWrite)
{
    ba = std::make_shared<blue::ByteArray>(4);
    writeDataUnit8(18); // 5个分区
    auto n = readFromBuffer(10);
    EXPECT_EQ(n, 10);
    EXPECT_EQ(total_read, n);
    EXPECT_EQ(res, original.substr(0, n));
}

TEST_F(TestReadBuffer, BigDataWrite)
{
    ba = std::make_shared<blue::ByteArray>(1);
    writeDataUnit8(1000);
    auto n = readFromBuffer(10000);
    EXPECT_EQ(n, 1000);
    EXPECT_EQ(total_read, n);
    EXPECT_EQ(res, original.substr(0, n));
}

TEST_F(TestReadBuffer, FromMidRead)
{
    ba = std::make_shared<blue::ByteArray>(4);
    writeDataUnit8(400);    // 100个分区
    ba->setPosition(200);   // 中间开始读
    std::vector<iovec> data;
    auto n = ba->getReadBuffers(data, 1000);
    for (auto &iov : data)
    {
        res.append((char*)(iov.iov_base), iov.iov_len);
        total_read += iov.iov_len;
    }
    EXPECT_EQ(n, 200);
    EXPECT_EQ(total_read, n);
    EXPECT_EQ(res, original.substr(200, n));
}

TEST_F(TestReadBuffer, EmptyArray)
{
    ba = std::make_shared<blue::ByteArray>(4);
    auto n = readFromBuffer(1000);
    EXPECT_EQ(n, 0);
    EXPECT_EQ(total_read, n);
    EXPECT_EQ(res, original.substr(0, n));
    EXPECT_TRUE(res.empty());
    EXPECT_TRUE(original.empty());
}

// writeBuffers

class TestWriteBuffer : public ::testing::Test
{
protected:
    void SetUp() override
    {
        total_cap = 0;
        origin.clear();
        res.clear();
    }

    void TearDown() override
    {
        ba.reset();
        total_cap = 0;
        origin.clear();
        res.clear();
    }

    void writeData(size_t writesize)
    {
        auto data = makeData<uint8_t>(writesize, [] { return static_cast<uint8_t>(rng()()); });
        for (auto x : data)
        {
            ba->writeFint8(x);
            origin += x;
        }
    }

    std::pair<uint64_t, std::vector<iovec>> WriteFromBuffer(size_t reqsize)
    {
        std::vector<iovec> data;
        auto n = ba->getWriteBuffers(data, reqsize);
        for (auto &iov : data)
        {
            total_cap += iov.iov_len;
        }
        return std::make_pair(total_cap, data);
    }

    void WriteToVec(std::vector<iovec> &data)
    {
        for (auto &iov : data)
        {
            auto ch = static_cast<uint8_t>(rng()());
            memset(iov.iov_base, ch, iov.iov_len);
            origin.append((char*)(iov.iov_base), iov.iov_len);
        }
        ba->setSize(origin.size());
    }

    uint64_t ReadFromBuffer(size_t readsize)
    {
        ba->setPosition(0);
        std::vector<iovec> data;
        auto readn = ba->getReadBuffers(data, readsize);
        for (auto &iov : data)
        {
            res.append((char*)(iov.iov_base), iov.iov_len);
        }
        return readn;
    }

    blue::ByteArray::ByteArrayPtr ba;
    uint64_t total_cap; // 用于验证还可以写入的大小
    std::string origin;
    std::string res;
};

TEST_F(TestWriteBuffer, OnlyOneBlock)
{
    ba = std::make_shared<blue::ByteArray>(8);
    auto [n, buffer] = WriteFromBuffer(8);
    EXPECT_EQ(n, 8);
    EXPECT_EQ(n, total_cap);
    WriteToVec(buffer);
    auto read_n = ReadFromBuffer(8);
    EXPECT_EQ(read_n, 8);
    EXPECT_EQ(res, origin);
}

TEST_F(TestWriteBuffer, ManyBlock)
{
    ba = std::make_shared<blue::ByteArray>(4);
    auto [n, buffer] = WriteFromBuffer(20); // 会扩容
    EXPECT_GE(ba->getCapacity(), 20);
    EXPECT_EQ(n, 20);
    EXPECT_EQ(n, total_cap);
    WriteToVec(buffer);
    auto read_n = ReadFromBuffer(40);   // 实际不够40
    EXPECT_EQ(read_n, 20);
    EXPECT_EQ(res, origin);
}

TEST_F(TestWriteBuffer, ManyBlockAndReadLessWrite)
{
    ba = std::make_shared<blue::ByteArray>(4);
    auto [n, buffer] = WriteFromBuffer(30); // 会扩容
    EXPECT_GE(ba->getCapacity(), 30);
    EXPECT_EQ(n, 30);
    EXPECT_EQ(n, total_cap);
    WriteToVec(buffer);
    auto read_n = ReadFromBuffer(10);
    EXPECT_EQ(read_n, 10);
    EXPECT_EQ(res, origin.substr(0, read_n));
}

TEST_F(TestWriteBuffer, ManyBlockAndReadEqWrite)
{
    ba = std::make_shared<blue::ByteArray>(4);
    auto [n, buffer] = WriteFromBuffer(30); // 会扩容
    EXPECT_GE(ba->getCapacity(), 30);
    EXPECT_EQ(n, 30);
    EXPECT_EQ(n, total_cap);
    WriteToVec(buffer);
    auto read_n = ReadFromBuffer(30);
    EXPECT_EQ(read_n, 30);
    EXPECT_EQ(res, origin);
}

TEST_F(TestWriteBuffer, ManyBlockAndLargeData)
{
    ba = std::make_shared<blue::ByteArray>(4);
    auto [n, buffer] = WriteFromBuffer(1000); // 会扩容
    EXPECT_GE(ba->getCapacity(), 1000);
    EXPECT_EQ(n, 1000);
    EXPECT_EQ(n, total_cap);
    WriteToVec(buffer);
    auto read_n = ReadFromBuffer(10000);
    EXPECT_EQ(read_n, 1000);
    EXPECT_EQ(res, origin.substr(0, read_n));
}

TEST_F(TestWriteBuffer, ManyBlockAndHavedData)
{
    ba = std::make_shared<blue::ByteArray>(4);
    writeData(100);
    EXPECT_EQ(ba->getSize(), 100);
    EXPECT_EQ(ba->getCapacity(), 0);
    auto [n, buffer] = WriteFromBuffer(100); // 在索要100空间
    EXPECT_GE(ba->getCapacity(), 100);
    EXPECT_EQ(n, 100);
    EXPECT_EQ(n, total_cap);
    WriteToVec(buffer);
    auto read_n = ReadFromBuffer(10000);
    EXPECT_EQ(read_n, 200);
    EXPECT_EQ(res, origin.substr(0, read_n));
}

TEST_F(TestWriteBuffer, GetZeroData)
{
    ba = std::make_shared<blue::ByteArray>(4);
    writeData(1);
    EXPECT_EQ(ba->getSize(), 1);
    EXPECT_GE(ba->getCapacity(), 2);
    auto [n, buffer] = WriteFromBuffer(0);
    EXPECT_EQ(n, 0);
    EXPECT_EQ(n, total_cap);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(origin.empty());
}