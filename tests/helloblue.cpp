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


static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("root");

std::atomic<bool> g_running{true};

void handle(int sig)
{
    g_running = false;
}

blue::Task<void> helloblue()
{
    auto server = std::make_shared<blue::http::HttpServer>();
    auto addr = blue::Address::LookupAnyIpAddress("0.0.0.0:8888");
    if (!addr)
    {
        BLUE_LOG_ERROR(g_logger) << "addr is empty";
        co_return;
    }
    while (!server->bind<int>(addr))
    {
        co_await blue::sleepFor(1);
    }

    auto dispatch = server->getDispatch();
    dispatch->addServlet("/hello",[](auto req, auto rep, auto session) -> int32_t
    {
        std::string body = "Hello Blue";
        rep->setHeader("Content-Type", "text/html; charset=utf-8");
        rep->setHeader("Content-Length", std::to_string(body.size()));
        rep->setBody(body);
        return 0;
    });

    bool ans = co_await server->start();
    if (!ans)
    {
        BLUE_LOG_INFO(g_logger) << "Server started error on " << addr->toString();
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