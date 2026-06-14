#pragma once
#include <gumbo.h>
#include <regex>
#include <string>
#include <vector>
#include "blue/url.h"

namespace blue
{
    namespace proxy
    {
        class UrlRewriter
        {
        public:
            UrlRewriter(const std::string& target, const std::string& proxy_path = "/blue");
            
            // 核心重写方法
            std::string rewrite(const std::string& url) const;
            std::string rewriteSrcset(const std::string& srcset) const;
            
            // 获取重写后的 URL（用于 base 标签）
            std::string getProxyBaseUrl() const;
            
            // 获取原始 target 信息
            const std::string& getTarget() const { return m_target; }
            const std::string& getProxyPath() const { return m_proxy_path; }
            
            std::string process_css(const std::string& css);
            std::string process_html(const std::string& html);
        private:
            bool shouldSkip(const std::string& url) const;
            std::string normalizePath(const std::string& path) const;
            std::string getBaseUrl() const;
            void serializeNode(GumboNode *node, std::stringstream &ss);

            void serializeAttributes(const GumboVector *attrs, std::stringstream &ss,GumboTag tag);

            // 辅助函数
            bool isVoidElement(GumboTag tag);
            bool isRewritableAttribute(GumboTag tag, const std::string &attr_name);
            std::string escapeHtmlAttr(const std::string &value);
            void escapeText(const char *text, std::stringstream &ss);
        private:
            bool base_injected = false;
            
            std::string m_target;
            std::string m_proxy_path;
            std::shared_ptr<Url> m_target_url;
            std::string m_base_url;  // scheme://authority
        };
    }
}