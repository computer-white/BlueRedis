#include "url_rewriter.h"
#include "blue/log.h"

namespace blue
{
    namespace proxy
    {

        static Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        UrlRewriter::UrlRewriter(const std::string &target, const std::string &proxy_path)
            : m_target(target), m_proxy_path(proxy_path)
        {
            m_target_url = Url::CreateUrl(target);
            if (m_target_url)
            {
                m_base_url = m_target_url->getScheme() + "://" + m_target_url->getAuthority();
            }
        }

        std::string UrlRewriter::rewrite(const std::string &url) const
        {
            if (shouldSkip(url))
            {
                return url;
            }

            // 情况1：绝对 URL（http:// 或 https://）
            if (url.find("http://") == 0 || url.find("https://") == 0)
            {
                return m_proxy_path + "/" + url;
            }

            // 情况2：协议相对 URL（//example.com/path）
            if (url.size() >= 2 && url[0] == '/' && url[1] == '/')
            {
                return m_proxy_path + "/" + m_target_url->getScheme() + ":" + url;
            }

            // 情况3：根相对路径（/path）
            if (url[0] == '/')
            {
                return m_proxy_path + "/" + m_base_url + url;
            }

            // 情况4：相对路径
            if (!m_target_url)
            {
                return url;
            }

            std::string current_path = m_target_url->getPath();
            std::string resolved = normalizePath(current_path + "/" + url);
            return m_proxy_path + "/" + m_base_url + resolved;
        }

        std::string UrlRewriter::rewriteSrcset(const std::string &srcset) const
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
                {
                    break;
                }
                    

                // 找到 URL 结束位置
                size_t url_start = pos;
                while (pos < len && srcset[pos] != ',' && srcset[pos] != ' ' && srcset[pos] != '\t')
                {
                    pos++;
                }

                if (pos > url_start)
                {
                    std::string url_part = srcset.substr(url_start, pos - url_start);
                    result << rewrite(url_part);
                }

                // 输出空白和描述符
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

        std::string UrlRewriter::getProxyBaseUrl() const
        {
            return m_proxy_path + "/" + m_base_url;
        }

        bool UrlRewriter::shouldSkip(const std::string &url) const
        {
            return url.empty() ||
                   url.find("javascript:") == 0 ||
                   url.find("data:") == 0 ||
                   url.find("mailto:") == 0 ||
                   url.find("tel:") == 0 ||
                   url.find("#") == 0;
        }

        std::string UrlRewriter::normalizePath(const std::string &path) const
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
                    if (!parts.empty())
                    {
                        parts.pop_back();
                    }
                }
                else
                {
                    parts.push_back(part);
                }
            }

            std::string result;
            for (const auto &p : parts)
            {
                result += "/" + p;
            }
            return result.empty() ? "/" : result;
        }

        std::string UrlRewriter::getBaseUrl() const
        {
            return m_base_url;
        }

        std::string UrlRewriter::process_css(const std::string& css)
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
                
                std::string new_url = rewrite(url);
                
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

        std::string UrlRewriter::process_html(const std::string& html)
        {
            GumboOutput *output = gumbo_parse(html.c_str());
            if (!output)
            {
                return html; // 解析失败，返回原文
            }

            std::stringstream ss;
            serializeNode(output->root, ss);

            gumbo_destroy_output(&kGumboDefaultOptions, output);

            return ss.str();
        }

        void UrlRewriter::serializeNode(GumboNode *node, std::stringstream &ss)
        {
            if (!node)
            {
                return;
            }
            const std::string target = m_target;
            const std::string proxy_path = m_proxy_path;

            switch (node->type)
            {
            case GUMBO_NODE_DOCUMENT:
            {
                const GumboVector *children = &node->v.document.children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serializeNode(static_cast<GumboNode *>(children->data[i]),
                                   ss);
                }
                break;
            }

            case GUMBO_NODE_ELEMENT:
            {
                GumboElement *element = &node->v.element;
                const char *tag_name = gumbo_normalized_tagname(element->tag);

                if (element->tag == GUMBO_TAG_META)
                {
                    GumboAttribute *http_equiv = gumbo_get_attribute(&element->attributes, "http-equiv");
                    if (http_equiv && strcasecmp(http_equiv->value, "Content-Security-Policy") == 0)
                    {
                        // 跳过这个 meta 标签，不输出
                        break; // 直接跳出，不序列化
                    }
                }

                ss << "<" << tag_name;
                serializeAttributes(&element->attributes, ss, element->tag);

                // 在 <head> 内注入 <base> 和 JS 拦截器（只注入一次）
                if (element->tag == GUMBO_TAG_HEAD && !base_injected)
                {
                    ss << ">";
                    ss << "<base href=\"" << proxy_path << "/" << target<< "\">";

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
                        serializeNode(static_cast<GumboNode *>(children->data[i]),
                                       ss);
                    }

                    ss << "</head>";
                    return;
                }

                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serializeNode(static_cast<GumboNode *>(children->data[i]),
                                   ss);
                }

                if (!isVoidElement(element->tag))
                {
                    ss << "</" << tag_name << ">";
                }
                break;
            }

            case GUMBO_NODE_TEXT:
                escapeText(node->v.text.text, ss);
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
                serializeAttributes(&element->attributes, ss, GUMBO_TAG_TEMPLATE);
                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serializeNode(static_cast<GumboNode *>(children->data[i]),
                                   ss);
                }

                ss << "</template>";
                break;
            }
            }
        }

        void UrlRewriter::serializeAttributes(const GumboVector *attrs, std::stringstream &ss, GumboTag tag)
        {
            const std::string target = m_target;
            const std::string proxy_path = m_proxy_path;
            for (unsigned int i = 0; i < attrs->length; i++)
            {
                auto *attr = static_cast<GumboAttribute *>(attrs->data[i]);
                std::string attr_name(attr->name);
                std::string attr_value(attr->value);

                // 判断是否需要重写
                if (isRewritableAttribute(tag, attr_name))
                {
                    BLUE_LOG_WARN(g_logger) << "Rewriting: tag=" << (int)tag 
                            << " attr=" << attr_name 
                            << " value=" << attr_value;
                    if (attr_name == "srcset")
                    {
                        attr_value = rewriteSrcset(attr_value);
                    }
                    else
                    {
                        attr_value = rewrite(attr_value);
                    }
                }

                ss << " " << attr->name << "=\"" << escapeHtmlAttr(attr_value) << "\"";
            }
        }

        // 辅助函数
        bool UrlRewriter::isVoidElement(GumboTag tag)
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

        bool UrlRewriter::isRewritableAttribute(GumboTag tag, const std::string &attr_name)
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

        std::string UrlRewriter::escapeHtmlAttr(const std::string &value)
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

        void UrlRewriter::escapeText(const char *text, std::stringstream &ss)
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

    } // namespace proxy
} // namespace blue