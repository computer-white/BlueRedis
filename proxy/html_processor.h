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
 /**
 * @file html_processor.h
 * @brief html重写模块
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.19
 * @copyright Copyright (c) 2026年 blue
 */
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