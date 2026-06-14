#pragma once
#include <gumbo.h>
#include "url_rewriter.h"

namespace blue
{
    namespace proxy
    {
        class HtmlProcessor
        {
        public:
            static std::string process(const std::string &html,
                                       const UrlRewriter &rewriter);

        private:
            static void serializeNode(GumboNode *node, std::stringstream &ss,
                                      const UrlRewriter &rewriter);

            static void serializeAttributes(const GumboVector *attrs, std::stringstream &ss,
                                            const UrlRewriter &rewriter, GumboTag tag);

            // 辅助函数
            static bool isVoidElement(GumboTag tag);
            static bool isRewritableAttribute(GumboTag tag, const std::string &attr_name);
            static std::string escapeHtmlAttr(const std::string &value);
            static void escapeText(const char *text, std::stringstream &ss);
        private:
            bool base_injected = false;
        };
    }
}