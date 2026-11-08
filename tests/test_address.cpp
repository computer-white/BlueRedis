/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "sys/un.h"
#include "src/net/address.h"

namespace
{
    sockaddr_in makeSocketAddrIn(const char *ip, uint16_t port)
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
        return addr;
    }

    sockaddr_in6 makeSocketAddrIn6(const char *ip, uint16_t port)
    {
        sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port);
        inet_pton(AF_INET6, ip, &addr.sin6_addr);
        return addr;
    }
}

// IPV4 基础测试
TEST(IPV4AddressTest, BasicProperties)
{
    auto addr = blue::IPv4Address::Create("192.168.1.100", 8080);
    ASSERT_NE(addr, nullptr);

    EXPECT_EQ(addr->getFamily(), AF_INET);
    EXPECT_EQ(addr->getPort(), 8080);
    EXPECT_EQ(addr->getIp(), "192.168.1.100");
    EXPECT_EQ(addr->toString(), "192.168.1.100:8080");
    EXPECT_EQ(addr->toString(false), "192.168.1.100");
    EXPECT_EQ(addr->getAddrLen(), sizeof(sockaddr_in));
}

TEST(IPV4AddressTest, CreateFromSockaddr)
{
    sockaddr_in raw_addr = makeSocketAddrIn("10.0.0.1", 443);
    auto addr = std::make_shared<blue::IPv4Address>(raw_addr);

    EXPECT_EQ(addr->getIp(), "10.0.0.1");
    EXPECT_EQ(addr->getPort(), 443);
}

TEST(IPV4AddressTest, PortMutation)
{
    auto addr = blue::IPv4Address::Create("127.0.0.1", 80);
    ASSERT_NE(addr, nullptr);

    EXPECT_EQ(addr->getPort(), 80);
    addr->setPort(65535);
    EXPECT_EQ(addr->getPort(), 65535);
    EXPECT_EQ(addr->toString(), "127.0.0.1:65535");
}

TEST(IPV4AddressTest, EqualityAndSorting)
{
    auto a1 = blue::IPv4Address::Create("192.168.1.1", 80);
    auto a2 = blue::IPv4Address::Create("192.168.1.1", 80);
    auto a3 = blue::IPv4Address::Create("192.168.1.1", 8080);
    auto a4 = blue::IPv4Address::Create("192.168.1.2", 80);

    EXPECT_TRUE(*a1 == *a2);
    EXPECT_FALSE(*a1 == *a3);
    EXPECT_TRUE(*a1 != *a3);

    // a1 (port 80) < a1 (port 8080)
    EXPECT_TRUE(*a1 < *a3);
    // a1 (192.168.1.1) < a4 (192.168.1.2)
    EXPECT_TRUE(*a1 < *a4);
}

// IPV4 子网计算
TEST(IPV4AddressTest, SubnetCalculations)
{
    auto ip = blue::IPv4Address::Create("192.168.1.100", 8080);
    ASSERT_NE(ip, nullptr);

    uint32_t prefix = 24;

    auto mask = ip->subnetMask(prefix);
    EXPECT_EQ(mask->getIp(), "255.255.255.0");

    auto network = ip->networkAddress(prefix);
    EXPECT_EQ(network->getIp(), "192.168.1.0");

    auto broadcast = ip->broadcastAddress(prefix);
    EXPECT_EQ(broadcast->getIp(), "192.168.1.255");

    // 测试 /16 掩码 (255.255.0.0)
    prefix = 16;
    mask = ip->subnetMask(prefix);
    EXPECT_EQ(mask->getIp(), "255.255.0.0");

    network = ip->networkAddress(prefix);
    EXPECT_EQ(network->getIp(), "192.168.0.0");

    broadcast = ip->broadcastAddress(prefix);
    EXPECT_EQ(broadcast->getIp(), "192.168.255.255");

    // 测试越界
    EXPECT_EQ(ip->subnetMask(32), nullptr);
    EXPECT_EQ(ip->networkAddress(32), nullptr);
    EXPECT_EQ(ip->broadcastAddress(32), nullptr);
}

// IPV6 Address 基础测试
TEST(IPV6AddressTest, BasicProperties)
{
    auto addr = blue::IPv6Address::Create("2001:db8::1", 8080);
    ASSERT_NE(addr, nullptr);

    EXPECT_EQ(addr->getFamily(), AF_INET6);
    EXPECT_EQ(addr->getPort(), 8080);
    EXPECT_EQ(addr->getIp(), "2001:db8::1");
    EXPECT_EQ(addr->toString(), "[2001:db8::1]:8080");
    EXPECT_EQ(addr->toString(false), "[2001:db8::1]");
}

// IPV6 子网计算
TEST(IPV6AddressTest, SubnetCalculations)
{
    auto ip = blue::IPv6Address::Create("2001:db8::1234", 80);
    ASSERT_NE(ip, nullptr);

    // /64 掩码 (前 64 位全 1)
    uint32_t prefix = 64;

    auto mask = ip->subnetMask(prefix);
    EXPECT_EQ(mask->getIp(), "ffff:ffff:ffff:ffff::");

    auto network = ip->networkAddress(prefix);
    EXPECT_EQ(network->getIp(), "2001:db8::");

    auto broadcast = ip->broadcastAddress(prefix);
    EXPECT_EQ(broadcast->getIp(), "2001:db8::ffff:ffff:ffff:ffff");

    // 测试非 8 字节对齐的掩码 /68 (64 + 4)
    prefix = 68;
    mask = ip->subnetMask(prefix);
    EXPECT_EQ(mask->getIp(), "ffff:ffff:ffff:ffff:f000::");

    network = ip->networkAddress(prefix);
    EXPECT_EQ(network->getIp(), "2001:db8::");

    // 测试越界
    EXPECT_EQ(ip->subnetMask(128), nullptr);
    EXPECT_EQ(ip->networkAddress(128), nullptr);
    EXPECT_EQ(ip->broadcastAddress(128), nullptr);
}

// Unix Address 测试
TEST(UnixAddressTest, BasicProperties)
{
    blue::UnixAddress addr("/tmp/blue.sock");
    EXPECT_EQ(addr.getFamily(), AF_UNIX);
    EXPECT_EQ(addr.toString(), "/tmp/blue.sock");

    // 路径长度测试
    socklen_t expected_len = strlen("/tmp/blue.sock") + 1 + offsetof(sockaddr_un, sun_path);
    EXPECT_EQ(addr.getAddrLen(), expected_len);
}

TEST(UnixAddressTest, AbstractSocket)
{
    // Linux 特有：以 \0 开头的抽象命名空间
    std::string path("\0blue_abs", 9);
    blue::UnixAddress addr(path);
    EXPECT_EQ(addr.getFamily(), AF_UNIX);
    // toString 应该显示 \0 前缀
    EXPECT_EQ(addr.toString(), "\\0blue_abs");
}

// Address 工厂与多态行为测试
TEST(AddressFactoryTest, CreateFromSockaddr)
{
    // IPv4
    sockaddr_in v4_raw = makeSocketAddrIn("1.1.1.1", 53);
    auto addr4 = blue::Address::Create((sockaddr *)&v4_raw, sizeof(v4_raw));
    ASSERT_NE(addr4, nullptr);
    EXPECT_EQ(addr4->getFamily(), AF_INET);
    EXPECT_EQ(addr4->toString(), "1.1.1.1:53");

    // IPv6
    sockaddr_in6 v6_raw = makeSocketAddrIn6("::1", 53);
    auto addr6 = blue::Address::Create((sockaddr *)&v6_raw, sizeof(v6_raw));
    ASSERT_NE(addr6, nullptr);
    EXPECT_EQ(addr6->getFamily(), AF_INET6);
    EXPECT_EQ(addr6->toString(), "[::1]:53");

    // Unknown
    sockaddr unknown_raw;
    memset(&unknown_raw, 0, sizeof(unknown_raw));
    unknown_raw.sa_family = AF_UNSPEC;
    auto addr_unknown = blue::Address::Create(&unknown_raw, sizeof(unknown_raw));
    ASSERT_NE(addr_unknown, nullptr);
    EXPECT_EQ(addr_unknown->getFamily(), AF_UNSPEC);
    EXPECT_EQ(addr_unknown->toString(), "[ family = 0]");
}

TEST(IPAddressTest, CreateAndCast)
{
    // 测试 IPAddress::Create 直接解析 IP
    auto ip = blue::IPAddress::Create("8.8.8.8", 53);
    ASSERT_NE(ip, nullptr);
    EXPECT_EQ(ip->getIp(), "8.8.8.8");
    EXPECT_EQ(ip->getPort(), 53);

    // 测试解析失败
    auto bad_ip = blue::IPAddress::Create("not_an_ip", 53);
    EXPECT_EQ(bad_ip, nullptr);
}

// DNS 解析与接口查询
TEST(AddressLookupTest, LookupLocalhost)
{
    // 测试纯 IP
    auto ip1 = blue::Address::LookupAny("127.0.0.1:8080");
    ASSERT_NE(ip1, nullptr);
    EXPECT_EQ(ip1->toString(), "127.0.0.1:8080");

    // 测试带协议的 URL
    auto ip2 = blue::Address::LookupAny("http://192.168.1.1:80/index.html");
    ASSERT_NE(ip2, nullptr);
    EXPECT_EQ(ip2->toString(), "192.168.1.1:80");

    // 测试带方括号的 IPv6
    auto ip3 = blue::Address::LookupAny("http://[::1]:9090/path", AF_UNSPEC);
    ASSERT_NE(ip3, nullptr);
    EXPECT_EQ(ip3->toString(), "[::1]:9090");
}

TEST(AddressLookupTest, LookupDomain)
{
    auto addr = blue::Address::LookupAny("localhost:80");
    ASSERT_NE(addr, nullptr);
    EXPECT_TRUE(addr->getFamily() == AF_INET || addr->getFamily() == AF_INET6);
}

TEST(AddressLookupTest, GetInterfaceAddress)
{
    std::vector<std::pair<std::shared_ptr<blue::Address>, uint32_t>> results;
    std::string loopback = "lo"; // Linux 下 loopback 接口名固定为 lo

    bool success = blue::Address::GetInterfaceAddress(results, loopback, AF_INET);
    EXPECT_TRUE(success);
    EXPECT_FALSE(results.empty()); // 至少有 127.0.0.1

    // 验证确实拿到了回环地址
    bool found_loopback = false;
    for (const auto &[addr, prefix] : results)
    {
        auto ipaddr = std::dynamic_pointer_cast<blue::IPAddress>(addr);
        if (ipaddr->getIp() == "127.0.0.1")
        {
            found_loopback = true;
            EXPECT_EQ(prefix, 8); // 回环地址掩码通常是 /8
            break;
        }
    }
    EXPECT_TRUE(found_loopback);

    // 测试 * 通配符（返回 0.0.0.0 和 ::）
    std::vector<std::pair<std::shared_ptr<blue::Address>, uint32_t>> wildcard_results;
    std::string interface = "*";
    blue::Address::GetInterfaceAddress(wildcard_results, interface, AF_UNSPEC);
    EXPECT_EQ(wildcard_results.size(), 2); // IPv4 和 IPv6 的 INADDR_ANY
}