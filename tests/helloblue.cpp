/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <csignal>
#include <atomic>
#include "blue/await.h"
#include "blue/log.h"
#include "blue/task.h"
#include "blue/io_manager.h"
#include "http/httpserver.h"

std::atomic<bool> g_running{true};

void handle(int sig)
{
    g_running = false;
}

blue::Task<void> helloblue()
{
    auto server = std::make_shared<blue::http::HttpServer<int>>();
    auto addr = blue::Address::LookupAnyIpAddress("0.0.0.0:8888");
    while (!server->bind(addr))
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

    co_await server->start();

    while (g_running.load())
    {
        co_await blue::sleepFor(2);
    }
    server->ShutDownServer();
    blue::IOManager::GetThis()->clear();
}


int main()
{
    std::signal(SIGINT, handle);
    std::signal(SIGTERM, handle);
    blue::IOManager iom(2);
    iom.scheduleMul(helloblue());
    iom.wait_all();
}