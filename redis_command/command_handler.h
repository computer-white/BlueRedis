/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file command_handler.h
 * @brief redis server
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.17
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <array>
#include <memory>
#include <regex>
#include <chrono>
#include <list>
#include <iomanip>
#include <unordered_set>
#include "src/util/config.h"
#include "src/core/task.h"
#include "src/net/tcpServer.h"
#include "src/util/resp_parser.h"
#include "src/core/asyncio.h"
#include "src/core/await.h"
#include "src/util/configinit.h"
#include "server_data.h"
#include "generator.h"
#include "command/command_handler_table.h"
#define USE_GENERATOR 0 // 1: Generator, 0: Batch

namespace blue
{
    /**
     * @brief redis 服务器
     */
    class CommandHandler : public TcpServer
    {
    public:
        using CommandHandlerPtr = std::shared_ptr<CommandHandler>;
        using SteadyClock = std::chrono::steady_clock;
        using TimePoint = SteadyClock::time_point;

    public:
        CommandHandler(IOManager *manager = IOManager::GetThis(),
                       IOManager *acceptmanager = IOManager::GetThis());
        ~CommandHandler();

    public:
        /**
         * @brief 搭配generator流式处理命令
         * @param sock
         */
        Generator<RespValue> commandFlow(MSocket::MSocketPtr sock, RespStreamParser &parser,
                                         const char *data, size_t size, bool &should_done);

        /**
         * @brief 批量处理命令
         * @param batch_commands 批量命令列表
         * @param sock 客户端sock
         * @param RecordAOF 是否记录AOF
         */
        std::vector<RespValue> executeBatch(std::vector<std::vector<RespValue>> batch_commands,
                                            MSocket::MSocketPtr sock, bool RecordAOF);

        /**
         * @brief 关闭服务器
         */
        bool ShutDownServer();

    protected:
        /**
         * @brief 处理client事件
         * @param sock 客户端 socket fd
         */
        virtual Task<void> handleClient(MSocket::MSocketPtr sock) override;

    private:
        /**
         * @brief 处理事务模式
         */
        AutoRespValue handleTransactionCommand(const std::string &cmd,
                                               std::vector<RespValue> &args,
                                               MSocket::MSocketPtr sock,
                                               const TimePoint &start);

        /**
         * @brief 处理订阅模式
         */
        AutoRespValue handleSubscriptionCommand(const std::string &cmd,
                                                std::vector<RespValue> &args,
                                                MSocket::MSocketPtr sock,
                                                const TimePoint &start);

        /**
         * @brief 处理进入事务或订阅模式
         */
        AutoRespValue handleModeSwitchCommand(const std::string &cmd,
                                              std::vector<RespValue> &args,
                                              MSocket::MSocketPtr sock,
                                              const TimePoint &start);

        /**
         * @brief 处理发布订阅
         */
        AutoRespValue handlePublishCommand(const std::string &cmd,
                                           std::vector<RespValue> &args,
                                           MSocket::MSocketPtr sock,
                                           const TimePoint &start);

    private:
        std::shared_ptr<ServerData> m_server;
        CommandHandlerTable m_table;
        std::vector<MSocket::MSocketWPtr> m_clients;
    };
}
