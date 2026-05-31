#include <fnmatch.h>
#include "httpservlet.h"

// servlet
namespace blue
{
    namespace http
    {
        FunctionServlet::FunctionServlet(CallBack cb)
        :Servlet("FunctionServlet"),
        m_cb(cb)
        {
        }

        int32_t FunctionServlet::handle(http::HttpRequest::HttpRequestPtr request,
                                        http::HttpResponse::HttpResponsePtr response,
                                        http::HttpSession::HttpSessionPtr session)
        {
            return m_cb(request,response,session);
        }

        ServletDispatch::ServletDispatch()
        :Servlet("ServletDispatch")
        {
            m_default.reset(new NotFoundServlet());
        }

        int32_t ServletDispatch::handle(http::HttpRequest::HttpRequestPtr request,
                                        http::HttpResponse::HttpResponsePtr response,
                                        http::HttpSession::HttpSessionPtr session)
        {
            auto servlet = getMatchedServlet(request->getPath());
            if (servlet)
            {
                return servlet->handle(request,response,session);
            }
            return 0;
        }

        void ServletDispatch::addServlet(const std::string &url, std::shared_ptr<Servlet> sv)
        {
            MRWmutexType::WritelockSco lock(m_mutex);
            m_pinpointData[url] = sv;
        }

        void ServletDispatch::addServlet(const std::string &url, FunctionServlet::CallBack fs)
        {
            MRWmutexType::WritelockSco lock(m_mutex);
            m_pinpointData[url].reset(new FunctionServlet(fs));
        }

        void ServletDispatch::addBlurServlet(const std::string &url, std::shared_ptr<Servlet> sv)
        {
            MRWmutexType::WritelockSco lock(m_mutex);
            for (auto it = m_blurData.begin();it != m_blurData.end();it++)
            {
                if (!(fnmatch(it->first.c_str(),url.c_str(),0)))
                {
                    m_blurData.erase(it);
                    break;
                }
            }
            m_blurData.emplace_back(url,sv);
        }

        void ServletDispatch::addBlurServlet(const std::string &url, FunctionServlet::CallBack fs)
        {
            auto sv = std::make_shared<FunctionServlet>(fs);
            addBlurServlet(url,sv);
        }

        void ServletDispatch::delServlet(const std::string &url)
        {
            MRWmutexType::ReadlockSco lock(m_mutex);
            auto it = m_pinpointData.find(url);
            if (it == m_pinpointData.end())
            {
                return;
            }
            lock.unlock();
            MRWmutexType::WritelockSco lock2(m_mutex);
            if (it == m_pinpointData.end())
            {
                return;
            }
            m_pinpointData.erase(it);
        }

        void ServletDispatch::delBlurServlet(const std::string &url)
        {
            MRWmutexType::ReadlockSco lock(m_mutex);
            auto it = std::find_if(m_blurData.begin(),m_blurData.end(),[u = url](const auto& p){
                return !(fnmatch(p.first.c_str(),u.c_str(),0));
            });
            if (it == m_blurData.end())
            {
                return;
            }
            lock.unlock();
            MRWmutexType::WritelockSco lock2(m_mutex);
            if (it == m_blurData.end())
            {
                return;
            }
            m_blurData.erase(it);
        }

        std::shared_ptr<Servlet> ServletDispatch::getServlet(const std::string &url)
        {
            MRWmutexType::ReadlockSco lock(m_mutex);
            auto it = m_pinpointData.find(url);
            return it == m_pinpointData.end() ? nullptr : it->second;
        }

        std::shared_ptr<Servlet> ServletDispatch::getBlurServlet(const std::string &url)
        {
            MRWmutexType::ReadlockSco lock(m_mutex);
            auto it = std::find_if(m_blurData.begin(),m_blurData.end(),[u = url](const auto& p){
                return !(fnmatch(p.first.c_str(),u.c_str(),0));
            });
            return it == m_blurData.end() ? nullptr : it->second;
        }

        std::shared_ptr<Servlet> ServletDispatch::getMatchedServlet(const std::string &url)
        {
            MRWmutexType::ReadlockSco lock(m_mutex);
            auto it = m_pinpointData.find(url);
            if (it != m_pinpointData.end())
            {
                return it->second;
            }
            auto it2 = std::find_if(m_blurData.begin(),m_blurData.end(),[u = url](const auto& p){
                return !(fnmatch(p.first.c_str(),u.c_str(),0));
            });
            if (it2 != m_blurData.end())
            {
                return it2->second;
            }
            return m_default;
        }

        NotFoundServlet::NotFoundServlet()
         :Servlet("NotFoundServlet")
         {

         }
        int32_t  NotFoundServlet::handle(http::HttpRequest::HttpRequestPtr request,
                                   http::HttpResponse::HttpResponsePtr response,
                                   http::HttpSession::HttpSessionPtr session)
        {
            static const char* page_404 = 
                            "<html>\r\n"
                            "<head>\r\n"
                            "<meta charset='UTF-8'>\r\n"
                            "<title>404 Not Found</title>\r\n"
                            "<style>\r\n"
                            "body {\r\n"
                            "    margin: 0;\r\n"
                            "    padding: 0;\r\n"
                            "    background-color: #f5f5f5;\r\n"
                            "    font-family: Arial, sans-serif;\r\n"
                            "    display: flex;\r\n"
                            "    justify-content: center;\r\n"
                            "    align-items: center;\r\n"
                            "    height: 100vh;\r\n"
                            "}\r\n"
                            ".container {\r\n"
                            "    text-align: center;\r\n"
                            "}\r\n"
                            "h1 {\r\n"
                            "    font-size: 72px;\r\n"
                            "    color: #333;\r\n"
                            "    margin: 0;\r\n"
                            "}\r\n"
                            "p {\r\n"
                            "    font-size: 18px;\r\n"
                            "    color: #999;\r\n"
                            "    margin-top: 10px;\r\n"
                            "}\r\n"
                            "hr {\r\n"
                            "    width: 200px;\r\n"
                            "    border: none;\r\n"
                            "    border-top: 1px solid #ddd;\r\n"
                            "    margin: 20px auto;\r\n"
                            "}\r\n"
                            ".server {\r\n"
                            "    font-size: 14px;\r\n"
                            "    color: #ccc;\r\n"
                            "}\r\n"
                            "</style>\r\n"
                            "</head>\r\n"
                            "<body>\r\n"
                            "<div class='container'>\r\n"
                            "<h1>404</h1>\r\n"
                            "<p>Not Found</p>\r\n"
                            "<hr>\r\n"
                            "<div class='server'>blue/1.0.0</div>\r\n"
                            "</div>\r\n"
                            "</body>\r\n"
                            "</html>\r\n";
                        response->setStatus(blue::http::HttpStatus::NOT_FOUND);
                        response->setHeader("Server", "blue/1.0.0");
                        response->setHeader("Content-Type", "text/html");
                        response->setHeader("Content-Length", std::to_string(strlen(page_404)));
                        response->setBody(page_404);
            return 0;
        }
    }
}