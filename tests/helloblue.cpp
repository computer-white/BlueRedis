/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <csignal>
#include <atomic>
#include "src/core/await.h"
#include "src/util/log.h"
#include "src/core/task.h"
#include "src/core/io_manager.h"
#include "src/http/httpserver.h"
#include "src/http/httpcontext.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("root");

std::atomic<bool> g_running{true};

void handle(int sig)
{
    g_running = false;
}

blue::Task<void> LoggerMw(blue::http::HttpContext &ctx)
{
    BLUE_LOG_INFO(g_logger) << "-->" << ctx.Request()->getPath();
    co_await ctx.Next();
    BLUE_LOG_INFO(g_logger) << "<--" << blue::http::HttpStatusToChars(ctx.Response()->getStatus());
    co_return;
}

void AuthMw(blue::http::HttpContext &ctx)
{
    auto token = ctx.Request()->getHeader("Authorization");
    if (token == "Bearer valid-token")
    {
        // 通过：不设响应，Abort → 链跳到 final 执行用户 Handle
        ctx.Abort();
        return; // 同步函数，直接 return
    }
    // 失败：设响应 + MarkResponse + Abort → 不执行 Handle
    ctx.Response()->setStatus(blue::http::HttpStatus::UNAUTHORIZED);
    ctx.Response()->setBody(R"({"error":"unauthorized"})");
    ctx.Response()->setHeader("Content-Type", "application/json");
    ctx.MarkResponse();
    ctx.Abort();
}

blue::Task<void> helloblue()
{
    auto server = std::make_shared<blue::http::HttpServer>();

    server->Use(LoggerMw);
    server->Use(AuthMw);
    server->Get("/hello", [](auto req, auto rep)
                {
        std::string body = "Hello Blue";
        rep->setHeader("Content-Type", "text/html; charset=utf-8");
        rep->setBody(body); });

    bool ans = co_await server->start("0.0.0.0:8888");
    if (!ans)
    {
        co_return;
    }

    BLUE_LOG_INFO(g_logger) << "HelloBlue Server is running";

    while (g_running.load())
    {
        co_await blue::sleepFor(2);
    }

    server->ShutDownServer();
    co_await server->waitAcceptDone();
    blue::IOManager::GetThis()->clear();
    BLUE_LOG_INFO(g_logger) << "HelloBlue Server Done";
}

int main()
{
    std::signal(SIGINT, handle);
    std::signal(SIGTERM, handle);
    blue::IOManager iom(2);
    iom.scheduleMul(helloblue());
    iom.wait_all();
}