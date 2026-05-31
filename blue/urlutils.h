#ifndef BLUE_URILUTILS_H
#define BLUE_URILUTILS_H
#define HAVE_LIBIDN2 1
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

#ifdef HAVE_LIBIDN2
#include <idn2.h>
#endif

// url解码和编码
namespace blue
{
    /* 支持unicode 和 ascii 互相转化*/
    class URLUtils
    {
    public:
        /**
         * @brief 将 Unicode 域名转换为 ASCII
         * @param domain Unicode 域名
         * @return ASCII 域名
         */
        static std::string DomainToASCII(const std::string &domain)
        {
#ifdef HAVE_LIBIDN2
            // 检查是否需要转换
            if (!_ContainsNonASCII(domain))
            {
                return domain;
            }

            char *ascii = nullptr;
            int rc = idn2_to_ascii_8z(domain.c_str(), &ascii, IDN2_NONTRANSITIONAL);
            if (rc == IDN2_OK)
            {
                std::string result(ascii);
                free(ascii);
                return result;
            }
            // 转换失败，返回原始字符串
            if (ascii)
            {
                free(ascii);
            }
            return domain;
#else
            // 如果没有 libidn2，简单返回原字符串
            return domain;
#endif
        }

        /**
         * @brief 将 ASCII (Punycode) 转换回 Unicode
         * @param ascii ASCII 域名
         * @return Unicode 域名
         */
        static std::string DomainToUnicode(const std::string &ascii)
        {
#ifdef HAVE_LIBIDN2
            char *unicode = nullptr;
            int rc = idn2_to_unicode_8z8z(ascii.c_str(), &unicode, 0);
            if (rc == IDN2_OK)
            {
                std::string result(unicode);
                free(unicode);
                return result;
            }
            if (unicode)
            {
                free(unicode);
            }
            return ascii;
#else
            return ascii;
#endif
        }

        /**
         * @brief URL 编码
         * @param str 需要编码的字符串
         * @param encodeAll 是否全部编码,false保留(字母,数字,-_.~)
         * @return 编码结果
         */
        static std::string UrlEncode(const std::string &str, bool encodeAll = false)
        {
            std::ostringstream escaped;
            escaped.fill('0');

            for (size_t i = 0; i < str.length(); ++i)
            {
                unsigned char c = static_cast<unsigned char>(str[i]);

                if (!encodeAll && (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'))
                {
                    escaped << c;
                }
                else
                {
                    escaped << std::uppercase;
                    escaped << '%' << std::setw(2) << std::hex << static_cast<int>(c);
                    escaped << std::nouppercase;
                }
            }
            return escaped.str();
        }

        /**
         * @brief URL 解码
         * @param str 需要解码的字符串
         */
        static std::string UrlDecode(const std::string &str)
        {
            std::string result;
            result.reserve(str.length());

            for (size_t i = 0; i < str.length(); ++i)
            {
                if (str[i] == '%' && i + 2 < str.length() &&
                    std::isxdigit(str[i + 1]) && std::isxdigit(str[i + 2]))
                {
                    // 百分号编码
                    int value;
                    std::istringstream is(str.substr(i + 1, 2));
                    if (is >> std::hex >> value)
                    {
                        result += static_cast<char>(value);
                        i += 2;
                    }
                    else
                    {
                        result += str[i];
                    }
                }
                else if (str[i] == '+')
                {
                    // 加号转为空格
                    result += ' ';
                }
                else
                {
                    result += str[i];
                }
            }
            return result;
        }

        /**
         * @brief 检查路径是否需要编码
         * @param path 需要检查的路径
         * @return 编码后的字符串
         */
        static std::string EncodePath(const std::string &path)
        {
            std::string result;
            result.reserve(path.length());

            for (size_t i = 0; i < path.length(); ++i)
            {
                unsigned char c = static_cast<unsigned char>(path[i]);

                // 路径中允许的未保留字符 + 子分隔符 + : @ /
                if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
                    c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' ||
                    c == '*' || c == '+' || c == ',' || c == ';' || c == '=' ||
                    c == ':' || c == '@' || c == '/')
                {
                    result += c;
                }
                else
                {
                    result += UrlEncode(std::string(1, c), true);
                }
            }
            return result;
        }

    private:
        /**
         * @brief 检查字符串是否包含非 ASCII 字符
         * @param str 需要检查的字符串
         */
        static bool _ContainsNonASCII(const std::string &str)
        {
            return std::any_of(str.begin(), str.end(),
                               [](char c)
                               { return static_cast<unsigned char>(c) > 127; });
        }
    };
}

#endif