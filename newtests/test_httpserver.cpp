#include "blue/address.h"
#include "http/httpserver.h"
#include "blue/await.h"
#include "blue/log.h"
#include "blue/configinit.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
blue::Task<void> test()
{
    auto httpserver = std::make_shared<blue::http::HttpServer<int>>();
    auto address = blue::Address::LookupAny("0.0.0.0:8082");
    while (!httpserver->bind(address))
    {
        co_await blue::sleepFor(2);
    }

    auto ds = httpserver->getDispatch();
    ds->addServlet("/blue/xxx", [](blue::http::HttpRequest::HttpRequestPtr req,
                                    blue::http::HttpResponse::HttpResponsePtr resp,
                                    blue::http::HttpSession::HttpSessionPtr session) -> int32_t {
        std::string body = "<html>\r\n"
                        "<head>\r\n"
                        "<meta charset='UTF-8'>\r\n"
                        "<title>Blue Servlet</title>\r\n"
                        "<style>\r\n"
                        "body{margin:0;padding:40px;font-family:monospace;background:#f0f0f0;}\r\n"
                        "h1{color:#1677ff;}\r\n"
                        "pre{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);overflow-x:auto;}\r\n"
                        "</style>\r\n"
                        "</head>\r\n"
                        "<body>\r\n"
                        "<h1>Hello Blue, Pinpoint Servlet</h1>\r\n"
                        "<pre>" + req->toString() + "</pre>\r\n"
                        "</body>\r\n"
                        "</html>\r\n";
        BLUE_LOG_INFO(g_logger) << "query : " << req->getQuery(); 
        resp->setHeader("Content-Type", "text/html; charset=utf-8");
        resp->setHeader("Content-Length", std::to_string(body.size()));
        resp->setBody(body);
        return 0;
    });

    ds->addBlurServlet("/blue/*", [](blue::http::HttpRequest::HttpRequestPtr req,
                                    blue::http::HttpResponse::HttpResponsePtr resp,
                                    blue::http::HttpSession::HttpSessionPtr session) -> int32_t {
        std::string body = "<html>\r\n"
                        "<head>\r\n"
                        "<meta charset='UTF-8'>\r\n"
                        "<title>Blue Blur Servlet</title>\r\n"
                        "<style>\r\n"
                        "body{margin:0;padding:40px;font-family:monospace;background:#f5f5f5;}\r\n"
                        "h1{color:#1677ff;}\r\n"
                        "pre{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);overflow-x:auto;}\r\n"
                        "</style>\r\n"
                        "</head>\r\n"
                        "<body>\r\n"
                        "<h1>Hello Blue, Blur Servlet</h1>\r\n"
                        "<pre>" + req->toString() + "</pre>\r\n"
                        "</body>\r\n"
                        "</html>\r\n";
        
        resp->setHeader("Content-Type", "text/html; charset=utf-8");
        resp->setHeader("Content-Length", std::to_string(body.size()));
        resp->setBody(body);
        return 0;
    });
    bool ans = co_await httpserver->start();
    if (ans)
    {
        BLUE_LOG_INFO(g_logger) << "start 成功";
    }
     while (!httpserver->getIsStop())
    {
        co_await blue::sleepFor(2);
    }
    BLUE_LOG_INFO(g_logger) << "httpserver stop";
    blue::IOManager::GetThis()->clear();
    co_return;
}

int main()
{
    blue::http::IniteConfig();
    blue::IOManager iom(2);
    iom.schedule(test());
    iom.wait_all();
}
