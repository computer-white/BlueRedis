/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <csignal>
#include <atomic>
#include "blue/io_manager.h"
#include "blue/log.h"
#include "redis_command/cluster/cluster_manager.h"

using namespace blue;

static Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

std::atomic<bool> g_running{true};

void signalHandler(int signum)
{
    g_running.store(false, std::memory_order_release);
}

Task<void> test()
{
    std::vector<std::string> slaves_hosts = {"127.0.0.1:6666"};
    std::string master_addr = "127.0.0.1";
    uint16_t master_port = 6668;

    auto cluster = std::make_shared<blue::cluster::ClusterManager<int>>();
    if (!cluster)
    {
        BLUE_LOG_INFO(g_logger) << "cluster is empty";
        co_return;
    }

    while(!co_await cluster->bindMaster(std::move(master_addr), master_port))
    {
        co_await sleepFor(2);
    }

    while (!co_await cluster->bindSlave(std::move(slaves_hosts)))
    {
        co_await sleepFor(2);
    }
    BLUE_LOG_INFO(g_logger) << "cluster started ";
    // 保持集群运行，直到收到停止信号
    while (g_running.load(std::memory_order_acquire))
    {
        co_await sleepFor(1);
    }
    bool ans = co_await cluster->stop();
    if (ans)
    {
        BLUE_LOG_INFO(g_logger) << "cluster stopped ";
        IOManager::GetThis()->clear();
    }
    co_return;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    IOManager iom(2);
    iom.schedule(test());
    iom.wait_all();
}