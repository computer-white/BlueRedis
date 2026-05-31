#ifndef BLUE_HTTP_HTTPSERVLET_H
#define BLUE_HttP_HTTPSERVLET_H
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "blue/mthread.h"
#include "http.h"
#include "httpsession.h"
/* URL路由 + 处理函数 = servlet*/
// servlet
namespace blue
{
    namespace http
    {
        class Servlet
        {
        public:
            using ServletPtr = std::shared_ptr<Servlet>;

        public:
            /**
             * @brief servlet的构造函数
             * @param name 设置servlet的名称
             */
            Servlet(const std::string &name)
                : m_name(name) {}
            /**
             * @brief 默认虚析构函数
             */
            virtual ~Servlet() {}

            /**
             * @brief 关键函数,实现对request中url的路由后选择合适的子类进行处理
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             * @param session 一次客户端与服务器的连接抽象
             */
            virtual int32_t handle(http::HttpRequest::HttpRequestPtr request,
                                   http::HttpResponse::HttpResponsePtr response,
                                   http::HttpSession::HttpSessionPtr session) = 0;
            /**
             * @brief 获取servlet名称
             * @return 返回servlet名称
             */
            const std::string &getName() const noexcept { return m_name; }

        protected:
            std::string m_name;
        };

        class FunctionServlet : public Servlet
        {
        public:
            using FunctionServletPtr = std::shared_ptr<FunctionServlet>;
            using CallBack = std::function<int32_t(http::HttpRequest::HttpRequestPtr request,
                                                    http::HttpResponse::HttpResponsePtr response,
                                                    http::HttpSession::HttpSessionPtr session)>;

        public:
            /**
             * @brief 将匹配成功的url与cb函数对应
             * @param cb 对于不同匹配成功的url选择和设置的不同的回调函数
             */
            FunctionServlet(CallBack cb);

            /**
             * @brief 关键函数,实现对request中url的路由后传递给用户设置的回调函数中进行处理
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             * @param session 一次客户端与服务器的连接抽象
             */
            virtual int32_t handle(http::HttpRequest::HttpRequestPtr request,
                                   http::HttpResponse::HttpResponsePtr response,
                                   http::HttpSession::HttpSessionPtr session) override;

        private:
            CallBack m_cb;
        };

        class ServletDispatch : public Servlet
        {
        public:
            using ServletDispatchPtr = std::shared_ptr<ServletDispatch>;
            using MRWmutexType = blue::MRWmutex;
        public:
            /**
             * @brief 实现对url的精准和模糊匹配并派遣相应处理的servlet
             */
            ServletDispatch();

            /**
             * @brief 关键函数,实现对request中url进行模糊和精准匹配后选择合适的servlet对象进行处理
             * @param request 请求实例智能指针
             * @param response 响应智能指针
             * @param session 一次客户端与服务器的连接抽象
             * @note 若模糊和精准都没匹配上则选择使用默认servlet(404 not found)
             */
            virtual int32_t handle(http::HttpRequest::HttpRequestPtr request,
                                   http::HttpResponse::HttpResponsePtr response,
                                   http::HttpSession::HttpSessionPtr session) override;
            /**
             * @brief 将url设置为精准匹配模式串,并设置想要对匹配成功的url进行处理的servlet对象智能指针
             * @param url 想要进行精准匹配的url模式串
             * @param sv servlet对象智能指针
             */
            void addServlet(const std::string& url, std::shared_ptr<Servlet> sv);

            /**
             * @brief 将url设置为精准匹配模式串,并设置想要对匹配成功的url进行处理的回调函数
             * @param url 想要进行精准匹配的url模式串
             * @param fs 回调函数
             */
            void addServlet(const std::string& url, FunctionServlet::CallBack fs);

            /**
             * @brief 将url设置为模糊匹配模式串,并设置想要对匹配成功的url进行处理的servlet对象智能指针
             * @param url 想要进行模糊匹配的url模式串
             * @param sv servlet对象智能指针
             */
            void addBlurServlet(const std::string& url, std::shared_ptr<Servlet> sv);

            /**
             * @brief 将url设置为模糊匹配模式串,并设置想要对匹配成功的url进行处理的回调函数
             * @param url 想要进行模糊匹配的url模式串
             * @param fs 回调函数
             */
            void addBlurServlet(const std::string& url, FunctionServlet::CallBack fs);
            
            /**
             * @brief 删除精准匹配模式串url
             * @param url 需要删除的精准匹配的url模式串
             */
            void delServlet(const std::string& url);

            /**
             * @brief 删除模糊匹配模式串url
             * @param url 需要删除的模糊匹配的url模式串
             */
            void delBlurServlet(const std::string& url);

            /**
             * @brief 获取默认处理servlet智能指针
             * @return 默认处理servlet智能指针
             */
            std::shared_ptr<Servlet> getDefault() const { return m_default; }

            /**
             * @brief 设置默认处理servlet智能指针
             * @param defval 想要设置的默认的servlet智能指针
             */
            void setDefault(std::shared_ptr<Servlet> defval) { m_default = defval; } 


            /**
             * @brief 获取精准匹配下的servlet
             * @param url 对url进行精准匹配
             * @return 精准匹配成功的url对应的servlet
             */
            std::shared_ptr<Servlet> getServlet(const std::string& url);

            /**
             * @brief 获取模糊匹配下的servlet
             * @param url 对url进行模糊匹配
             * @return 模糊匹配成功的url对应的servlet
             */
            std::shared_ptr<Servlet> getBlurServlet(const std::string& url);

            /**
             * @brief 获取精准和模糊匹配下的servlet
             * @param url 对url进行精准和模糊匹配
             * @return 精准和模糊匹配成功的url对应的servlet
             * @note 哪个匹配成功返回哪个,没有返回默认servlet,默认先进行精准匹配
             */
            std::shared_ptr<Servlet> getMatchedServlet(const std::string& url);

        private:
            MRWmutexType m_mutex;
            /* /blue/xxx -> servlet */
            std::unordered_map<std::string, std::shared_ptr<Servlet> > m_pinpointData; // 精准匹配
            /* /blue/* -> servlet */
            std::vector<std::pair<std::string, std::shared_ptr<Servlet> >> m_blurData; // 模糊匹配

            std::shared_ptr<Servlet> m_default;
        };

        class NotFoundServlet : public Servlet
        {
            public:
                using NotFoundServletPtr = std::shared_ptr<NotFoundServlet>;
            public:

                /**
                 * @brief 对于没有在dispatch中进行模糊和精准匹配找到的url的处理servlet
                 */
                NotFoundServlet();

                /**
                 * @brief 关键函数,实现对request中url进行模糊和精准匹配后没有匹配到处理函数
                 * @param request 请求实例智能指针
                 * @param response 响应智能指针
                 * @param session 一次客户端与服务器的连接抽象
                 */
                virtual int32_t handle(http::HttpRequest::HttpRequestPtr request,
                                   http::HttpResponse::HttpResponsePtr response,
                                   http::HttpSession::HttpSessionPtr session) override;
        };
    }
}

#endif