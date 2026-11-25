#pragma once
#include <vector>
#include <functional>
#include <memory>
#include "src/core/task.h"
#include "src/http/http.h" // 按你的实际头文件调整

namespace blue
{
    namespace http
    {

        class MiddlewareChain;
        class HttpContext
        {
        public:
            using HttpContextPtr = std::shared_ptr<HttpContext>;

            HttpContext(HttpRequest::HttpRequestPtr req,
                        HttpResponse::HttpResponsePtr resp)
                : m_req(std::move(req)), m_resp(std::move(resp)) {}

            /**
             * @brief 获取 httpRequestPtr
             */
            HttpRequest::HttpRequestPtr Request() const { return m_req; }

            /**
             * @brief 获取 httpResponsePtr
             */
            HttpResponse::HttpResponsePtr Response() const { return m_resp; }

            /**
             * @brief 设置中间件链
             */
            void SetChain(MiddlewareChain *chain) { m_chain = chain; }

            /**
             * @brief 下一个中间件
             */
            Task<void> Next();

            /**
             * @brief 设置Abort
             */
            void Abort() { m_aborted = true; }

            /**
             * @brief 是否Abort
             */
            bool Aborted() const { return m_aborted; }

            /**
             * @brief 是否有Response
             */
            bool HasResponse() const { return m_hasResponse; }

            /**
             * @brief 标记设置了Response
             */
            void MarkResponse() { m_hasResponse = true; }

        private:
            HttpRequest::HttpRequestPtr m_req;
            HttpResponse::HttpResponsePtr m_resp;
            MiddlewareChain *m_chain = nullptr;
            bool m_aborted = false;
            bool m_hasResponse = false;
        };
    }
} // namespace