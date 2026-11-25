#pragma once
#include <vector>
#include <functional>
#include "src/core/task.h"
#include "src/http/httpcontext.h"

namespace blue
{
    namespace http
    {
        using AsyncFunc = std::function<Task<void>(HttpContext &)>;

        template <typename F>
        AsyncFunc ToAsync(F &&f)
        {
            // f(ctx) 的返回值
            using Ret = std::invoke_result_t<F &, HttpContext &>;
            if constexpr (std::is_void_v<Ret>)
            {
                return [f = std::forward<F>(f)](HttpContext &c) -> Task<void>
                {
                    f(c);
                    co_return;
                };
            }
            else
            {
                return AsyncFunc(std::forward<F>(f));
            }
        }

        class MiddlewareChain
        {
        public:
            template <typename F>
            void Use(F &&mw)
            {
                m_middlewares.push_back(ToAsync(std::forward<F>(mw)));
            }

            template <typename F>
            void SetFinal(F &&h) { m_final = ToAsync(std::forward<F>(h)); }

            Task<void> Run(HttpContext &ctx)
            {
                m_ctx = &ctx;
                ctx.SetChain(this);
                m_index = 0;
                co_await Next();
            }

            Task<void> Next()
            {
                if (!m_ctx)
                {
                    co_return;
                }

                if (m_ctx->Aborted())
                {
                    if (!m_ctx->HasResponse() && m_final)
                    {
                        auto f = std::move(m_final);
                        m_final = nullptr;
                        co_await f(*m_ctx);
                    }
                    co_return;
                }

                if (m_index < m_middlewares.size())
                {
                    auto mw = m_middlewares[m_index++];
                    co_await mw(*m_ctx);
                }
                else if (m_final)
                {
                    auto f = std::move(m_final);
                    m_final = nullptr;
                    co_await f(*m_ctx);
                }
                co_return;
            }

        private:
            std::vector<AsyncFunc> m_middlewares;
            AsyncFunc m_final = nullptr;
            HttpContext *m_ctx = nullptr;
            size_t m_index = 0;
        };

        // 将 Context 的 Next 函数定义放到这里，防止循环引用
        inline Task<void> HttpContext::Next()
        {
            co_await m_chain->Next();
        }
    }
}