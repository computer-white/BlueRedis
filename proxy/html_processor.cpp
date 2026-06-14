#include "html_processor.h"
#include "blue/log.h"
namespace blue
{
    namespace proxy
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
        std::string HtmlProcessor::process(const std::string &html,
                                           const UrlRewriter &rewriter)
        {
            GumboOutput *output = gumbo_parse(html.c_str());
            if (!output)
            {
                return html; // 解析失败，返回原文
            }

            std::stringstream ss;
            serializeNode(output->root, ss, rewriter);

            gumbo_destroy_output(&kGumboDefaultOptions, output);

            return ss.str();
        }

        void HtmlProcessor::serializeNode(GumboNode *node, std::stringstream &ss,
                                          const UrlRewriter &rewriter)
        {
            if (!node)
            {
                return;
            }
            const std::string target = rewriter.getTarget();
            const std::string proxy_path = rewriter.getProxyPath();

            switch (node->type)
            {
            case GUMBO_NODE_DOCUMENT:
            {
                const GumboVector *children = &node->v.document.children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serializeNode(static_cast<GumboNode *>(children->data[i]),
                                   ss, rewriter);
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
                serializeAttributes(&element->attributes, ss, rewriter, element->tag);

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
                                       ss, rewriter);
                    }

                    ss << "</head>";
                    return;
                }

                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serializeNode(static_cast<GumboNode *>(children->data[i]),
                                   ss, rewriter);
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
                serializeAttributes(&element->attributes, ss, rewriter, GUMBO_TAG_TEMPLATE);
                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serializeNode(static_cast<GumboNode *>(children->data[i]),
                                   ss, rewriter);
                }

                ss << "</template>";
                break;
            }
            }
        }

        void HtmlProcessor::serializeAttributes(const GumboVector *attrs, std::stringstream &ss,
                                                const UrlRewriter &rewriter, GumboTag tag)
        {
            const std::string target = rewriter.getTarget();
            const std::string proxy_path = rewriter.getProxyPath();
            for (unsigned int i = 0; i < attributes->length; i++)
            {
                auto *attr = static_cast<GumboAttribute *>(attributes->data[i]);
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
                        attr_value = rewriter.rewriteSrcset(attr_value);
                    }
                    else
                    {
                        attr_value = rewriter.rewrite(attr_value);
                    }
                }

                ss << " " << attr->name << "=\"" << escapeHtmlAttr(attr_value) << "\"";
            }
        }

        // 辅助函数
        bool HtmlProcessor::isVoidElement(GumboTag tag)
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

        bool HtmlProcessor::isRewritableAttribute(GumboTag tag, const std::string &attr_name)
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

        std::string HtmlProcessor::escapeHtmlAttr(const std::string &value)
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

        void HtmlProcessor::escapeText(const char *text, std::stringstream &ss)
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
    }
} // namespace name