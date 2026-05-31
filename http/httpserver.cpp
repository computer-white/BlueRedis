#include <regex>
#include <chrono>
#include <fcntl.h>
#include "blue/log.h"
#include "blue/config.h"
#include "blue/dbmanager.h"
#include "blue/redismanager.h"
#include "httpserver.h"
#include "httpconnection.h"
#ifdef USE_GUMBO
#include <gumbo.h>
#endif

// http server
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
            
        // 全局连接池缓存
        static std::map<std::string, HttpConnectionPool::HttpConnectionPoolPtr> s_pools;
        static http::HttpConnectionPool::MmutexType s_poolMutex;

        extern std::string s_db_host;
        extern std::string s_db_user;
        extern std::string s_db_database;
        extern std::string s_db_passward;
        extern uint16_t s_db_port;
        extern blue::DbManager::DbManagerPtr s_dbmanager_ptr;

        extern std::string s_redis_host;
        extern uint16_t s_redis_port;
        extern std::string s_redis_passward;
        extern blue::RedisManager::RedisManagerPtr s_redismanager_ptr;

        extern uint64_t s_rate_limit;
        extern uint64_t s_rate_limit_expire;
        extern uint64_t s_cache_expire;

        extern uint64_t s_select_timeout;

        /*
            // 需要处理的标签和属性
            struct ResourceRewrite {
                GumboTag tag;
                const char* attribute;
            };

            ResourceRewrite rewrites[] = {
                {GUMBO_TAG_LINK,   "href"},      // CSS
                {GUMBO_TAG_SCRIPT, "src"},       // JavaScript
                {GUMBO_TAG_IMG,    "src"},       // 图片
                {GUMBO_TAG_IMG,    "srcset"},    // 响应式图片
                {GUMBO_TAG_SOURCE, "src"},       // <source> 标签
                {GUMBO_TAG_SOURCE, "srcset"},    // <source> 的 srcset
                {GUMBO_TAG_IFRAME, "src"},       // 内嵌框架
                {GUMBO_TAG_EMBED,  "src"},       // 嵌入内容
                {GUMBO_TAG_OBJECT, "data"},      // 对象数据
                {GUMBO_TAG_VIDEO,  "src"},       // 视频
                {GUMBO_TAG_AUDIO,  "src"},       // 音频
                {GUMBO_TAG_INPUT,  "src"},       // <input type="image">
            };


            ┌─────────────────────────────────────────────────────────┐
            │                    需要修改的内容                         │
            ├──────────────────┬──────────────────────────────────────┤
            │  HTML 元素/属性   │  示例                                │
            ├──────────────────┼──────────────────────────────────────┤
            │ <a href="">       │  链接跳转                            │
            │ <link href="">    │  CSS 样式表                          │
            │ <script src="">   │  JavaScript                         │
            │ <img src="">      │  图片                                │
            │ <img srcset="">   │  响应式图片                          │
            │ <source src="">   │  媒体资源                            │
            │ <iframe src="">   │  内嵌页面                            │
            │ <embed src="">    │  嵌入内容                            │
            │ <object data="">  │  对象数据                            │
            │ <video src="">    │  视频                                │
            │ <audio src="">    │  音频                                │
            │ <form action="">  │  表单提交                            │
            │ <input src="">    │  image 类型的 input                  │
            │ <base href="">    │  基础 URL（整个页面的相对路径基准）   │
            ├──────────────────┼──────────────────────────────────────┤
            │ CSS 中            │  url()、@import                      │
            │ 内联样式/脚本      │  style=""、onclick="" 中的 URL       │
            │ HTTP 响应头       │  Location、Refresh、CSP              │
            └──────────────────┴──────────────────────────────────────┘
        */

        // CSS URL 重写函数
        static std::string _process_css(const std::string& css,
                                        const std::string& target,
                                        const std::string& proxy_path) 
        {
            std::string result = css;
            
            // 重写 url("//xxx") 和 url(//xxx)
            std::regex url_re(R"(url\(\s*["']?(//[^"')]+)["']?\s*\))");
            std::sregex_iterator begin(css.begin(), css.end(), url_re);
            std::sregex_iterator end;
            
            size_t last_pos = 0;
            for (auto it = begin; it != end; ++it)
            {
                const std::smatch& match = *it;
                std::string url = match[1].str();
                
                // 跳过 data: 等
                if (url.find("data:") == 0)
                {
                    continue;
                }
                
                std::string new_url = rewrite_url(url, target, proxy_path);
                
                // 输出匹配之前的部分
                result.append(css, last_pos, match.position() - last_pos);
                // 输出替换后的 url()
                result += "url(\"" + new_url + "\")";
                
                last_pos = match.position() + match.length();
            }
            
            // 输出剩余部分
            result.append(css, last_pos, css.length() - last_pos);
            
            return result;
        }

        // 修改cookie的domain
        static std::string fix_cookie_domain(const std::string& set_cookie) 
        {
            std::string result = set_cookie;
            
            // 1. 删除 domain=xxx 部分
            std::regex domain_re(R"(;\s*domain=[^;]*)", std::regex::icase);
            result = std::regex_replace(result, domain_re, "");
            
            // 2. 可选：设置 domain=localhost
            result += "; domain=localhost";
            
            // 3. 删除 Secure 标记（因为你是 http，不是 https）
            std::regex secure_re(R"(;\s*secure\b)", std::regex::icase);
            result = std::regex_replace(result, secure_re, "");
            
            return result;
        }

        // URL 解析辅助函数：从 target 参数提取域名
        static std::string extract_host_from_target(const std::string &target)
        {
            // target 格式: http://www.baidu.com 或 https://example.com/path
            auto urlptr = blue::Url::CreateUrl(target);
            return urlptr->getHost();
        }
        
        // 路径规格化
        static std::string normalize_path(const std::string& path) 
        {
            std::vector<std::string> parts;
            std::stringstream ss(path);
            std::string part;
            
            while (std::getline(ss, part, '/')) 
            {
                if (part.empty() || part == ".") 
                {
                    continue;
                } 
                else if (part == "..") 
                {
                    if (!parts.empty()) parts.pop_back();
                } 
                else 
                {
                    parts.push_back(part);
                }
            }
            
            std::string result;
            for (auto& p : parts) 
            {
                result += "/" + p;
            }
            return result.empty() ? "/" : result;
        }

        // URL 重写函数
        static std::string rewrite_url(const std::string &url,
                                       const std::string &target,
                                       const std::string &proxy_path)
        {
            if (url.empty())
                return url;

            // 不处理特殊协议和锚点
            if (url.find("javascript:") == 0 ||
                url.find("data:") == 0 ||
                url.find("mailto:") == 0 ||
                url.find("tel:") == 0 ||
                url.find("#") == 0)
            {
                return url;
            }

            auto target_url = blue::Url::CreateUrl(target);
            if (!target_url)
                return url;

            // 情况1：绝对 URL（http:// 或 https://）
            if (url.find("http://") != std::string::npos 
            || url.find("https://") != std::string::npos)
            {
                return proxy_path + "/" + url;
            }

            // 情况2：协议相对 URL（//example.com/path）
            if (url.size() >= 2 && url[0] == '/' && url[1] == '/')
            {
                return proxy_path + "/" + target_url->getScheme() + ":" + url;
            }

            // 构造基础 URL（scheme + authority）
            std::string base = target_url->getScheme() + "://" + target_url->getAuthority();

            // 情况3：根相对路径（/path）
            if (url[0] == '/')
            {
                return proxy_path + "/" + base + url;
            }

            // 情况4：相对路径（需要拼接当前路径的目录部分）
            std::string current_path = target_url->getPath();
            // 拼接：base + 当前目录 + "/" + 相对路径
            std::string resolved_path = current_path + "/" + url;
            /*
                <a href="../home">     →    /home       →    /blue/http://www.baidu.com/home
                <a href="../../top">   →    /top        →    /blue/http://www.baidu.com/top
                <a href="./page">      →    /s/page     →    /blue/http://www.baidu.com/s/page
                <a href="images/a.png">→    /s/images/a.png → /blue/http://www.baidu.com/s/images/a.png
            */
            resolved_path = normalize_path(resolved_path);
            return proxy_path + "/" + base + resolved_path;
        }

        // HTML 属性值转义
        static std::string escape_html_attr(const std::string &value)
        {
            std::string result;
            result.reserve(value.size());
            for (char c : value)
            {
                switch (c)
                {
                case '&':
                    result += "&amp;";
                    break;
                case '"':
                    result += "&quot;";
                    break;
                case '<':
                    result += "&lt;";
                    break;
                case '>':
                    result += "&gt;";
                    break;
                default:
                    result += c;
                }
            }
            return result;
        }

        // HTML 文本转义
        static void escape_text(const char *text, std::stringstream &ss)
        {
            while (*text)
            {
                switch (*text)
                {
                case '&':
                    ss << "&amp;";
                    break;
                case '<':
                    ss << "&lt;";
                    break;
                case '>':
                    ss << "&gt;";
                    break;
                default:
                    ss << *text;
                }
                text++;
            }
        }

        // 判断是否是 void 元素（HTML5 规范）
        static bool is_void_element(GumboTag tag)
        {
            switch (tag)
            {
            case GUMBO_TAG_AREA:
            case GUMBO_TAG_BASE:
            case GUMBO_TAG_BR:
            case GUMBO_TAG_COL:
            case GUMBO_TAG_EMBED:
            case GUMBO_TAG_HR:
            case GUMBO_TAG_IMG:
            case GUMBO_TAG_INPUT:
            case GUMBO_TAG_LINK:
            case GUMBO_TAG_META:
            case GUMBO_TAG_PARAM:
            case GUMBO_TAG_SOURCE:
            case GUMBO_TAG_TRACK:
            case GUMBO_TAG_WBR:
                return true;
            default:
                return false;
            }
        }

        // 判断属性是否需要重写
        static bool is_rewritable_attribute(GumboTag tag, const std::string &attr_name)
        {
            switch (tag)
            {
            case GUMBO_TAG_A:
            case GUMBO_TAG_LINK:
            case GUMBO_TAG_BASE:
                return attr_name == "href";

            case GUMBO_TAG_IMG:
            case GUMBO_TAG_SCRIPT:
            case GUMBO_TAG_IFRAME:
            case GUMBO_TAG_EMBED:
            case GUMBO_TAG_VIDEO:
            case GUMBO_TAG_AUDIO:
            case GUMBO_TAG_INPUT: // <input type="image" src="...">
                return attr_name == "src";

            case GUMBO_TAG_FORM:
                return attr_name == "action";

            case GUMBO_TAG_OBJECT:
                return attr_name == "data";

            case GUMBO_TAG_SOURCE:
                return attr_name == "src" || attr_name == "srcset";

            default:
                return false;
            }
        }

        // 处理 srcset 属性（可能包含多个 URL 和描述符）
        static std::string rewrite_srcset(const std::string &srcset,
                                          const std::string &target,
                                          const std::string &proxy_path)
        {
            std::stringstream result;
            size_t pos = 0;
            const size_t len = srcset.length();

            while (pos < len)
            {
                // 跳过空白
                while (pos < len && (srcset[pos] == ' ' || srcset[pos] == '\t'))
                {
                    result << srcset[pos];
                    pos++;
                }

                if (pos >= len)
                    break;

                // 找到 URL 结束位置（空白或逗号）
                size_t url_start = pos;
                while (pos < len && srcset[pos] != ',' && srcset[pos] != ' ' && srcset[pos] != '\t')
                {
                    pos++;
                }

                if (pos > url_start)
                {
                    std::string url_part = srcset.substr(url_start, pos - url_start);
                    result << rewrite_url(url_part, target, proxy_path);
                }

                // 输出空白和描述符（如 "480w"）
                while (pos < len && srcset[pos] != ',')
                {
                    result << srcset[pos];
                    pos++;
                }

                // 输出逗号
                if (pos < len && srcset[pos] == ',')
                {
                    result << ',';
                    pos++;
                }
            }

            return result.str();
        }

        // 序列化属性（并重写需要的 URL）
        static void serialize_attributes(const GumboVector *attributes,
                                         std::stringstream &ss,
                                         const std::string &target,
                                         const std::string &proxy_path,
                                         GumboTag tag)
        {
            for (unsigned int i = 0; i < attributes->length; i++)
            {
                auto *attr = static_cast<GumboAttribute *>(attributes->data[i]);
                std::string attr_name(attr->name);
                std::string attr_value(attr->value);

                // 判断是否需要重写
                if (is_rewritable_attribute(tag, attr_name))
                {
                    BLUE_LOG_WARN(g_logger) << "Rewriting: tag=" << (int)tag 
                            << " attr=" << attr_name 
                            << " value=" << attr_value;
                    if (attr_name == "srcset")
                    {
                        attr_value = rewrite_srcset(attr_value, target, proxy_path);
                    }
                    else
                    {
                        attr_value = rewrite_url(attr_value, target, proxy_path);
                    }
                }

                ss << " " << attr->name << "=\"" << escape_html_attr(attr_value) << "\"";
            }
        }

        // 递归序列化 DOM 节点
        static void serialize_node(GumboNode *node,
                                   std::stringstream &ss,
                                   const std::string &target,
                                   const std::string &proxy_path,
                                   bool base_injected = false)
        {
            if (!node)
                return;

            switch (node->type)
            {
            case GUMBO_NODE_DOCUMENT:
            {
                const GumboVector *children = &node->v.document.children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serialize_node(static_cast<GumboNode *>(children->data[i]),
                                   ss, target, proxy_path, base_injected);
                }
                break;
            }

            case GUMBO_NODE_ELEMENT:
            {
                GumboElement *element = &node->v.element;
                const char *tag_name = gumbo_normalized_tagname(element->tag);

                if (element->tag == GUMBO_TAG_META)
                {
                    GumboAttribute* http_equiv = gumbo_get_attribute(&element->attributes, "http-equiv");
                    if (http_equiv && strcasecmp(http_equiv->value, "Content-Security-Policy") == 0)
                    {
                        // 跳过这个 meta 标签，不输出
                        break;  // 直接跳出，不序列化
                    }
                }

                ss << "<" << tag_name;
                serialize_attributes(&element->attributes, ss, target, proxy_path, element->tag);

                // 在 <head> 内注入 <base> 和 JS 拦截器（只注入一次）
                if (element->tag == GUMBO_TAG_HEAD && !base_injected)
                {
                    ss << ">";
                    ss << "<base href=\"" << proxy_path << "/" << target << "\">";
                    
                    auto target_url = blue::Url::CreateUrl(target);
                    if (target_url)
                    {
                        std::string authority = target_url->getScheme() + "://" + target_url->getAuthority();
                        
                        ss << "<script>"
                        << "(function(){"
                        << "var p='" << proxy_path << "/" << authority << "';"
                        << "var origOpen=XMLHttpRequest.prototype.open;"
                        << "XMLHttpRequest.prototype.open=function(m,u){"
                        << "if(u.indexOf('/')===0&&u.indexOf('" << proxy_path << "/')!==0)u=p+u;"
                        << "origOpen.call(this,m,u);"
                        << "};"
                        << "var origFetch=window.fetch;"
                        << "window.fetch=function(u,o){"
                        << "if(typeof u==='string'&&u.indexOf('/')===0&&u.indexOf('" << proxy_path << "/')!==0)u=p+u;"
                        << "return origFetch.call(this,u,o);"
                        << "};"
                        << "})();"
                        << "</script>";
                    }
                    
                    base_injected = true;

                    const GumboVector *children = &element->children;
                    for (unsigned int i = 0; i < children->length; i++)
                    {
                        serialize_node(static_cast<GumboNode *>(children->data[i]),
                                    ss, target, proxy_path, base_injected);
                    }

                    ss << "</head>";
                    return;
                }

                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serialize_node(static_cast<GumboNode *>(children->data[i]),
                                ss, target, proxy_path, base_injected);
                }

                if (!is_void_element(element->tag))
                {
                    ss << "</" << tag_name << ">";
                }
                break;
            }

            case GUMBO_NODE_TEXT:
                escape_text(node->v.text.text, ss);
                break;

            case GUMBO_NODE_CDATA:
                ss << "<![CDATA[" << node->v.text.text << "]]>";
                break;

            case GUMBO_NODE_COMMENT:
                ss << "<!--" << node->v.text.text << "-->";
                break;

            case GUMBO_NODE_WHITESPACE:
                ss << node->v.text.text;
                break;

            case GUMBO_NODE_TEMPLATE:
            {
                GumboElement *element = &node->v.element;
                ss << "<template";
                serialize_attributes(&element->attributes, ss, target, proxy_path, GUMBO_TAG_TEMPLATE);
                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serialize_node(static_cast<GumboNode *>(children->data[i]),
                                   ss, target, proxy_path, base_injected);
                }

                ss << "</template>";
                break;
            }
            }
        }

        // 解析主函数
        static std::string _process_html(const std::string &html,
                                         const std::string &target,
                                         const std::string &proxy_path)
        {
            GumboOutput *output = gumbo_parse(html.c_str());
            if (!output)
            {
                return html; // 解析失败，返回原文
            }

            std::stringstream ss;
            serialize_node(output->root, ss, target, proxy_path);

            gumbo_destroy_output(&kGumboDefaultOptions, output);

            return ss.str();
        }

        template <typename T>
        HttpServer<T>::HttpServer(bool keepAlive, int level, int option_name, T option, IOManager *manager, IOManager *acceptmanager)
            : TcpServer<T>(level, option_name, option, manager, acceptmanager),
              m_keepAlive(keepAlive)
        {
            m_dispatch.reset(new ServletDispatch());
        }

        template <typename T>
        void HttpServer<T>::handleClient(MSocket::MSocketPtr sock)
        {
            auto remoteAddress = std::dynamic_pointer_cast<IPAddress>(sock->getRemoteAddress());
            auto localAddress = std::dynamic_pointer_cast<IPAddress>(sock->getLocalAddress());
            _setIpAndPort(remoteAddress, m_remoteIP, m_remotePort);
            _setIpAndPort(localAddress, m_localIp, m_localPort);
            
            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << remoteAddress->toString() << " ip : " << m_remoteIP << " port : " << m_remotePort;
            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << localAddress->toString()  << " ip : " << m_localIp  << " port : " << m_localPort;

            // ===== 检测 TLS，如果是就替换为 SSLSocket =====
            char first_byte;
            int peek_ret = ::recv(sock->getSocketfd(), &first_byte, 1, MSG_PEEK | MSG_DONTWAIT);
            
            std::shared_ptr<HttpSession> session;
            if (peek_ret == 1 && first_byte == 0x16) 
            {
                // HTTPS：创建 SSLSocket，握手，然后直接当 SocketStream 用
                auto ssl_sock = std::make_shared<SSLSocket>(sock,true,true);
                if (!ssl_sock->isValid()) 
                {
                    BLUE_LOG_ERROR(g_logger) << "SSLSocket creation failed (cert not found?)";
                    sock->close();
                    return;
                }
                if (!ssl_sock->handshake()) 
                {
                    BLUE_LOG_ERROR(g_logger) << "SSL handshake failed";
                    sock->close();
                    return;
                }
                session = std::make_shared<HttpSession>(ssl_sock);
            } else 
            {
                auto stream = std::make_shared<SocketStream>(sock);
                session = std::make_shared<HttpSession>(stream);
            }

            bool temkeepAlive = m_keepAlive;
            do
            {
                // 若双方有一个是长连接就长连接
                // 错误由recvRequest处理
                auto [recvstatus, requestPtr] = session->recvRequest();
                if (recvstatus == http::HttpSession::RecvStatus::ERROR ||
                    recvstatus == http::HttpSession::RecvStatus::CLOSE)
                {
                    session->close();
                    return;
                }
                auto responsePtr = std::make_shared<HttpResponse>(requestPtr->getVersion(), (requestPtr->isKeepAlive() || temkeepAlive));
                temkeepAlive = (requestPtr->isKeepAlive() || temkeepAlive);

                std::string path = requestPtr->getPath();
                std::string host = requestPtr->getHeader("Host");
                BLUE_LOG_INFO(g_logger) << "path : " << path << " host : " << host;
                std::string targeturl;
                std::string target_param = requestPtr->getParam("target", "");

                if (requestPtr->getMethod() == HttpMethod::CONNECT)
                {
                    BLUE_LOG_INFO(g_logger) << "CONNECT: " << requestPtr->getPath();
                    _handleConnect(sock, requestPtr);
                    return;  // CONNECT
                }

                if (path == "/proxy.pac")
                {
                    // FindProxyForURL 浏览器每次请求调用
                    std::string pac = R"(function FindProxyForURL(url, host) {
                        // 本地地址直连,isPlainHostName判断是否是本机主机名
                        if (isPlainHostName(host) || host == "127.0.0.1" || host == "localhost")
                            return "DIRECT";
                        // 百度相关走代理
                        if (shExpMatch(host, "*.baidu.com") || shExpMatch(host, "*.bdstatic.com"))
                            return "PROXY localhost:8020";
                        // websocket
                        if (url.startsWith("ws://") || url.startsWith("wss://"))
                            return "PROXY localhost:8020";
                        // 其他直连
                        return "DIRECT";
                    })";
                    responsePtr->setBody(pac);
                    responsePtr->setHeader("Content-Type","application/x-ns-proxy-autoconfig");
                    responsePtr->setHeader("Content-Length",std::to_string(pac.size()));
                    responsePtr->setStatus(HttpStatus::OK);
                }
                else if (path.find("/admin/") == 0 || path == "/admin")
                {
                    _handleAdmin(requestPtr, responsePtr, session);
                }
                else if (!target_param.empty())
                {
                    targeturl = target_param;
                    // 整个path带有/blue...
                    std::string extra = path;
                    size_t blue_pos = extra.find("/blue");
                    if (blue_pos != std::string::npos)
                    {
                        extra = extra.substr(blue_pos + strlen("/blue"));
                    }
                    if (!extra.empty() && extra != "/")
                    {
                        auto u = blue::Url::CreateUrl(targeturl);
                        if (u)
                        {
                            targeturl = u->getScheme() + "://" + u->getAuthority() + extra;
                            std::string q = u->getQuery();
                            if (!q.empty()) targeturl += "?" + q;
                        }
                    }
                    _forwardRequest(requestPtr, responsePtr, targeturl, false);
                }

                // ===== 正向代理 =====
                // 正向代理：Host 不是 localhost，path 就是目标路径
                else if (!host.empty() && 
                        host.find("localhost") == std::string::npos && 
                        host.find("127.0.0.1") == std::string::npos)
                {
                    // websocket
                    std::string upgrade = requestPtr->getHeader("Upgrade");
                    if (strcasecmp(upgrade.c_str(), "websocket") == 0)
                    {
                        BLUE_LOG_INFO(g_logger) << "WebSocket upgrade: " << requestPtr->getPath();
                        if (path.find("http://") == 0 || path.find("https://") == 0)
                        {
                            targeturl = path;
                        }
                        else
                        {
                            targeturl = "http://" + host + path;
                            std::string query = requestPtr->getQuery();
                            if (!query.empty()) targeturl += "?" + query;
                        }
                        _handleWebSocket(sock, requestPtr, responsePtr, targeturl);
                        return;
                    }
                    // 正常正向代理
                    if (path.find("http://") == 0 || path.find("https://") == 0)
                    {
                        targeturl = path;
                    }
                    else
                    {
                        targeturl = "http://" + host + path;
                        std::string query = requestPtr->getQuery();
                        if (!query.empty()) targeturl += "?" + query;
                    }
                    _forwardRequest(requestPtr, responsePtr, targeturl, true);
                }

                // ===== 反向代理（路径前缀模式）=====
                else if (path.find("/blue/") == 0)
                {
                    size_t scheme_pos = path.find("http://");
                    if (scheme_pos == std::string::npos)
                        scheme_pos = path.find("https://");
                    if (scheme_pos != std::string::npos)
                    {
                        // /blue/xxx/https://www.baidu.com/news
                        //              ↑ scheme_pos
                        targeturl = path.substr(scheme_pos);  // "https://www.baidu.com/news"
                        
                        // 提取中间路径：/blue 和 scheme 之间的部分
                        std::string middle = path.substr(strlen("/blue"), scheme_pos - strlen("/blue"));
                        BLUE_LOG_INFO(g_logger) << "middle : " << middle;
                        // middle = "/xxx"
                        // 如果 middle 不为空，拼到 target URL 的 path 上
                        if (!middle.empty() && middle != "/")
                        {
                            // 去掉 middle 尾部斜杠
                            while (!middle.empty() && middle.back() == '/')
                                middle.pop_back();
                            // 去掉 middle 首部斜杠
                            if (!middle.empty() && middle.front() == '/')
                                middle.erase(0, 1);
                            
                            auto u = blue::Url::CreateUrl(targeturl);
                            if (u)
                            {
                                // 用 middle 作为实际路径
                                targeturl = u->getScheme() + "://" + u->getAuthority() + "/" + middle;
                                std::string q = u->getQuery();
                                if (!q.empty()) targeturl += "?" + q;
                            }
                        }
                        _forwardRequest(requestPtr, responsePtr, targeturl, false);
                    }
                    else
                    {
                        m_dispatch->handle(requestPtr, responsePtr, session);
                    }
                }
                else
                {
                    m_dispatch->handle(requestPtr, responsePtr, session);
                }
                // 自动补全缺失的响应头
                if (responsePtr->getHeader("Content-Type").empty())
                {
                    responsePtr->setHeader("Content-Type", "text/html; charset=utf-8");
                }
                if (responsePtr->getHeader("Content-Length").empty())
                {
                    responsePtr->setHeader("Content-Length", std::to_string(responsePtr->getBody().size()));
                }
                session->sendResponse(responsePtr, requestPtr);
                BLUE_LOG_INFO(g_logger) << sock->getRemoteAddress()->toString()
                                        << " \"" << http::HttpMethodToChars(requestPtr->getMethod())
                                        << " " << requestPtr->getPath()
                                        << " HTTP/" << requestPtr->versionToStr() << "\" "
                                        << http::HttpStatusToChars(responsePtr->getStatus())
                                        << " " << responsePtr->getBody().size() << "B "
                                        << requestPtr->getHeader("User-Agent");
            } while (temkeepAlive);
            session->close();
            return;
        }

        template <typename T>
        void HttpServer<T>::_setIpAndPort(blue::IPAddress::IPAddressPtr address, std::string &ip, std::string &port)
        {
            if (address->getFamily() == AF_INET)
            {
                ip = std::dynamic_pointer_cast<IPv4Address>(address)->getIp();
            }
            else if (address->getFamily() == AF_INET6)
            {
                ip = std::dynamic_pointer_cast<IPv6Address>(address)->getIp();
            }
            else
            {
                ip = std::dynamic_pointer_cast<UnknowAddress>(address)->getIp();
            }
            port = std::to_string(address->getPort());
        }

        template <typename T>
        void HttpServer<T>::_forwardRequest(HttpRequest::HttpRequestPtr request,
                                            HttpResponse::HttpResponsePtr response, 
                                            const std::string &targeturl,
                                            bool isForwardProxy)
        {
            // ===== Redis 缓存（正向代理 GET 请求）=====
            std::string cache_key;
            bool use_cache = false;
            if (isForwardProxy && (request->getMethod() == HttpMethod::GET && s_redismanager_ptr))
            {
                use_cache = true;
                cache_key = "cache:" + targeturl;
                std::string cached = s_redismanager_ptr->get(cache_key);
                if (!cached.empty())
                {
                    response->setBody(cached);
                    response->setHeader("Content-Length", std::to_string(cached.size()));
                    response->setHeader("X-Cache", "HIT");
                    response->setStatus(blue::http::HttpStatus::OK);
                    return;
                }
            }
            // ===== Redis 缓存（反向代理 GET 请求）=====
            if (!isForwardProxy && (request->getMethod() == HttpMethod::GET && s_redismanager_ptr))
            {
                use_cache = true;
                cache_key = "rcache:" + targeturl;
                std::string cached = s_redismanager_ptr->get(cache_key);
                if (!cached.empty())
                {
                    response->setBody(cached);
                    response->setHeader("Content-Length",std::to_string(cached.size()));
                    response->setHeader("X-Cache","HIT");
                    response->setStatus(blue::http::HttpStatus::OK);
                    return;
                }
            }
            // 对于同一个客户端请求进行限流
            if (s_redismanager_ptr)
            {
                std::string key = "rate:" + m_remoteIP;
                long long count = s_redismanager_ptr->incr(key);
                if (count == 1)
                {
                    // 设置60秒窗口
                    s_redismanager_ptr->expire(key, s_rate_limit_expire);  // 窗口
                }
                if (count > s_rate_limit)  // 每分钟最多rate_limit个请求
                {
                    response->setStatus(blue::http::HttpStatus::TOO_MANY_REQUESTS);
                    response->setBody("Rate limit exceeded");
                    return;
                }
            }
            // 设置下游(client)ip(remoteip)
            std::string xxf = request->getHeader("X-Forwarded-For");
            if (!xxf.empty())
            {
                xxf += ", ";
            }
            xxf += m_remoteIP;
            request->setHeader("X-Forwarded-For", xxf);
            auto UrlPtr = blue::Url::CreateUrl(targeturl);
            if (!UrlPtr)
            {
                response->setStatus(blue::http::HttpStatus::BAD_REQUEST);
                response->setBody("invalid target url");
                return;
            }

            // 代理前缀
            std::string proxy_path = "/blue";

            // 将客户端的请求中的header拿出来作为我们发给targeturl的header
            std::map<std::string,std::string> headers;
            _setHeaders(request,headers);

            std::string poolKey = UrlPtr->getScheme() + "://" + UrlPtr->getHost() + ":" + std::to_string(UrlPtr->getPort());

            HttpConnectionPool::HttpConnectionPoolPtr pool;
            {
                blue::http::HttpConnectionPool::MmutexType::lockSco lock(s_poolMutex);
                auto it = s_pools.find(poolKey);
                if (it == s_pools.end())
                {
                    pool = std::make_shared<HttpConnectionPool>(
                        UrlPtr->getHost(), "", UrlPtr->getPort(), 60000, 100, 10);
                    s_pools[poolKey] = pool;
                }
                else
                {
                    pool = it->second;
                }
            }

            // 转发
            auto now = std::chrono::steady_clock::now();
            // 用连接池发请求（复用连接）
            auto result = pool->doRequest(request->getMethod(), UrlPtr, 5000, headers, request->getBody());
            auto end = std::chrono::steady_clock::now();
            int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();

            // 记录日志
            if (s_dbmanager_ptr)
            {
                std::string method_str = http::HttpMethodToChars(request->getMethod());
                int status_code = 0;
                int body_size = 0;
                std::string error_msg;
                
                if (result->response)
                {
                    status_code = (int)result->response->getStatus();
                    body_size = result->response->getBody().size();
                }
                if (!result->error.empty())
                {
                    error_msg = result->error;
                }
                
                s_dbmanager_ptr->logRequest(m_remoteIP, 
                                method_str,
                                targeturl, UrlPtr->getHost(),
                                status_code, body_size,
                                request->getHeader("User-Agent"),
                                duration, isForwardProxy, false, error_msg);
            }
            if (result->result == (int)blue::http::HttpResult::ResultStatus::OK && result->response)
            {
                for (auto &header : result->response->getHeaders())
                {
                    response->setHeader(header.first, header.second);
                }
                response->delHeader("Transfer-Encoding"); // 和 Content-Length 互斥
                int status = (int)result->response->getStatus();

                // ===== 正向代理：透传 + 存缓存 =====
                if (isForwardProxy)
                {
                    response->setBody(result->response->getBody());
                    response->setHeader("Content-Length", std::to_string(result->response->getBody().size()));
                    response->setStatus((blue::http::HttpStatus)status);
                    // 缓存响应（60 秒）
                    if (use_cache && status == 200)
                    {
                        s_redismanager_ptr->set(cache_key, result->response->getBody(), s_cache_expire);
                        response->setHeader("X-Cache", "MISS");
                    }
                    return;
                }

                // ===== 反向代理：重写 HTML/CSS =====
                if (status == 301 || status == 302 || status == 307 || status == 308)
                {
                    std::string location = result->response->getHeader("Location");
                    if (!location.empty())
                    {
                        // 重定向到targeturl成循环不允许
                        // TODO
                        if (location == targeturl || location == targeturl + "/") 
                        {
                            BLUE_LOG_WARN(g_logger) << "Redirect loop detected: " << location;
                            std::string err = "Redirect loop detected";
                            response->setStatus(blue::http::HttpStatus::BAD_GATEWAY);
                            response->setBody(err);
                            response->setHeader("Content-Length",std::to_string(err.size()));
                            if (use_cache)
                            {
                                s_redismanager_ptr->set(cache_key,err,s_cache_expire);
                            }
                            return;
                        }

                        // 把重定向 URL 也改成代理模式
                        std::string new_location = proxy_path + "/" + location;
                        response->setHeader("Location", new_location);
                        response->delHeader("Content-Length"); // 重定向没有 body
                        response->setBody("");                 // 清空 body
                        response->setStatus((blue::http::HttpStatus)status);
                        if (use_cache)
                        {
                            s_redismanager_ptr->set(cache_key,"",s_cache_expire);
                        }
                        return;
                    }
                }

                std::string set_cookie = result->response->getHeader("Set-Cookie");
                if (!set_cookie.empty())
                {
                    std::string fixed_cookie = fix_cookie_domain(set_cookie);
                    response->setHeader("Set-Cookie", fixed_cookie);
                }

                std::string ContentType = result->response->getHeader("Content-Type");
                if (ContentType.find("text/html") != std::string::npos)
                {
                    auto original_html = result->response->getBody();
                    auto modified_body = _process_html(original_html, targeturl, proxy_path);
                    response->setBody(modified_body);
                    response->setStatus((blue::http::HttpStatus)status);
                    // 设置的修改后未压缩大小,稍后在sendresponse时会设置压缩后的length
                    response->setHeader("Content-Length", std::to_string(modified_body.size()));
                }
                else if (ContentType.find("text/css") != std::string::npos)
                {
                    auto original_css = result->response->getBody();
                    auto modified_css = _process_css(original_css, targeturl, proxy_path);
                    response->setBody(modified_css);
                    response->setStatus((blue::http::HttpStatus)status);
                    // 设置的修改后未压缩大小,稍后在sendresponse时会设置压缩后的length
                    response->setHeader("Content-Length", std::to_string(modified_css.size()));
                }
                else
                {
                    response->setBody(result->response->getBody());
                    response->setHeader("Content-Length", std::to_string(result->response->getBody().size()));
                    response->setStatus((blue::http::HttpStatus)status);
                }
                if (!ContentType.empty())
                {
                    response->setHeader("Content-Type", ContentType);
                }
                if (use_cache)
                {
                    s_redismanager_ptr->set(cache_key,response->getBody(),s_cache_expire);
                }
            }
            else
            {
                response->setStatus(blue::http::HttpStatus::BAD_GATEWAY);
                response->setBody("forward failed: " + result->error);
            }
        }

        template <typename T>
        void HttpServer<T>::_handleAdmin(HttpRequest::HttpRequestPtr request, 
                        HttpResponse::HttpResponsePtr response, 
                        HttpSession::HttpSessionPtr session)
        {
            std::string sub_path = request->getPath().substr(strlen("/admin"));
            if (sub_path.empty() || sub_path == "/")
                sub_path = "/index";
            std::string body;
            if (sub_path == "/api/stats")
            {
                body = "{\"labels\":[],\"values\":[]}";
                if (s_dbmanager_ptr)
                {
                    auto res = s_dbmanager_ptr->query(
                        "SELECT DATE_FORMAT(created_at,'%H:%i') as minute, COUNT(*) as cnt "
                        "FROM request_logs WHERE created_at > DATE_SUB(NOW(), INTERVAL 60 MINUTE) "
                        "GROUP BY minute ORDER BY minute");
                    if (res)
                    {
                        std::string labels = "[", values = "[";
                        MYSQL_ROW row;
                        bool first = true;
                        while ((row = mysql_fetch_row(res)))
                        {
                            if (!first) { labels += ","; values += ","; }
                            labels += "\"" + std::string(row[0]) + "\"";
                            values += std::string(row[1]);
                            first = false;
                        }
                        labels += "]"; values += "]";
                        body = "{\"labels\":" + labels + ",\"values\":" + values + "}";
                        mysql_free_result(res);
                    }
                }
                response->setBody(body);
                response->setHeader("Content-Type", "application/json");
                response->setHeader("Content-Length", std::to_string(body.size()));
                response->setStatus(blue::http::HttpStatus::OK);
                return;
            }
            else if (sub_path == "/index")
            {
                body = R"(<!DOCTYPE html>
                    <html><head><meta charset="UTF-8" http-equiv="refresh" content="5">><title>Blue Proxy</title>
                    <style>
                    *{margin:0;padding:0;box-sizing:border-box}
                    body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                    h1{color:#1677ff;margin-bottom:20px}
                    .card{background:#16213e;border-radius:8px;padding:20px;margin-bottom:20px}
                    .card h2{color:#e94560;margin-bottom:10px;font-size:16px}
                    .stat{display:inline-block;margin:10px 20px 10px 0}
                    .stat .val{font-size:24px;color:#1677ff}
                    .stat .label{font-size:12px;color:#888}
                    table{width:100%;border-collapse:collapse;margin-top:10px}
                    th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                    td{padding:8px;border-bottom:1px solid #333;font-size:13px}
                    tr:hover{background:#0f3460}
                    a{color:#1677ff;text-decoration:none}
                    .menu{display:flex;gap:20px;margin-bottom:20px}
                    .menu a{padding:8px 16px;background:#16213e;border-radius:4px}
                    .menu a:hover{background:#0f3460}
                    </style></head><body>
                    <h1>🔵 Blue Proxy</h1>
                    <div class="menu">
                    <a href="/admin/index">Dashboard</a>
                    <a href="/admin/logs">Request Logs</a>
                    <a href="/admin/pools">Pool Stats</a>
                    <a href="/admin/config">Config</a>
                    </div>
                    <div class="card">
                    <h2>Server Info
                    <div class="card">
                    <h2>📊 Requests Per Minute</h2>
                    <canvas id="chart" width="800" height="300"></canvas>
                    </div>
                    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
                    <script>
                    fetch('/admin/api/stats')
                    .then(r => r.json())
                    .then(data => {
                        new Chart(document.getElementById('chart'), {
                        type: 'line',
                        data: {
                            labels: data.labels,
                            datasets: [{
                            label: 'Requests/min',
                            data: data.values,
                            borderColor: '#1677ff',
                            backgroundColor: 'rgba(22,119,255,0.1)',
                            tension: 0.3
                            }]
                        },
                        options: {
                            responsive: true,
                            plugins: { legend: { labels: { color: '#e0e0e0' } } },
                            scales: {
                            x: { ticks: { color: '#888' }, grid: { color: '#333' } },
                            y: { ticks: { color: '#888' }, grid: { color: '#333' }, beginAtZero: true }
                            }
                        }
                        });
                    });
                    </script>
                    </h2>
                    <div class="stat"><div class="val">)" + std::to_string(blue::Fiber::TotalFibers()) + R"(</div><div class="label">Fibers</div></div>
                    <div class="stat"><div class="val">)" + std::to_string(s_pools.size()) + R"(</div><div class="label">Connection Pools</div></div>
                    </div>
                    </body></html>)";
            }
            else if (sub_path == "/logs")
            {
                std::string search = request->getParam("search", "");
                body = R"(<html><head><meta charset='UTF-8'><title>Logs</title>
            <style>body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
            h1{color:#1677ff}.card{background:#16213e;border-radius:8px;padding:20px;margin:20px 0}
            table{width:100%;border-collapse:collapse;margin-top:10px}
            th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
            td{padding:8px;border-bottom:1px solid #333;font-size:13px;max-width:400px;overflow:hidden}
            tr:hover{background:#0f3460}
            a{color:#1677ff;text-decoration:none}
            input{padding:8px;border:1px solid #333;background:#1a1a2e;color:#e0e0e0;border-radius:4px;width:300px}
            button{padding:8px 16px;background:#1677ff;color:#fff;border:none;border-radius:4px;cursor:pointer}
            </style></head><body>
            <h1>📋 Request Logs</h1>
            <a href='/admin/index'>← Back</a>
            <form style='margin:20px 0'>
            <input name='search' value=')" + search + R"(' placeholder='Search by URL or IP...'>
            <button type='submit'>Search</button>
            </form>)";

                if (s_dbmanager_ptr)
                {
                    int page = atoi(request->getParam("page", "1").c_str());
                    if (page < 1) page = 1;
                    int limit = 20;
                    int offset = (page - 1) * limit;
                    std::string sql = "SELECT id,client_ip,target_url,status_code,duration_ms,created_at FROM request_logs ";
                    if (!search.empty())
                    {
                        sql += "WHERE target_url LIKE '%" + s_dbmanager_ptr->escape(search) 
                            + "%' OR client_ip LIKE '%" + s_dbmanager_ptr->escape(search) + "%' ";
                    }
                    sql += "ORDER BY id DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);
                    
                    auto res = s_dbmanager_ptr->query(sql);
                    if (res)
                    {
                        body += "<table><tr><th>ID</th><th>IP</th><th>Target</th><th>Status</th><th>Time</th><th>Date</th></tr>";
                        MYSQL_ROW row;
                        while ((row = mysql_fetch_row(res)))
                        {
                            body += "<tr><td>" + std::string(row[0] ? row[0] : "-") +
                                    "</td><td>" + std::string(row[1] ? row[1] : "-") +
                                    "</td><td style='max-width:300px;overflow:hidden'>" + std::string(row[2] ? row[2] : "-") +
                                    "</td><td>" + std::string(row[3] ? row[3] : "-") +
                                    "</td><td>" + std::string(row[4] ? row[4] : "-") + "ms" +
                                    "</td><td>" + std::string(row[5] ? row[5] : "-") + "</td></tr>";
                        }
                        body += "</table>";
                        mysql_free_result(res);
                    }
                    body += "<div style='margin-top:20px'>";
                    body += "<a href='/admin/logs?page=" + std::to_string(page > 1 ? page - 1 : 1) + "'>← Prev</a> ";
                    body += " Page " + std::to_string(page) + " ";
                    body += "<a href='/admin/logs?page=" + std::to_string(page + 1) + "'>Next →</a>";
                    body += "</div>";
                }
                body += "</body></html>";
            }
            else if (sub_path == "/pools")
            {
                body = R"(<html><head><meta charset='UTF-8'><title>Pool Stats</title>
                        <style>body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                        h1{color:#1677ff}.card{background:#16213e;border-radius:8px;padding:20px;margin:20px 0}
                        table{width:100%;border-collapse:collapse}th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                        td{padding:8px;border-bottom:1px solid #333}a{color:#1677ff}</style></head><body>
                        <h1>🔗 Connection Pools</h1><a href='/admin/index'>← Back</a><div class='card'><table>
                        <tr><th>Pool Key</th><th>Total</th><th>Idle</th></tr>)";

                for (auto& [key, pool] : s_pools)
                {
                    body += "<tr><td>" + key + "</td><td>" + std::to_string(pool->getTotalCounts()) +
                            "</td><td>" + std::to_string(pool->getIdleCounts()) + "</td></tr>";
                }
                body += "</table></div></body></html>";
            }
            else if (sub_path == "/config")
            {
                body = R"(<html><head><meta charset='UTF-8'><title>Config</title>
                        <style>body{font-family:monospace;background:#1a1a2e;color:#e0e0e0;padding:20px}
                        h1{color:#1677ff}.card{background:#16213e;border-radius:8px;padding:20px;margin:20px 0}
                        table{width:100%;border-collapse:collapse}th{text-align:left;padding:8px;border-bottom:2px solid #e94560;color:#e94560}
                        td{padding:8px;border-bottom:1px solid #333}a{color:#1677ff}</style></head><body>
                        <h1>⚙️ Configuration</h1><a href='/admin/index'>← Back</a><div class='card'><table>
                        <tr><th>Key</th><th>Value</th><th>Description</th></tr>
                        <tr><td>proxy.rate_limit</td><td>)" + std::to_string(s_rate_limit) + R"(</td><td>Rate limit per minute</td></tr>
                        <tr><td>proxy.cache_ttl</td><td>)" + std::to_string(s_cache_expire) + R"(</td><td>Cache TTL (seconds)</td></tr>
                        <tr><td>db.host</td><td>)" + s_db_host + R"(</td><td>Database host</td></tr>
                        <tr><td>db.database</td><td>)" + s_db_database+ R"(</td><td>Database name</td></tr>
                        <tr><td>redis.host</td><td>)" + s_redis_host + R"(</td><td>Redis host</td></tr>
                        </table></div></body></html>)";
            
            }
            
            response->setBody(body);
            response->setHeader("Content-Type", "text/html; charset=utf-8");
            response->setHeader("Content-Length", std::to_string(body.size()));
            response->setStatus(blue::http::HttpStatus::OK);
        }

        template <typename T>
        void HttpServer<T>::_handleConnect(MSocket::MSocketPtr sock,
                                            HttpRequest::HttpRequestPtr request)
        {
            // CONNECT 的 path 是 host:port
            std::string host_port = request->getHeader("Host");
            if (host_port.empty())
            {
                host_port = request->getPath();
            }
            
            // 解析 host 和 port
            std::string host = host_port;
            uint16_t port = 443;
            size_t colon = host_port.find(':');
            if (colon != std::string::npos)
            {
                host = host_port.substr(0, colon);
                port = std::stoi(host_port.substr(colon + 1));
            }
            
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel to " << host << ":" << port;

            // 连接目标站
            auto addr = blue::Address::LookupAnyIpAddress(host);
            if (!addr) return;
            addr->setPort(port);
            
            auto remote_sock = blue::MSocket::CreateTcp(addr);
            if (!remote_sock->connect(addr)) return;
            
            int client_fd = sock->getSocketfd();
            int remote_fd = remote_sock->getSocketfd();


            // 返回 200 给浏览器，表示隧道建立
            std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
            ::send(client_fd, ok.c_str(), ok.size(), 0);
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel established";
            
            char buf[16384];
            fd_set fds;
            
            while (true)
            {
                FD_ZERO(&fds);
                FD_SET(client_fd, &fds);
                FD_SET(remote_fd, &fds);
                
                int max_fd = std::max(client_fd, remote_fd);
                struct timeval tv = {0, (long)s_select_timeout * 1000};
                
                int ret = select(max_fd + 1, &fds, nullptr, nullptr, &tv);
                if (ret < 0)
                {
                    BLUE_LOG_WARN(g_logger) << "CONNECT select timeout or error: " << ret;
                    break;
                }
                else if (ret == 0)
                {
                    blue::Fiber::YieldToHold();
                }
                else
                {
                    if (FD_ISSET(client_fd, &fds))
                    {
                        ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
                        if (n <= 0)
                        {
                            BLUE_LOG_INFO(g_logger) << "CONNECT client closed, n=" << n;
                            break;
                        }
                        ssize_t sent = ::send(remote_fd, buf, n, 0);
                        if (sent <= 0)
                        {
                            BLUE_LOG_INFO(g_logger) << "CONNECT remote send failed: " << sent 
                                << " errno=" << errno << " " << strerror(errno);
                            break;
                        }
                    }
                    
                    if (FD_ISSET(remote_fd, &fds))
                    {
                        ssize_t n = ::recv(remote_fd, buf, sizeof(buf), 0);
                        if (n <= 0)
                        {
                            BLUE_LOG_INFO(g_logger) << "CONNECT remote closed, n=" << n;
                            break;
                        }
                        ssize_t sent = ::send(client_fd, buf, n, 0);
                        if (sent <= 0)
                        {
                            BLUE_LOG_INFO(g_logger) << "CONNECT client send failed: " << sent;
                            break;
                        }
                    }
                }
            }
            
            remote_sock->close();
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel closed";
        }

        template <typename T>
        void HttpServer<T>::_handleWebSocket(MSocket::MSocketPtr sock, 
                            HttpRequest::HttpRequestPtr request, 
                            HttpResponse::HttpResponsePtr response, 
                            const std::string &targeturl)
        {
            // 1. 连接目标站
            auto url = blue::Url::CreateUrl(targeturl);
            auto addr = url->createAddress();
            auto remote = blue::MSocket::CreateTcp(addr);
            if (!remote->connect(addr)) return;

            // 2. 转发升级请求到目标站
            std::string req_str = request->toString();
            remote->send(req_str.c_str(), req_str.size());

            // 3. 读取目标站的 101 响应
            char buf[4096];
            ssize_t n = remote->recv(buf, sizeof(buf), 0);
            
            // 4. 透传 101 给浏览器
            ::send(sock->getSocketfd(), buf, n, 0);

            // 5. 双向透传 WebSocket 帧（和 CONNECT 隧道一样）
            int client_fd = sock->getSocketfd();
            int remote_fd = remote->getSocketfd();

            fd_set fds;
            while (true) {
                FD_ZERO(&fds);
                FD_SET(client_fd, &fds);
                FD_SET(remote_fd, &fds);
                struct timeval tv = {0, (long)(s_select_timeout * 1000)};
                int ret = select(std::max(client_fd, remote_fd) + 1, &fds, nullptr, nullptr, &tv);
                if (ret > 0) {
                    if (FD_ISSET(client_fd, &fds)) {
                        n = ::recv(client_fd, buf, sizeof(buf), 0);
                        if (n <= 0) break;
                        ::send(remote_fd, buf, n, 0);
                    }
                    if (FD_ISSET(remote_fd, &fds)) {
                        n = ::recv(remote_fd, buf, sizeof(buf), 0);
                        if (n <= 0) break;
                        ::send(client_fd, buf, n, 0);
                    }
                } else if (ret == 0) {
                    blue::Fiber::YieldToHold();
                } else break;
            }
            remote->close();
        }


        template <typename T>
        void HttpServer<T>::_setHeaders(HttpRequest::HttpRequestPtr request, std::map<std::string,std::string> &headers)
        {
            for (auto &[key,val] : request->getHeaders())
            {
                // 跳过 Host（让 DoRequest 自己设置）
                if (strcasecmp(key.c_str(), "host") == 0)
                    continue;
                // 跳过 Referer（指向 localhost，百度会拒绝）
                if (strcasecmp(key.c_str(), "referer") == 0)
                    continue;
                // 跳过 Sec-Fetch-* 头（导致跨域问题）
                if (strncasecmp(key.c_str(), "sec-", 4) == 0)
                    continue;
                headers[key] = val;
            }
        }

        template class blue::http::HttpServer<int>;
        template class blue::http::HttpServer<timeval>;
    }
}