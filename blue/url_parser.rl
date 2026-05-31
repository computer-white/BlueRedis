// url_parser.rl
// 用法: ragel -G2 -o url_parser.cpp url_parser.rl
// 或者: ragel -T0 -o url_parser.cpp url_parser.rl  (表驱动)

#include <iostream>
#include <cstring>
#include <algorithm>
#include "url.h"
#include "urlutils.h"

%%{
    machine url_parser;
    write data;
}%%

namespace blue
{
    std::shared_ptr<Url> Url::CreateUrl(const std::string& url_str)
    {
        auto url = std::make_shared<Url>();
        
        int cs;
        const char* p   = url_str.c_str();
        const char* pe  = p + url_str.size();
        const char* eof = pe;

        // 各字段的标记指针
        const char* scheme_start   = nullptr;
        const char* userinfo_start = nullptr;
        const char* host_start     = nullptr;
        const char* port_start     = nullptr;
        const char* path_start     = nullptr;
        const char* query_start    = nullptr;
        const char* fragment_start = nullptr;

        // 标记 IP 类型
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
            
            IPv4address = dec_octet '.' dec_octet '.' dec_octet '.' dec_octet >mark_ipv4;

            # === IPv6 解析 ===
            h16 = HEX{1,4};
            ls32 = h16 ':' h16 | IPv4address;
            
            IPv6address = (
                ( h16 ':' ){6} ls32 >mark_ipv6 |
                '::' ( h16 ':' ){5} ls32 >mark_ipv6 |
                ( h16 )? '::' ( h16 ':' ){4} ls32 >mark_ipv6 |
                ( ( h16 ':' ){0,1} h16 )? '::' ( h16 ':' ){3} ls32 >mark_ipv6 |
                ( ( h16 ':' ){0,2} h16 )? '::' ( h16 ':' ){2} ls32 >mark_ipv6 |
                ( ( h16 ':' ){0,3} h16 )? '::' h16 ':' ls32 >mark_ipv6 |
                ( ( h16 ':' ){0,4} h16 )? '::' ls32 >mark_ipv6 |
                ( ( h16 ':' ){0,5} h16 )? '::' h16 >mark_ipv6 |
                ( ( h16 ':' ){0,6} h16 )? '::' >mark_ipv6
            );

            # IPvFuture (IPv6 扩展，极少用)
            IPvFuture = 'v' HEX+ '.' (IUNRESERVED | SUB_DELIMS | ':')+;

            # === 括号包裹的 IP 字面量（如 http://[::1]:8080/）===
            IP_literal = '[' ( IPv6address | IPvFuture ) ']';

            # === Host 定义 ===
            HOSTCHAR = alnum | UTF8_CHAR | [\-._~!$&'()*+,;=%];
            HOST = ( IP_literal | IPv4address | HOSTCHAR* ) >mark_host;

            # Port
            PORT = DIGIT*;

            # Path
            SEGMENT  = IPCHAR*;
            PATH     = ('/' SEGMENT)*;

            # Query
            QUERY_CHAR = (print - '#') | UTF8_CHAR | '%' HEX{2};
            QUERY = QUERY_CHAR*;

            # Fragment
            FRAGMENT_CHAR = print | UTF8_CHAR | '%' HEX{2};
            FRAGMENT = FRAGMENT_CHAR*;

            # 主规则
            main := (
                ( SCHEME >mark_scheme '://'
                ( USERINFO >mark_userinfo '@' )?
                HOST >mark_host
                ( ':' PORT >mark_port )?
                )?
                PATH >mark_path
                ( '?' QUERY >mark_query )?
                ( '#' FRAGMENT >mark_fragment )?
            ) 0;

            write data;
            write init;
            write exec;
        }%%

        (void)url_parser_en_main;

        /*
            http://user@www.baidu.com:8080/blue/xxx?query#fragment
            ^      ^    ^             ^   ^        ^     ^
            |      |    |             |   |        |     |
           s_s   u_s  h_s            po_s pth_s    q_s   f_s 
        */
        if (p == pe) 
        {
            if (scheme_start)
            {
                const char* scheme_end = std::find(scheme_start, (host_start ? host_start : pe), ':');
                if (scheme_end - scheme_start > 0)
                {
                    url->setScheme(std::string(scheme_start, scheme_end));
                }
            }
            if (userinfo_start)
            {
                const char* userinfo_end = (host_start ? host_start - 1 : pe);
                if (userinfo_end - userinfo_start > 0)
                {
                    std::string userinfo(userinfo_start,userinfo_end);
                    url->setUserinfo(userinfo);
                    url->setDecodedUserinfo(URLUtils::UrlDecode(userinfo));
                }
            }
            if (host_start)
            {
                const char* host_end = port_start ? port_start - 1 : (path_start ? path_start : pe);
                std::string host_str;
                if (host_end - host_start > 0)
                {
                    host_str = std::string(host_start, host_end);  // 正确构造完整字符串
                }
    
                bool hasbrackets = false;
				// 去掉 IPv6 的方括号
				if (!host_str.empty() && host_str[0] == '[' && host_str.back() == ']')
                {
					host_str = host_str.substr(1, host_str.size() - 2);
                    hasbrackets = true;
				}

                // url->setHost(host_str);  // 设置host
                
                // 转换为二进制存储
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
                
                    // 转换为 IDN ASCII 形式
                    std::string ascii_host = URLUtils::DomainToASCII(host_str);
                    url->setHost(ascii_host);
                }
            }
            if (port_start)
            {
                const char* port_end = path_start ? path_start : pe;
                if (port_end - port_start > 0)
                {
                    std::string port_str(port_start, port_end);
                    url->setPort(port_str.empty() ? 0 : std::stoi(port_str));
                }
            }
            if (path_start)
            {
                const char* path_end = query_start ? query_start - 1 : (fragment_start ? fragment_start - 1 : pe);
                if (path_end - path_start > 0)
                {
                    std::string path(path_start, path_end);
                    url->setPath(path);
                    url->setDecodedPath(URLUtils::UrlDecode(path));
                }
            }
            if (query_start)
            {
                const char* query_end = fragment_start ? fragment_start - 1 : pe;
                if (query_end - query_start > 0)
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
            return url;
        }
        return nullptr;
    }

} // namespace blue