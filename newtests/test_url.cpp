#include <iostream>
#include "blue/macro.h"
#include "blue/url.h"
#include "blue/urlutils.h"

void testBasicURL()
{
    // 测试基本 URL
    auto url = blue::Url::CreateUrl("http://blue@www.baidu.com:80");
    std::cout << url->toString() << std::endl;
    auto addr = url->createAddress();
    std::cout << addr->toString() << std::endl;

    std::cout << "Basic URL test passed!" << std::endl;
}

void testChineseURL()
{

    // 测试中文域名
    auto url1 = blue::Url::CreateUrl("http://中国.中国/路径");
    std::cout << "Original host: " << url1->getUnicodeHost() << std::endl;
    std::cout << "ASCII host: " << url1->getHost() << std::endl;
    std::cout << "Decoded path: " << url1->getDecodedPath() << std::endl;
    std::cout << "Display URL: " << url1->getUnicodeURL() << std::endl;

    // 测试中文路径和查询参数
    auto url2 = blue::Url::CreateUrl("http://example.com/文件/下载?名称=测试&类型=pdf#章节一");
    std::cout << "Path: " << url2->getPath() << std::endl;
    std::cout << "Decoded path: " << url2->getDecodedPath() << std::endl;
    std::cout << "Decoded query: " << url2->getDecodedQuery() << std::endl;
    std::cout << "Decoded fragment: " << url2->getDecodedFragment() << std::endl;

    // 测试 IPv6
    auto url3 = blue::Url::CreateUrl("http://[::1]:8080/path");
    BLUE_ASSERT(url3->getHost() == "[::1]");
    BLUE_ASSERT(url3->getHostType() == blue::HostType::IPv6);

    std::cout << "\nChinese URL tests passed!" << std::endl;
}

void testURLUtils()
{
    // 测试 URL 编码
    std::string encoded = blue::URLUtils::UrlEncode("中国");
    BLUE_ASSERT(encoded == "%E4%B8%AD%E5%9B%BD");

    // 测试 URL 解码
    std::string decoded = blue::URLUtils::UrlDecode("%E4%B8%AD%E5%9B%BD");
    BLUE_ASSERT(decoded == "中国");

    // 测试 IDN 转换（需要 libidn2）
    std::string ascii = blue::URLUtils::DomainToASCII("中国.中国");
    std::cout << "IDN ASCII: " << ascii << std::endl;

    std::string unicode = blue::URLUtils::DomainToUnicode(ascii);
    std::cout << "IDN Unicode: " << unicode << std::endl;

    std::cout << "URL Utils tests passed!" << std::endl;
}

void testAdvancedChineseURL()
{
    // 测试1：混合中英文
    auto url1 = blue::Url::CreateUrl("http://中文host.com/中文路径/english-path?参数=value&中文键=中文值#片段");
    std::cout << url1->getHost() << std::endl;
    BLUE_ASSERT(url1->getHost() == "xn--host-zf5fy05j.com");
    BLUE_ASSERT(url1->getDecodedPath() == "/中文路径/english-path");
    BLUE_ASSERT(url1->getDecodedQuery() == "参数=value&中文键=中文值");

    // 测试2：百分号编码的中文
    auto url2 = blue::Url::CreateUrl("http://example.com/%E4%B8%AD%E6%96%87");
    BLUE_ASSERT(url2->getDecodedPath() == "/中文");

    // 测试3：特殊字符
    auto url3 = blue::Url::CreateUrl("http://example.com/path?q=hello%20world&filter=价格>100");
    BLUE_ASSERT(url3);
    BLUE_ASSERT(url3->getDecodedQuery() == "q=hello world&filter=价格>100");

    // 测试4：完整的中文URL
    auto url4 = blue::Url::CreateUrl("https://用户:密码@中国域名.中国:8080/文档/报告.pdf?标题=年度总结&作者=张三#摘要");
    BLUE_ASSERT(url4->getScheme() == "https");
    BLUE_ASSERT(url4->getDecodedUserinfo() == "用户:密码");
    BLUE_ASSERT(url4->getUnicodeHost() == "中国域名.中国");
    BLUE_ASSERT(url4->getPort() == 8080);
    BLUE_ASSERT(url4->getDecodedPath() == "/文档/报告.pdf");
    BLUE_ASSERT(url4->getDecodedQuery() == "标题=年度总结&作者=张三");
    BLUE_ASSERT(url4->getDecodedFragment() == "摘要");

    std::cout << "Advanced Chinese URL tests passed!" << std::endl;
}

int main(int argc, char *argv[])
{

    testBasicURL();
    testChineseURL();
    testURLUtils();
    testAdvancedChineseURL();
    return 0;
}