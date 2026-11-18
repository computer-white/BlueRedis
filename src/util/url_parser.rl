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
// url_parser.rl
// 用法: ragel -G2 -o url_parser.cpp url_parser.rl
// 或者: ragel -T0 -o url_parser.cpp url_parser.rl  (表驱动)

#include <iostream>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include "src/util/url.h"
#include "src/util/urlutils.h"

%%{
    machine url_parser;
    write data;
}%%

namespace blue
{
    std::shared_ptr<Url> Url::CreateUrl(const std::string& url_str)
    {
        try
        {
            auto url = std::make_shared<Url>();

            int cs;
            const char* p   = url_str.c_str();
            const char* pe  = p + url_str.size();
            const char* eof = pe;

            const char* scheme_start   = nullptr;
            const char* userinfo_start = nullptr;
            const char* host_start     = nullptr;
            const char* port_start     = nullptr;
            const char* path_start     = nullptr;
            const char* query_start    = nullptr;
            const char* fragment_start = nullptr;

            int host_type = 0;  // 0=域名, 4=IPv4, 6=IPv6

            %%{
                action mark_scheme   { scheme_start   = fpc; }
                action mark_userinfo { userinfo_start = fpc; }
                action mark_host     { host_start     = fpc; }
                action mark_port     { port_start     = fpc; }
                action mark_path     { path_start     = fpc; }
                action mark_query    { query_start    = fpc; }
                action mark_fragment { fragment_start = fpc; }
                action mark_ipv4     { host_type = 4; }
                action mark_ipv6     { host_type = 6; }

                # 字符集定义
                ALPHA = alpha;
                DIGIT = digit;
                HEX   = xdigit;

                # 添加unicode支持
                # === UTF-8 多字节字符支持 ===
                # UTF-8 编码范围：
                # 2字节: U+0080-U+07FF -> C2..DF 80..BF
                # 3字节: U+0800-U+FFFF -> E0..EF 80..BF 80..BF
                # 4字节: U+10000-U+10FFFF -> F0..F4 80..BF 80..BF 80..BF
                UTF8_2 = 0xc2..0xdf 0x80..0xbf;
                UTF8_3 = 0xe0..0xef 0x80..0xbf 0x80..0xbf;
                UTF8_4 = 0xf0..0xf4 0x80..0xbf 0x80..0xbf 0x80..0xbf;
                UTF8_CHAR = UTF8_2 | UTF8_3 | UTF8_4;

                # 字符集
                IUNRESERVED  = alnum | UTF8_CHAR | [\-._~];

                # 分隔符
                SUB_DELIMS  = [!\$&'()*+,;=];

                # 路径字符
                IPCHAR       = IUNRESERVED | SUB_DELIMS | [:@] | '%' HEX{2};

                # Scheme
                SCHEME = ALPHA (ALPHA | DIGIT | [+\-.])*;

                # Userinfo
                USERINFO = (IUNRESERVED | SUB_DELIMS | '%' HEX{2} | ':')*;

                # === IPv4 解析 ===
                dec_octet = 
                    ( '25' [0-5]          |
                    '2' [0-4] DIGIT     |
                    '1' DIGIT{2}        |
                    [1-9] DIGIT         |
                    DIGIT );

                # 不带 mark 的 IPv4，供 IPv6 的 ls32 复用
                IPv4address_plain = dec_octet '.' dec_octet '.' dec_octet '.' dec_octet;
                IPv4address       = IPv4address_plain %mark_ipv4;

                h16 = HEX{1,4};
                ls32 = h16 ':' h16 | IPv4address_plain;

                IPv6address = (
                    ( h16 ':' ){6} ls32 |
                    '::' ( h16 ':' ){5} ls32 |
                    ( h16 )? '::' ( h16 ':' ){4} ls32 |
                    ( ( h16 ':' ){0,1} h16 )? '::' ( h16 ':' ){3} ls32 |
                    ( ( h16 ':' ){0,2} h16 )? '::' ( h16 ':' ){2} ls32 |
                    ( ( h16 ':' ){0,3} h16 )? '::' h16 ':' ls32 |
                    ( ( h16 ':' ){0,4} h16 )? '::' ls32 |
                    ( ( h16 ':' ){0,5} h16 )? '::' h16 |
                    ( ( h16 ':' ){0,6} h16 )? '::'
                ) %mark_ipv6;

                IPvFuture = 'v' HEX+ '.' (IUNRESERVED | SUB_DELIMS | ':')+;
                IP_literal = '[' ( IPv6address | IPvFuture ) ']';

                HOSTCHAR = alnum | UTF8_CHAR | [\-._~!$&'()*+,;=%];
                HOST = ( IP_literal | IPv4address | HOSTCHAR+ ) >mark_host;

                PORT = DIGIT*;

                SEGMENT  = IPCHAR*;
                PATH     = ('/' SEGMENT)*;

                QUERY_CHAR = (print - '#') | UTF8_CHAR | '%' HEX{2};
                QUERY = QUERY_CHAR*;

                FRAGMENT_CHAR = print | UTF8_CHAR | '%' HEX{2};
                FRAGMENT = FRAGMENT_CHAR*;

                # 注意：去掉末尾的 0，用 cs 判断是否到达 final
                main := (
                    ( SCHEME >mark_scheme '://'
                    ( USERINFO >mark_userinfo '@' )?
                    HOST
                    ( ':' PORT >mark_port )?
                    )?
                    PATH >mark_path
                    ( '?' QUERY >mark_query )?
                    ( '#' FRAGMENT >mark_fragment )?
                );

                write init;
                write exec;
            }%%

            if (cs < url_parser_first_final || cs == url_parser_error)
            {
                return nullptr;
            }

            // 计算每个字段的右边界：取"下一个已知起点"的最小值，再回退分隔符
            auto next_field = [&](std::initializer_list<const char*> starts) -> const char* {
                const char* end = pe;
                for (auto s : starts)
                {
                    if (s && s < end)
                    {
                        end = s;
                    }
                }
                return end;
            };

            if (scheme_start)
            {
                const char* scheme_end = std::find(scheme_start, next_field({host_start, pe}), ':');
                if (scheme_end > scheme_start)
                {
                    url->setScheme(std::string(scheme_start, scheme_end));
                }
            }

            if (userinfo_start)
            {
                const char* userinfo_end = next_field({host_start, pe});
                if (userinfo_end > userinfo_start && *(userinfo_end - 1) == '@')
                {
                    --userinfo_end;  // 去掉 '@'
                }
                if (userinfo_end > userinfo_start)
                {
                    std::string userinfo(userinfo_start, userinfo_end);
                    url->setUserinfo(userinfo);
                    url->setDecodedUserinfo(URLUtils::UrlDecode(userinfo));
                }
            }

            if (host_start)
            {
                const char* host_end = next_field({port_start, path_start, query_start, fragment_start, pe});
                // port_start 指向 ':' 后第一个数字，':' 本身要排除
                if (port_start && host_end == port_start && port_start > host_start)
                {
                    --host_end;
                }

                std::string host_str;
                if (host_end > host_start)
                {
                    host_str = std::string(host_start, host_end);
                }

                bool hasbrackets = false;
                if (host_str.size() >= 2 && host_str.front() == '[' && host_str.back() == ']')
                {
                    host_str = host_str.substr(1, host_str.size() - 2);
                    hasbrackets = true;
                }

                if (host_type == 4)
                {
                    url->m_hostType = HostType::IPv4;
                    url->setHost(host_str);
                    inet_pton(AF_INET, host_str.c_str(), url->m_ipBinary.data());
                }
                else if (host_type == 6)
                {
                    url->m_hostType = HostType::IPv6;
                    url->setHost(hasbrackets ? "[" + host_str + "]" : host_str);
                    inet_pton(AF_INET6, host_str.c_str(), url->m_ipBinary.data());
                }
                else
                {
                    url->m_hostType = HostType::HOSTNAME;
                    url->setUnicodeHost(host_str);
                    url->setHost(URLUtils::DomainToASCII(host_str));
                }
            }

            if (port_start)
            {
                const char* port_end = next_field({path_start, query_start, fragment_start, pe});
                if (port_end > port_start)
                {
                    std::string port_str(port_start, port_end);
                    url->setPort(port_str.empty() ? 0 : std::stoi(port_str));
                }
            }

            if (path_start)
            {
                // path 可能以 '?' 或 '#' 结束，但 path_start 本身是 '/'，不会误含分隔符
                const char* path_end = next_field({query_start, fragment_start, pe});
                if (path_end > path_start && *(path_end - 1) == '?') --path_end;  // 防御
                if (path_end > path_start)
                {
                    std::string path(path_start, path_end);
                    url->setPath(path);
                    url->setDecodedPath(URLUtils::UrlDecode(path));
                }
            }

            if (query_start)
            {
                const char* query_end = next_field({fragment_start, pe});
                if (query_end > query_start && *(query_end - 1) == '#') --query_end;
                if (query_end > query_start)
                {
                    std::string query(query_start, query_end);
                    url->setQuery(query);
                    url->setDecodedQuery(URLUtils::UrlDecode(query));
                }
            }

            if (fragment_start)
            {
                std::string fragment(fragment_start, pe);
                url->setFragment(fragment);
                url->setDecodedFragment(URLUtils::UrlDecode(fragment));
            }
            if (url)
            {
                return url;
            }
            return nullptr;
        }
        catch(...)
        {
            return nullptr;
        }
    }

} // namespace blue