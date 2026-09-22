/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <climits>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include "blue/bytearray.h"
#include "blue/log.h"
#include "blue/macro.h"

namespace
{
    static blue::Logger::LoggerPtr g_Logger = BLUE_LOG_MASSAGE_ROOT();

    // 生成一批随机数据
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
        static std::mt19937 engine(20260921);
        return engine;
    }

    // 写入 -> 重置位置 -> 读回 -> 逐个比对 -> 校验剩余可读为 0
    // WF: void (ByteArray::*)(T)
    // RF: T (ByteArray::*)()
    template <typename T, typename WF, typename RF>
    void roundTrip(const std::vector<T> &data, WF wf, RF rf, size_t baseLen = 1)
    {
        blue::ByteArray ba(baseLen);
        for (const auto &x : data)
        {
            (ba.*wf)(x);
        }

        const size_t written = ba.getSize();
        EXPECT_EQ(written, ba.getSize());

        ba.setPosition(0);

        for (size_t i = 0; i < data.size(); ++i)
        {
            T v = (ba.*rf)();
            EXPECT_EQ(v, data[i]) << "round-trip mismatch at index " << i;
        }

        EXPECT_EQ(ba.getReadSize(), 0u)
            << "still has unread bytes after reading all elements";
        EXPECT_EQ(ba.getPosition(), written);
    }

    // 只测写入后的字节数，不读回
    template <typename T, typename WF>
    size_t writeSize(const std::vector<T> &data, WF wf, size_t baseLen = 1)
    {
        blue::ByteArray ba(baseLen);
        for (const auto &x : data)
        {
            (ba.*wf)(x);
        }
        return ba.getSize();
    }

    // 浮点近似比较（普通随机数其实可以精确相等，这里留出容差更稳）
    template <typename T>
    void expectNear(T a, T b, size_t idx)
    {
        if constexpr (std::is_same_v<T, float>)
        {
            EXPECT_FLOAT_EQ(a, b) << "index: " << idx;
        }
        else
        {
            EXPECT_DOUBLE_EQ(a, b) << "index: " << idx;
        }
    }

    template <typename T, typename WF, typename RF>
    void roundTripFloat(const std::vector<T> &data, WF wf, RF rf, size_t baseLen = 1)
    {
        blue::ByteArray ba(baseLen);
        for (const auto &x : data)
        {
            (ba.*wf)(x);
        }
        const size_t written = ba.getSize();

        ba.setPosition(0);
        for (size_t i = 0; i < data.size(); ++i)
        {
            T v = (ba.*rf)();
            expectNear(v, data[i], i);
        }
        EXPECT_EQ(ba.getReadSize(), 0u);
        EXPECT_EQ(ba.getPosition(), written);
    }

    // 文件读写往返
    template <typename T, typename WF, typename RF>
    void fileRoundTrip(const std::vector<T> &data, WF wf, RF rf,
                       const std::string &tag, size_t baseLen = 1)
    {
        std::filesystem::create_directories("./tem");
        const std::string path = "./tem/" + tag + ".dat";

        blue::ByteArray ba(baseLen);
        for (const auto &x : data)
        {
            (ba.*wf)(x);
        }
        // 执行完write操作
        ba.setPosition(0);
        ASSERT_TRUE(ba.writeToFile(path)) << "writeToFile failed: " << path;

        blue::ByteArray ba2(baseLen * 2);
        ASSERT_TRUE(ba2.readFromFile(path)) << "readFromFile failed: " << path;
        // readFromFile中包含write操作
        ba2.setPosition(0);
        EXPECT_EQ(ba.toString(), ba2.toString())
            << "file content differs from in-memory content";

        // 确保写文件没有改变原对象位置
        EXPECT_EQ(ba.getPosition(), 0u);
        EXPECT_EQ(ba2.getPosition(), 0u);
    }
}

// 定长大小
template <typename T>
struct FixedIntCase
{
    using Write = void (blue::ByteArray::*)(T);
    using Read = T (blue::ByteArray::*)();
    Write write;
    Read read;
    const char *name;
};

TEST(ByteArrayFixedint, Int8)
{
    auto data = makeData<int8_t>(100, []
                                 { return static_cast<int8_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFint8, &blue::ByteArray::readFint8);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFint8), 100u);
}

TEST(ByteArrayFixedint, Uint8)
{
    auto data = makeData<uint8_t>(100, []
                                  { return static_cast<uint8_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFuint8, &blue::ByteArray::readFuint8);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFuint8), 100u);
}

TEST(ByteArrayFixedint, Int16)
{
    auto data = makeData<int16_t>(100, []
                                  { return static_cast<int16_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFint16, &blue::ByteArray::readFint16);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFint16), 100u * sizeof(int16_t));
}

TEST(ByteArrayFixedint, Uint16)
{
    auto data = makeData<uint16_t>(100, []
                                   { return static_cast<uint16_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFuint16, &blue::ByteArray::readFuint16);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFuint16), 100u * sizeof(uint16_t));
}

TEST(ByteArrayFixedint, Int32)
{
    auto data = makeData<int32_t>(100, []
                                  { return static_cast<int32_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFint32, &blue::ByteArray::readFint32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFint32), 100u * sizeof(int32_t));
}

TEST(ByteArrayFixedint, Uint32)
{
    auto data = makeData<uint32_t>(100, []
                                   { return static_cast<uint32_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFuint32, &blue::ByteArray::readFuint32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFuint32), 100u * sizeof(uint32_t));
}

TEST(ByteArrayFixedint, Int64)
{
    auto data = makeData<int64_t>(100, []
                                  { return static_cast<int64_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFint64, &blue::ByteArray::readFint64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFint64), 100u * sizeof(int64_t));
}

TEST(ByteArrayFixedint, Uint64)
{
    auto data = makeData<uint64_t>(100, []
                                   { return static_cast<uint64_t>(rng()()); });
    roundTrip(data, &blue::ByteArray::writeFuint64, &blue::ByteArray::readFuint64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFuint64), 100u * sizeof(uint64_t));
}

// Varint 无符号

TEST(ByteArrayVarint, Uint32SmallCompress)
{
    std::vector<uint32_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(static_cast<uint32_t>(i % 128));
    }
    roundTrip(data, &blue::ByteArray::writeUint32, &blue::ByteArray::readUint32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeUint32), 100u);
}

TEST(ByteArrayVarint, Uint32Medium)
{
    std::vector<uint32_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(static_cast<uint32_t>(128 + i % 1000));
    }
    roundTrip(data, &blue::ByteArray::writeUint32, &blue::ByteArray::readUint32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeUint32), 100u * 2);
}

TEST(ByteArrayVarint, Uint32LargeNoCompress)
{
    std::vector<uint32_t> data(100, UINT32_MAX);
    roundTrip(data, &blue::ByteArray::writeUint32, &blue::ByteArray::readUint32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeUint32), 100u * 5);
}

TEST(ByteArrayVarint, Uint32RandomRoundTrip)
{
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    auto data = makeData<uint32_t>(1000, [&]
                                   { return dist(rng()); });
    roundTrip(data, &blue::ByteArray::writeUint32, &blue::ByteArray::readUint32);
    EXPECT_LE(writeSize(data, &blue::ByteArray::writeUint32), 1000u * 5);
    EXPECT_GE(writeSize(data, &blue::ByteArray::writeUint32), 1000u);
}

TEST(ByteArrayVarint, Uint64SmallCompress)
{
    std::vector<uint64_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(static_cast<uint64_t>(i % 128));
    }
    roundTrip(data, &blue::ByteArray::writeUint64, &blue::ByteArray::readUint64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeUint64), 100u);
}

TEST(ByteArrayVarint, Uint64LargeNoCompress)
{
    std::vector<uint64_t> data(100, UINT64_MAX);
    roundTrip(data, &blue::ByteArray::writeUint64, &blue::ByteArray::readUint64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeUint64), 100u * 10);
}

TEST(ByteArrayVarint, Uint64RandomRoundTrip)
{
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    auto data = makeData<uint64_t>(1000, [&]
                                   { return dist(rng()); });
    roundTrip(data, &blue::ByteArray::writeUint64, &blue::ByteArray::readUint64);
    EXPECT_LE(writeSize(data, &blue::ByteArray::writeUint64), 1000u * 10);
    EXPECT_GE(writeSize(data, &blue::ByteArray::writeUint64), 1000u);
}

// Varint + ZigZag 有符号

TEST(ByteArrayVarint, Int32SmallPositive)
{
    std::vector<int32_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(i % 64);
    }
    roundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt32), 100u);
}

TEST(ByteArrayVarint, Int32SmallNegative)
{
    std::vector<int32_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(-(i % 64) - 1);
    }
    roundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt32), 100u);
}

TEST(ByteArrayVarint, Int32SmallCrossZero)
{
    std::vector<int32_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(i % 64 - 32);
    }
    roundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt32), 100u);
}

TEST(ByteArrayVarint, Int32LargeNoCompress)
{
    std::vector<int32_t> data(100, INT32_MAX);
    roundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt32), 100u * 5);
}

TEST(ByteArrayVarint, Int32LargeNegativeNoCompress)
{
    std::vector<int32_t> data(100, INT32_MIN);
    roundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt32), 100u * 5);
}

TEST(ByteArrayVarint, Int32Boundaries)
{
    const std::vector<int32_t> cases = {
        0,
        1,
        -1,
        63,
        -64,
        64,
        -65,
        127,
        -128,
        128,
        -129,
        (1 << 20),
        -(1 << 20),
        (1 << 27),
        -(1 << 27) - 1,
        INT32_MAX,
        INT32_MIN,
        INT32_MAX - 1,
        INT32_MIN + 1,
    };
    roundTrip(cases, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
}

TEST(ByteArrayVarint, Int32RandomRoundTrip)
{
    std::uniform_int_distribution<int32_t> dist(INT32_MIN, INT32_MAX);
    auto data = makeData<int32_t>(1000, [&]
                                  { return dist(rng()); });
    roundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32);
    EXPECT_LE(writeSize(data, &blue::ByteArray::writeInt32), 1000u * 5);
    EXPECT_GE(writeSize(data, &blue::ByteArray::writeInt32), 1000u);
}

TEST(ByteArrayVarint, Int64SmallPositive)
{
    std::vector<int64_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(static_cast<int64_t>(i % 64));
    }
    roundTrip(data, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt64), 100u);
}

TEST(ByteArrayVarint, Int64SmallNegative)
{
    std::vector<int64_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(static_cast<int64_t>(-(i % 64) - 1));
    }
    roundTrip(data, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt64), 100u);
}

TEST(ByteArrayVarint, Int64SmallCrossZero)
{
    std::vector<int64_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(static_cast<int64_t>(i % 64 - 32));
    }
    roundTrip(data, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt64), 100u);
}

TEST(ByteArrayVarint, Int64LargeNoCompress)
{
    std::vector<int64_t> data(100, INT64_MAX);
    roundTrip(data, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt64), 100u * 10);
}

TEST(ByteArrayVarint, Int64LargeNegativeNoCompress)
{
    std::vector<int64_t> data(100, INT64_MIN);
    roundTrip(data, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeInt64), 100u * 10);
}

TEST(ByteArrayVarint, Int64Boundaries)
{
    const std::vector<int64_t> cases = {
        0LL,
        1LL,
        -1LL,
        63LL,
        -64LL,
        64LL,
        -65LL,
        127LL,
        -128LL,
        128LL,
        -129LL,
        (1LL << 40),
        -(1LL << 40),
        (1LL << 62),
        -(1LL << 62) - 1,
        INT64_MAX,
        INT64_MIN,
        INT64_MAX - 1,
        INT64_MIN + 1,
    };
    roundTrip(cases, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
}

TEST(ByteArrayVarint, Int64RandomRoundTrip)
{
    std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
    auto data = makeData<int64_t>(1000, [&]
                                  { return dist(rng()); });
    roundTrip(data, &blue::ByteArray::writeInt64, &blue::ByteArray::readInt64);
    EXPECT_LE(writeSize(data, &blue::ByteArray::writeInt64), 1000u * 10);
    EXPECT_GE(writeSize(data, &blue::ByteArray::writeInt64), 1000u);
}

TEST(ByteArrayVarint, Uint32StepBoundaries)
{
    auto sizeOf = [](uint32_t v)
    {
        blue::ByteArray ba(1);
        ba.writeUint32(v);
        return ba.getSize();
    };
    EXPECT_EQ(sizeOf(0x7F), 1u);   // 127
    EXPECT_EQ(sizeOf(0x80), 2u);   // 128
    EXPECT_EQ(sizeOf(0x3FFF), 2u); // 16383
    EXPECT_EQ(sizeOf(0x4000), 3u); // 16384
    EXPECT_EQ(sizeOf(0x1FFFFF), 3u);
    EXPECT_EQ(sizeOf(0x200000), 4u);
    EXPECT_EQ(sizeOf(0x0FFFFFFF), 4u);
    EXPECT_EQ(sizeOf(0x10000000), 5u);
    EXPECT_EQ(sizeOf(UINT32_MAX), 5u);
}

// float / double

TEST(ByteArrayFloat, FloatFixedLength)
{
    std::uniform_real_distribution<float> dist(-1e6f, 1e6f);
    auto data = makeData<float>(1000, [&]
                                { return dist(rng()); });
    roundTripFloat(data, &blue::ByteArray::writeFloat, &blue::ByteArray::readFloat);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeFloat), 1000u * sizeof(float));
}

TEST(ByteArrayFloat, DoubleFixedLength)
{
    std::uniform_real_distribution<double> dist(-1e12, 1e12);
    auto data = makeData<double>(1000, [&]
                                 { return dist(rng()); });
    roundTripFloat(data, &blue::ByteArray::writeDouble, &blue::ByteArray::readDouble);
    EXPECT_EQ(writeSize(data, &blue::ByteArray::writeDouble), 1000u * sizeof(double));
}

TEST(ByteArrayFloat, SpecialValues)
{
    const std::vector<float> data = {
        0.0f,
        -0.0f,
        1.0f,
        -1.0f,
        3.1415926f,
        -2.71828f,
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
    };
    blue::ByteArray ba(1);
    for (auto x : data)
    {
        ba.writeFloat(x);
    }
    ba.setPosition(0);
    for (size_t i = 0; i < data.size(); i++)
    {
        float v = ba.readFloat();
        if (std::isinf(data[i]))
        {
            EXPECT_TRUE(std::isinf(v)) << "index " << i;
            EXPECT_EQ(std::signbit(v), std::signbit(data[i])) << "index " << i;
        }
        else if (data[i] == 0.0f)
        {
            EXPECT_EQ(v, 0.0f) << "index " << i;
            EXPECT_EQ(std::signbit(v), std::signbit(data[i])) << "index " << i;
        }
        else
        {
            EXPECT_FLOAT_EQ(v, data[i]) << "index " << i;
        }
    }
}

// 字符串

TEST(ByteArrayString, String16)
{
    const std::string s = "hello world";
    blue::ByteArray ba(1);
    ba.writeString16(s);
    ba.setPosition(0);
    EXPECT_EQ(ba.readString16(), s);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayString, String32)
{
    const std::string s = "hello world, hello blue";
    blue::ByteArray ba(1);
    ba.writeString32(s);
    ba.setPosition(0);
    EXPECT_EQ(ba.readString32(), s);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayString, String64)
{
    const std::string s = "hello world, hello blue, hello red, hello";
    blue::ByteArray ba(1);
    ba.writeString64(s);
    ba.setPosition(0);
    EXPECT_EQ(ba.readString64(), s);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayString, StringVint)
{
    const std::string s = "hello world, hello blue, hello red, hello";
    blue::ByteArray ba(1);
    ba.writeStringVint(s);
    ba.setPosition(0);
    EXPECT_EQ(ba.readStringVint(), s);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayString, EmptyString)
{
    const std::string s;
    blue::ByteArray ba(1);
    ba.writeString16(s);
    ba.setPosition(0);
    EXPECT_EQ(ba.readString16(), s);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayString, LongStringAcrossNodes)
{
    // 刻意超过 baseSize，跨多个节点
    std::string s;
    for (int i = 0; i < 5000; i++)
    {
        s.push_back(static_cast<char>('a' + (i % 26)));
    }
    blue::ByteArray ba(16);
    ba.writeString32(s);
    ba.setPosition(0);
    EXPECT_EQ(ba.readString32(), s);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

// 混合写入

TEST(ByteArrayMixed, MixedTypesRoundTrip)
{
    blue::ByteArray ba(8);

    ba.writeFint32(-12345);
    ba.writeUint64(0xDEADBEEFCAFEBABEULL);
    ba.writeString32("blue redis");
    ba.writeFloat(3.14f);
    ba.writeDouble(2.718281828);
    ba.writeInt32(-1);
    ba.writeUint32(300);

    ba.setPosition(0);
    EXPECT_EQ(ba.readFint32(), -12345);
    EXPECT_EQ(ba.readUint64(), 0xDEADBEEFCAFEBABEULL);
    EXPECT_EQ(ba.readString32(), "blue redis");
    EXPECT_FLOAT_EQ(ba.readFloat(), 3.14f);
    EXPECT_DOUBLE_EQ(ba.readDouble(), 2.718281828);
    EXPECT_EQ(ba.readInt32(), -1);
    EXPECT_EQ(ba.readUint32(), 300);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

// 文件读写

TEST(ByteArrayFile, FixedInt32ToFile)
{
    auto data = makeData<int32_t>(100, []
                                  { return static_cast<int32_t>(rng()()); });
    fileRoundTrip(data, &blue::ByteArray::writeFint32, &blue::ByteArray::readFint32, "fixed_int32");
}

TEST(ByteArrayFile, VarInt32ToFile)
{
    std::vector<int32_t> data;
    for (int i = 0; i < 100; i++)
    {
        data.push_back(i % 64 - 32);
    }
    fileRoundTrip(data, &blue::ByteArray::writeInt32, &blue::ByteArray::readInt32, "varint_int32");
}

TEST(ByteArrayFile, StringToFile)
{
    const std::string s = "hello world,hello blue";
    std::filesystem::create_directories("./tem");
    const std::string path = "./tem/string32.dat";

    blue::ByteArray ba(1);
    ba.writeString32(s);
    ba.setPosition(0);
    ASSERT_TRUE(ba.writeToFile(path));

    blue::ByteArray ba2(2);
    ASSERT_TRUE(ba2.readFromFile(path));
    ba2.setPosition(0);
    EXPECT_EQ(ba2.readString32(), s);
}

TEST(ByteArrayFile, FloatToFile)
{
    std::uniform_real_distribution<float> dist(-1e6f, 1e6f);
    auto data = makeData<float>(100, [&]
                                { return dist(rng()); });
    fileRoundTrip(data, &blue::ByteArray::writeFloat, &blue::ByteArray::readFloat,
                  "float");
}

// clear / setPosition / getReadSize

TEST(ByteArrayBasic, ClearResetsState)
{
    blue::ByteArray ba(4);
    for (int i = 0; i < 10; ++i)
    {
        ba.writeFint32(i);
    }
    EXPECT_EQ(ba.getSize(), 40u);
    EXPECT_EQ(ba.getCapacity(), 0u);

    ba.clear();
    EXPECT_EQ(ba.getSize(), 0u);
    EXPECT_EQ(ba.getPosition(), 0u);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayBasic, SetPositionOutOfRangeThrows)
{
    blue::ByteArray ba(4);
    ba.writeFint32(1);
    EXPECT_THROW(ba.setPosition(100), std::out_of_range);
}

TEST(ByteArrayBasic, ReadMoreThanAvailableThrows)
{
    blue::ByteArray ba(4);
    ba.writeFint32(1);
    ba.setPosition(0);
    int32_t a = ba.readFint32();
    EXPECT_EQ(a, 1);
    EXPECT_THROW(ba.readFint32(), std::out_of_range);
}

TEST(ByteArrayBasic, ToStringAndHexString)
{
    blue::ByteArray ba(4);
    ba.writeFuint8('A');
    ba.writeFuint8('B');
    ba.setPosition(0);
    EXPECT_EQ(ba.toString(), std::string("AB"));
    EXPECT_FALSE(ba.toHexString().empty());
}

TEST(ByteArrayBasic, MoveAndResetPosition)
{
    blue::ByteArray ba(4);
    ba.writeFint32(42);
    EXPECT_EQ(ba.getPosition(), 4u);
    EXPECT_EQ(ba.getSize(), 4u);

    ba.setPosition(0);
    EXPECT_EQ(ba.getPosition(), 0u);
    EXPECT_EQ(ba.getReadSize(), 4u);

    EXPECT_EQ(ba.readFint32(), 42);
    EXPECT_EQ(ba.getReadSize(), 0u);
}

TEST(ByteArrayBasic, GetReadBuffers)
{
    blue::ByteArray ba(4);
    for (int i = 0; i < 10; ++i)
    {
        ba.writeFuint8(static_cast<uint8_t>(i));
    }
    ba.setPosition(0);

    std::vector<iovec> vecs;
    uint64_t n = ba.getReadBuffers(vecs, 10);
    EXPECT_EQ(n, 10u);
    // 4 字节一页，10 字节需要 3 段
    EXPECT_EQ(vecs.size(), 3u);
}

// 测试字节序

TEST(ByteArrayEndian, FixedInt32Swap)
{
    blue::ByteArray be(4);
    be.setLittleEndian(false);
    be.writeFint32(0x01020304);
    be.setPosition(0);
    EXPECT_EQ(be.readFint32(), 0x01020304);

    blue::ByteArray le(4);
    le.setLittleEndian(true);
    le.writeFint32(0x01020304);
    le.setPosition(0);
    EXPECT_EQ(le.readFint32(), 0x01020304);
}

TEST(ByteArrayEndian, RawBytesDiffer)
{
    blue::ByteArray be(4), le(4);
    be.setLittleEndian(false);
    le.setLittleEndian(true);
    be.writeFint32(0x01020304);
    le.writeFint32(0x01020304);

    be.setPosition(0);
    le.setPosition(0);
    EXPECT_NE(be.toString(), le.toString()); // 两种端序字节流应该不同
}
