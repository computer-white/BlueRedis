/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <csignal>
#include <vector>
#include <string>
#include "src/net/address.h"
#include "src/http/httpserver.h"
#include "src/core/await.h"
#include "src/util/log.h"
#include "src/util/configinit.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

std::atomic<bool> g_running{true};

void signalHandler(int signum)
{
    g_running.store(false, std::memory_order_release);
}

// 生成一个带导航链接的简单页面
static std::string makePage(const std::string &title, const std::string &body,
                            const std::vector<std::string> &links)
{
    std::string html = "<!DOCTYPE html><html><head>"
                       "<meta charset='UTF-8'>"
                       "<title>" +
                       title + "</title>"
                               "<style>body{margin:0;padding:40px;font-family:monospace;"
                               "background:#f5f5f5;}h1{color:#1677ff;}"
                               "a{display:block;margin:4px 0;color:#1677ff;}</style>"
                               "</head><body>";
    html += "<h1>" + title + "</h1>";
    html += "<p>" + body + "</p>";
    html += "<nav>";
    for (auto &l : links)
    {
        html += "<a href=\"" + l + "\">" + l + "</a>";
    }
    html += "</nav></body></html>";
    return html;
}

blue::Task<void> test()
{
    auto httpserver = std::make_shared<blue::http::HttpServer>();
    auto address = blue::Address::LookupAny("0.0.0.0:8082");
    if (!address)
    {
        BLUE_LOG_ERROR(g_logger) << "address is empty";
        co_return;
    }
    while (!httpserver->bind<int>(address))
    {
        co_await blue::sleepFor(2);
    }

    // 首页：链到二叉树的根 /page/0 和 /page/1
    httpserver->Get("/", [](auto req,auto resp)
                   {
    std::string body = makePage("Blue Spider Home", "Binary tree test site, 1000 pages.",
        {"/page/0", "/page/1"});
    resp->setHeader("Content-Type", "text/html; charset=utf-8");
    resp->setHeader("Content-Length", std::to_string(body.size()));
    resp->setBody(body);
    return 0; });

    // /page/*：二叉树，/page/N 链 /page/2N+1 和 /page/2N+2
    httpserver->Get("/page/*", [](auto req, auto resp)
                       {
    std::string path = req->getPath();   // "/page/123"
    int n = 0;
    if (path.size() > 6)
    {
        try { n = std::stoi(path.substr(6)); } catch (...) { n = 0; }
    }
    if (n < 0 || n >= 1000)
    {
        // 超出范围，404
        resp->setStatus((blue::http::HttpStatus)404);
        resp->setHeader("Content-Type", "text/plain");
        resp->setBody("404 Not Found");
        return 0;
    }

    std::vector<std::string> links;
    int left = n * 2 + 1;
    int right = n * 2 + 2;
    if (left < 1000) links.push_back("/page/" + std::to_string(left));
    if (right < 1000) links.push_back("/page/" + std::to_string(right));
    links.push_back("/");   // 回首页，增加重复链接测试去重

    std::string body = makePage("Page " + std::to_string(n),
        "This is page " + std::to_string(n) + " of 1000 (binary tree).", links);
    resp->setHeader("Content-Type", "text/html; charset=utf-8");
    resp->setHeader("Content-Length", std::to_string(body.size()));
    resp->setBody(body);
    return 0; });

    bool ans = co_await httpserver->start();
    if (!ans)
    {
        g_logger->Error("Http Server Start Error");
        co_return;
    }
    BLUE_LOG_INFO(g_logger) << "Http Server is running on :8082";
    while (g_running.load(std::memory_order_acquire) && !httpserver->getIsStop())
    {
        co_await blue::sleepFor(2);
    }
    httpserver->ShutDownServer();
    co_await httpserver->waitAcceptDone();
    blue::IOManager::GetThis()->clear();
    BLUE_LOG_INFO(g_logger) << "httpserver Done";
    co_return;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    blue::IOManager iom(2);
    iom.schedule(test());
    iom.wait_all();
}