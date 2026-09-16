/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "blue/macro.h"
#include "blue/url.h"
#include "blue/urlutils.h"

namespace
{
#ifdef BLUE_WITH_IDN
    constexpr bool kHasIdn = BLUE_WITH_IDN;
#else
    constexpr bool kHasIdn = true;
#endif
    std::shared_ptr<blue::Url> CreateUrl(const std::string &val)
    {
        auto url = blue::Url::CreateUrl(val);
        EXPECT_NE(url, nullptr);
        return url;
    }
}

class BasicURLTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        url_ = CreateUrl("http://blue@www.baidu.com:80");
    }
    std::shared_ptr<blue::Url> url_;
};

TEST_F(BasicURLTest, CreateSuccess)
{
    EXPECT_NE(url_, nullptr);
}

TEST_F(BasicURLTest, SchemeIsHttp)
{
    EXPECT_EQ(url_->getScheme(), "http");
}

TEST_F(BasicURLTest, UserIsblue)
{
    EXPECT_EQ(url_->getDecodedUserinfo(), "blue");
}

TEST_F(BasicURLTest, HostIsBaidu)
{
    EXPECT_EQ(url_->getHost(), "www.baidu.com");
}

TEST_F(BasicURLTest, PortIs80)
{
    EXPECT_EQ(url_->getPort(), 80);
}

TEST_F(BasicURLTest, toString)
{
    EXPECT_THAT(url_->toString(), ::testing::HasSubstr("www.baidu.com"));
}

TEST_F(BasicURLTest, CreateAddress)
{
    const auto addr = url_->createAddress();
    EXPECT_NE(addr, nullptr);
    EXPECT_FALSE(addr->toString().empty());
}

class ChineseURLTest : public ::testing::Test
{
};

TEST_F(ChineseURLTest, ChineseDomain)
{
    const auto url1 = CreateUrl("http://中国.中国/路径");
    EXPECT_EQ(url1->getUnicodeHost(), "中国.中国");
    EXPECT_FALSE(url1->getHost().empty());
    ASSERT_THAT(url1->getHost(), ::testing::HasSubstr("xn--"));
    EXPECT_EQ(url1->getDecodedPath(), "/路径");
    EXPECT_EQ(url1->getUnicodeURL(), "http://中国.中国/路径");
}

TEST_F(ChineseURLTest, ChinesePathQueryFragment)
{
    const auto url = CreateUrl("http://example.com/文件/下载?名称=测试&类型=pdf#章节一");
    EXPECT_EQ(url->getPath(), "/文件/下载");
    EXPECT_EQ(url->getDecodedPath(), "/文件/下载");
    EXPECT_EQ(url->getDecodedQuery(), "名称=测试&类型=pdf");
    EXPECT_EQ(url->getDecodedFragment(), "章节一");
}

TEST_F(ChineseURLTest, Ipv6)
{
    const auto url = CreateUrl("http://[::1]:8080/path");
    EXPECT_NE(url, nullptr);
    ASSERT_EQ(url->getHost(), "[::1]");
    ASSERT_EQ(url->getHostType(), blue::HostType::IPv6);
    EXPECT_EQ(url->getPort(), 8080);
    EXPECT_EQ(url->getPath(), "/path");
}

TEST(URLUtilsTest, URLEncode)
{
    const auto encoed = blue::URLUtils::UrlEncode("中国");
    ASSERT_EQ(encoed, "%E4%B8%AD%E5%9B%BD");
}

TEST(URLUtilsTest, URLDecode)
{
    const auto decode = blue::URLUtils::UrlDecode("%E4%B8%AD%E5%9B%BD");
    ASSERT_EQ(decode, "中国");
}

TEST(UrlUtilsTest, EncodeDecodeRoundTrip)
{
    const std::string original = "中国abc123/+?=&";
    const auto encoded = blue::URLUtils::UrlEncode(original);
    EXPECT_EQ(blue::URLUtils::UrlDecode(encoded), original);
}

class IdnTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!kHasIdn)
        {
            GTEST_SKIP() << "libidn2 未启用,跳过 IDN 测试";
        }
    }
};

// Unicode -> Ascii
TEST_F(IdnTest, DomainToAscii)
{
    const auto ascii = blue::URLUtils::DomainToASCII("中国.中国");
    EXPECT_FALSE(ascii.empty());
    ASSERT_THAT(ascii, ::testing::HasSubstr("xn--"));
}

// Ascii -> Unicode
TEST_F(IdnTest, AsciiToDomain)
{
    const auto ascii = blue::URLUtils::DomainToASCII("中国.中国");
    const auto unicode = blue::URLUtils::DomainToUnicode(ascii);
    EXPECT_EQ(unicode, "中国.中国");
}

class AdvancedChineseURLTest : public ::testing::Test
{
};

TEST_F(AdvancedChineseURLTest, MixedChineseEnglish)
{
    const auto url = CreateUrl("http://中文host.com/中文路径/english-path?参数=value&中文键=中文值#片段");
    EXPECT_EQ(url->getScheme(), "http");
    EXPECT_EQ(url->getHost(), "xn--host-zf5fy05j.com");
    EXPECT_EQ(url->getUnicodeHost(), "中文host.com");
    EXPECT_EQ(url->getDecodedPath(), "/中文路径/english-path");
    EXPECT_EQ(url->getDecodedQuery(), "参数=value&中文键=中文值");
    EXPECT_EQ(url->getDecodedFragment(), "片段");
}

TEST_F(AdvancedChineseURLTest, PercentEncodedPath)
{
    const auto url = CreateUrl("http://example.com/%E4%B8%AD%E6%96%87");
    EXPECT_EQ(url->getScheme(), "http");
    EXPECT_EQ(url->getPath(), "/%E4%B8%AD%E6%96%87");
    EXPECT_EQ(url->getDecodedPath(), "/中文");
    EXPECT_EQ(url->getPort(), 80);
}

TEST_F(AdvancedChineseURLTest, SpecialCharactersInQuery)
{
    const auto url = CreateUrl("http://example.com/path?q=hello%20world&filter=价格>100");
    EXPECT_EQ(url->getDecodedQuery(), "q=hello world&filter=价格>100");
}

TEST_F(AdvancedChineseURLTest, FullChineseUrl)
{
    const auto url = CreateUrl("https://用户:密码@中国域名.中国:8080/文档/报告.pdf?标题=年度总结&作者=张三#摘要");
    EXPECT_EQ(url->getScheme(), "https");
    EXPECT_EQ(url->getDecodedUserinfo(), "用户:密码");
    EXPECT_EQ(url->getUnicodeHost(), "中国域名.中国");
    EXPECT_EQ(url->getPort(), 8080);
    EXPECT_EQ(url->getDecodedPath(), "/文档/报告.pdf");
    EXPECT_EQ(url->getDecodedQuery(), "标题=年度总结&作者=张三");
    EXPECT_EQ(url->getDecodedFragment(), "摘要");
}

class ValidURLParamTest : public ::testing::TestWithParam<const char *>
{
};

TEST_P(ValidURLParamTest, CreateSucceed)
{
    auto url = blue::Url::CreateUrl(GetParam());
    EXPECT_NE(url, nullptr) << "有效URL: " << GetParam();
}

INSTANTIATE_TEST_SUITE_P(ValidUrls,
                         ValidURLParamTest,
                         ::testing::Values(
                             "http://blue@www.baidu.com:80",
                             "http://中国.中国/路径",
                             "http://example.com/文件/下载?名称=测试&类型=pdf#章节一",
                             "http://[::1]:8080/path",
                             "http://中文host.com/中文路径/english-path?参数=value&中文键=中文值#片段",
                             "http://example.com/%E4%B8%AD%E6%96%87",
                             "http://example.com/path?q=hello%20world&filter=价格>100",
                             "https://用户:密码@中国域名.中国:8080/文档/报告.pdf?标题=年度总结&作者=张三#摘要"));