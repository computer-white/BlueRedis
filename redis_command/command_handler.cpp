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
#include "command_handler.h"
#include "blue/asyncio.h"
#include "blue/await.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    CommandHandler::CommandHandler(IOManager *manager,
                                   IOManager *acceptmanager)
        : TcpServer(manager, acceptmanager)
    {
        m_server = std::make_shared<ServerData>();
        m_server->setTcpServer(this);
        // 设置 Replication 执行器
        m_server->getReplication().setExecutor([this](std::vector<RespValue> args,
                                                      MSocket::MSocketPtr sock,
                                                      bool record) -> RespValue
                                               { return m_table.executeTable(args, sock, m_server, record); });
        // 设置 AOF 执行器
        m_server->getAOF().setExecutor([this](std::vector<RespValue> args,
                                              MSocket::MSocketPtr sock,
                                              bool record) -> RespValue
                                       { return m_table.executeTable(args, sock, m_server, record); });
        m_server->loadFromFile();
        m_server->getAOF().loadAOF();
        m_server->getAOF().initAOF(); // 初始化AOF,追加打开AOF文件,并开启AOF同步协程
        IOManager::GetThis()->schedule(m_server->expireTime());
        m_server->getAOF().setLastAOFSync(SteadyClock::now());
        m_clients.reserve(s_redis_server_maxClients.load(std::memory_order_acquire));
    }

    CommandHandler::~CommandHandler()
    {
        m_server->setShutdown(true);                                  // 设置服务器停止标识
        m_server->getReplication().setStop();                         // 停止replicationLoop和processReplQueue
        m_server->getAOF().stop();                                    // 停止aofSyncLoop
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 等待结束
        m_server->getAOF().stopAOFFlushThread();                      // 停止stopAOFFlushThread
        m_server->saveToFile();                                       // 保存进入rdb
        m_server->getAOF().closeAOFWithFlush();                       // 刷新并关闭AOF写入文件流
    }

    bool CommandHandler::ShutDownServer()
    {
        for (auto wptr : m_clients)
        {
            if (wptr.expired())
            {
                continue;
            }
            auto ptr = wptr.lock();
            if (ptr)
            {
                ptr->shutdown(SHUT_RDWR);
            }
        }
        return true;
    }

    std::vector<RespValue> CommandHandler::executeBatch(std::vector<std::vector<RespValue>> batch_commands,
                                                        MSocket::MSocketPtr sock, bool RecordAOF)
    {
        std::vector<RespValue> results;
        results.reserve(batch_commands.size());

        // 批量执行
        for (auto &args : batch_commands)
        {
            results.push_back(m_table.executeTable(args, sock, m_server, RecordAOF));
        }
        return results;
    }

#if ((USE_GENERATOR) == 1)
    Task<void> CommandHandler::handleClient(MSocket::MSocketPtr sock)
    {
        m_clients.push_back(sock);
        BLUE_LOG_INFO(g_logger) << "Generator Mode";
        BLUE_LOG_INFO(g_logger) << "Redis Server begin, fd=" << sock->getSocketfd();
        // BLUE_LOG_INFO(g_logger) << "remote address: " <<  sock->getRemoteAddress()->toString();
        // BLUE_LOG_INFO(g_logger) << "local address : " <<  sock->getLocalAddress()->toString();

        RespStreamParser parser; // 解析器
        // const size_t MAX_COMMAND_SIZE = 1024 * 1024; // 解析缓冲区最大大小
        // const size_t BATCH_SIZE = 256 * 1024;        // 批量响应大小阈值

        std::string batch_response;
        batch_response.reserve(s_max_batch_size.load(std::memory_order_acquire()));
        int cmd_count = 0;

        // 回复函数
        auto send_response = [&](std::string &data) -> Task<void>
        {
            if (data.empty())
            {
                co_return;
            }
            size_t sent = 0;
            while (sent < data.size())
            {
                ssize_t n = co_await sock->send(data.data() + sent, data.size() - sent);
                if (n <= 0)
                {
                    BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd() << "] 发送失败";
                    break;
                }
                sent += n;
            }
            data.clear();
        };

        do
        {
            if (m_server->getShutdown().load(std::memory_order_acquire) || this->getIsStop())
            {
                // 关闭服务器
                this->ShutDownServer();
                break;
            }
            const uint64_t timeout = s_redis_server_timeout.load(std::memory_order_acquire);
            char tmp[8192];
            ssize_t ret;
            if (timeout > 0)
            {
                ret = co_await sock->recvT(tmp, sizeof(tmp), 0, timeout);
            }
            else
            {
                ret = co_await sock->recv(tmp, sizeof(tmp));
            }
            if (ret <= 0)
            {
                if (ret == 0)
                {
                    BLUE_LOG_INFO(g_logger) << "[client " << sock->getSocketfd() << "] 正常关闭";
                    break;
                }
                if (errno == ETIMEDOUT)
                {
                    BLUE_LOG_WARN(g_logger) << "[client " << sock->getSocketfd()
                                            << "] timeout (" << util::ConfigParser::FormatTime(std::chrono::microseconds(timeout)) << "), closing";
                    break;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd()
                                         << "] recv error: " << strerror(errno);
                break;
            }

            bool should_done = false;
            for (const auto &response : commandFlow(sock, parser, tmp, ret, should_done))
            {
                if (should_done)
                {
                    goto clean;
                }
                // batch_response += RespValue::encode(response);
                response->encodeTo(batch_response);
                cmd_count++;
                m_server->incrementCommands();

                if (batch_response.size() >= s_max_batch_size.load(std::memory_order_acquire()))
                {
                    co_await send_response(batch_response);
                    co_await std::suspend_always{};
                }
            }

            // 发送剩余的响应
            if (!batch_response.empty())
            {
                co_await send_response(batch_response);
            }

            if (cmd_count > 0)
            {
                BLUE_LOG_DEBUGE(g_logger) << "Processed " << cmd_count
                                          << " commands in batch, batch_size="
                                          << batch_response.size();
            }

            // 检查缓冲区大小
            if (parser.bufferSize() > s_max_command_size.load(std::memory_order_acquire()))
            {
                BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd()
                                         << "] 命令过大，关闭连接";
                break;
            }

        } while (true);
    clean:
        // 检查是否是管理员连接断开
        auto admin = m_server->getAdminSocket().lock();
        if (admin && admin.get() == sock.get())
        {
            m_server->getAdminSocket().reset();
        }

        // 关闭前清理订阅
        m_server->getSubscription().removeAllSubscribers(sock);

        sock->clearSubScription();
        BLUE_LOG_INFO(g_logger) << "one Client exit, fd:" << sock->getSocketfd();
        sock->close();

        // 删除过期的monitor
        m_server->getMonitor().removeMonitor();

        this->subConnection() if (this->getConnection() == 0 && m_server->getShutdown().load(std::memory_order_acquire))
        {
            bool end = co_await this->stop();
            if (end)
            {
                BLUE_LOG_INFO(g_logger) << "tcpserver stoped";
            }
        }
        co_return;
    }
#else
    Task<void> CommandHandler::handleClient(MSocket::MSocketPtr sock)
    {
        // 收集连接上来的客户端sock
        m_clients.push_back(sock);
        BLUE_LOG_INFO(g_logger) << "Batch_commands Mode";
        BLUE_LOG_INFO(g_logger) << "Redis Server begin, fd=" << sock->getSocketfd();
        // BLUE_LOG_INFO(g_logger) << "remote address: " <<  sock->getRemoteAddress()->toString();
        // BLUE_LOG_INFO(g_logger) << "local address : " <<  sock->getLocalAddress()->toString();

        RespStreamParser parser(s_max_command_size.load(std::memory_order_acquire)); // 解析器
        std::vector<std::vector<RespValue>> batch_commands;                          // 批量命令数组
        batch_commands.reserve(s_max_exec_batch_size.load(std::memory_order_acquire));

        std::string batch_response;
        batch_response.reserve(s_max_batch_size.load(std::memory_order_acquire) * 2);
        int cmd_count = 0;

        blue::AutoRespValue response;

        // 回复函数
        auto send_response = [&](std::string &data) -> Task<void>
        {
            if (data.empty())
            {
                co_return;
            }
            size_t sent = 0;
            while (sent < data.size())
            {
                ssize_t n = co_await sock->send(data.data() + sent, data.size() - sent);
                if (n <= 0)
                {
                    BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd() << "] 发送失败";
                    break;
                }
                sent += n;
            }
            data.clear();
        };

        // 批量执行函数
        auto execute_and_encode = [&](std::vector<std::vector<RespValue>> &commands) -> Task<void>
        {
            if (commands.empty())
            {
                co_return;
            }

            auto results = executeBatch(std::move(commands), sock, true);
            for (auto &resp : results)
            {
                batch_response += RespValue::encode(resp);
            }
            commands.clear();
            commands.reserve(s_max_exec_batch_size.load(std::memory_order_acquire));

            // 如果响应达到阈值，立即发送
            if (batch_response.size() >= s_max_batch_size.load(std::memory_order_acquire))
            {
                co_await send_response(batch_response);

                // 让出 CPU，避免饿死其他协程
                co_await std::suspend_always{};
            }
        };
        do
        {
            if (m_server->getShutdown().load(std::memory_order_acquire) || this->getIsStop())
            {
                // 关闭服务器
                this->ShutDownServer();
                break;
            }
            const uint64_t timeout = s_redis_server_timeout.load(std::memory_order_acquire);
            char tmp[8192];
            ssize_t ret;
            if (timeout > 0)
            {
                ret = co_await sock->recvT(tmp, sizeof(tmp), 0, timeout);
            }
            else
            {
                ret = co_await sock->recv(tmp, sizeof(tmp));
            }
            if (ret <= 0)
            {
                if (ret == 0)
                {
                    BLUE_LOG_INFO(g_logger) << "[client " << sock->getSocketfd() << "] 正常关闭";
                    break;
                }
                if (errno == ETIMEDOUT)
                {
                    BLUE_LOG_WARN(g_logger) << "[client " << sock->getSocketfd()
                                            << "] timeout (" << util::ConfigParser::FormatTime(std::chrono::microseconds(timeout)) << "), closing";
                    break;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd()
                                         << "] recv error: " << strerror(errno);
                break;
            }

            if (!parser.feed({tmp, static_cast<size_t>(ret)}))
            {
                BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd()
                                         << "] 缓冲区溢出，关闭连接";
                break;
            }

            // 批量处理命令
            RespValue cmd_Resp;

            // 喂入数据
            while (parser.next(cmd_Resp))
            {
                auto copy_arr = cmd_Resp.arr;
                if (copy_arr.empty())
                {
                    continue;
                }
                // 安全检查
                if (cmd_Resp.type == RespValue::Type::ARRAY && copy_arr.size() > 1000)
                {
                    BLUE_LOG_WARN(g_logger) << "[client " << sock->getSocketfd()
                                            << "] 命令数组过大: " << copy_arr.size();
                    auto error_resp = RespValue::error("ERR command too large");
                    // batch_response += RespValue::encode(error_resp);
                    error_resp->encodeTo(batch_response);
                    break;
                }

                // 命令cmd
                std::string cmd = copy_arr[0].str;
                std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

                // blue::RespValue response;
                bool is_special = false; // 是否需要特殊处理

                auto start = SteadyClock::now();
                std::chrono::_V2::steady_clock::time_point end;
                if (sock->inTransaction()) // 事务模式
                {
                    is_special = true;
                    response = handleTransactionCommand(cmd, copy_arr, sock, start);
                }
                else if (sock->inSubScription()) // 订阅模式
                {
                    is_special = true;
                    response = handleSubscriptionCommand(cmd, copy_arr, sock, start);
                }
                else if (cmd == "MULTI" || cmd == "SUBSCRIBE") // MULTI, 进入事务模式, SUBSCRIBE channel [channel...] 订阅channel 进入订阅模式
                {
                    is_special = true;
                    response = handleModeSwitchCommand(cmd, copy_arr, sock, start);
                }
                else if (cmd == "PUBLISH") // PUBLISH channel message 发布channel 内容为message
                {
                    is_special = true;
                    response = handlePublishCommand(cmd, copy_arr, sock, start);
                }
                else
                {
                    // 收集命令
                    batch_commands.push_back(std::move(copy_arr));

                    // 达到阈值直接执行
                    if (batch_commands.size() >= s_max_exec_batch_size.load(std::memory_order_acquire))
                    {
                        co_await execute_and_encode(batch_commands);
                    }
                    continue;
                }

                std::string cmd_str;
                for (size_t i = 0; i < cmd_Resp.arr.size(); i++)
                {
                    if (i > 0)
                    {
                        cmd_str += " ";
                    }
                    cmd_str += cmd_Resp.arr[i].str;
                }

                if (is_special)
                {
                    auto end = SteadyClock::now();

                    // 记录慢查询
                    m_server->getSlowLog().pushEntry(cmd_str, sock, start, end);
                }
                if (response->str != "QUEUED" && m_server->getPushMonitor().load(std::memory_order_acquire))
                {
                    // 推送消息给监控客户端
                    m_server->getMonitor().pushToMonitor(cmd_str, sock);
                }

                // batch_response += RespValue::encode(response);
                response->encodeTo(batch_response);
                response->reset(); // 清空但保留容量
                cmd_count++;
                m_server->incrementCommands();

                if (!batch_response.empty())
                {
                    co_await send_response(batch_response);
                }
            }

            // 批量执行不够阈值的
            if (!batch_commands.empty())
            {
                co_await execute_and_encode(batch_commands);
            }

            // 发送剩余的响应
            if (!batch_response.empty())
            {
                co_await send_response(batch_response);
            }

            if (cmd_count > 0)
            {
                BLUE_LOG_DEBUGE(g_logger) << "Processed " << cmd_count
                                          << " commands in batch, batch_size="
                                          << batch_response.size();
            }

            // 检查缓冲区大小
            if (parser.bufferSize() > s_max_command_size.load(std::memory_order_acquire))
            {
                BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd()
                                         << "] 命令过大，关闭连接";
                break;
            }

        } while (true);

        // 检查是否是管理员连接断开
        auto admin = m_server->getAdminSocket().lock();
        if (admin && admin.get() == sock.get())
        {
            m_server->getAdminSocket().reset();
        }

        // 关闭前清理订阅
        m_server->getSubscription().removeAllSubscribers(sock);

        sock->clearSubScription();
        // BLUE_LOG_INFO(g_logger) << "one Client exit, fd:" << sock->getSocketfd();
        sock->close();

        // 删除过期的monitor
        m_server->getMonitor().removeMonitor();

        this->subConnection();
        if (this->getConnection() == 0 && m_server->getShutdown().load(std::memory_order_acquire))
        {
            bool end = co_await this->stop();
            if (end)
            {
                BLUE_LOG_INFO(g_logger) << "tcpserver stoped";
            }
        }
        co_return;
    }
#endif

    AutoRespValue CommandHandler::handleTransactionCommand(const std::string &cmd,
                                                           std::vector<RespValue> &args,
                                                           MSocket::MSocketPtr sock,
                                                           const TimePoint &start)
    {
        if (cmd == "EXEC") // EXEC
        {
            if (sock->getClientlevel() < 1)
            {
                return RespValue::error("ERR authentication required");
            }
            else if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'EXEC'");
            }
            else
            {
                sock->setVersionChecker([this, sock](const std::string &key) -> uint64_t
                                        { return m_server->getKeyVersion(key, sock); });
                if (sock->hasKeyModified())
                {
                    sock->clearTransaction();
                    sock->clearWatchedKey();
                    return RespValue::null_bulk();
                }
                else
                {
                    std::vector<RespValue> results;
                    for (const auto &transaction : sock->getTransaction())
                    {
                        auto response = m_table.executeTable(transaction, sock, m_server, true);
                        results.push_back(response);
                    }
                    sock->clearTransaction();
                    sock->clearWatchedKey();
                    return RespValue::array(std::move(results));
                }
            }
        }
        else if (cmd == "DISCARD") // DISCARD 清除所有事务,会退出事务模式
        {
            if (sock->getClientlevel() < 1)
            {
                return RespValue::error("ERR authentication required");
            }
            else if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'DISCARD'");
            }
            else
            {
                sock->clearTransaction();
                return RespValue::simple_string("OK");
            }
        }
        else
        {
            sock->addTransaction(std::move(args)); // move后args为空
            return RespValue::simple_string("QUEUED");
        }
    }

    AutoRespValue CommandHandler::handleSubscriptionCommand(const std::string &cmd,
                                                            std::vector<RespValue> &args,
                                                            MSocket::MSocketPtr sock,
                                                            const TimePoint &start)
    {
        if (cmd == "UNSUBSCRIBE") // UNSUBSCRIBE [channel...], 并退出订阅模式
        {
            std::vector<std::string> channels;
            if (sock->getClientlevel() < 1)
            {
                return RespValue::error("ERR authentication required");
            }
            else if (args.size() == 1) // 取消所有订阅
            {
                channels.assign(sock->getSubScriptionChannels().begin(), sock->getSubScriptionChannels().end());
            }
            else
            {
                for (size_t i = 1; i < args.size(); i++)
                {
                    channels.push_back(args[i].str);
                }
            }

            std::vector<RespValue> results;

            for (const auto &channel : channels)
            {
                // 从全局列表删除
                m_server->getSubscription().removeSubscriber(channel, sock);

                // 从连接订阅列表移除
                sock->removeSubScriptionChannel(channel);

                // 返回取消订阅消息
                std::vector<RespValue> msg;
                msg.push_back(*RespValue::bulk_string("unsubscribe"));
                msg.push_back(*RespValue::bulk_string(channel));
                msg.push_back(*RespValue::integer(sock->getSubScriptionChannels().size()));
                results.push_back(*RespValue::array(std::move(msg)));
            }
            if (sock->getSubScriptionChannels().size() == 0)
            {
                sock->endSubScription();
            }
            return RespValue::array(std::move(results));
        }
        else if (cmd == "PING") // PING [message]
        {
            if (sock->getClientlevel() < 1)
            {
                return RespValue::error("ERR authentication required");
            }
            else if (args.size() < 1 || args.size() > 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'PING'");
            }
            else
            {
                if (args.size() == 1)
                    return RespValue::simple_string("PONG");
                else
                    return RespValue::bulk_string(args[1].str);
            }
        }
        else
        {
            return RespValue::error("ERR in SubScription, only 'UNSUBSCRIBE' and 'PING'");
        }
    }

    AutoRespValue CommandHandler::handleModeSwitchCommand(const std::string &cmd,
                                                          std::vector<RespValue> &args,
                                                          MSocket::MSocketPtr sock,
                                                          const TimePoint &start)
    {
        if (cmd == "MULTI") // MULTI, 进入事务模式
        {
            if (sock->getClientlevel() < 1)
            {
                return RespValue::error("ERR authentication required");
            }
            else if (args.size() != 1)
            {
                return RespValue::error("ERR wrong number of arguments for 'MULTI'");
            }
            else if (!sock->beginTransaction())
            {
                return RespValue::error("ERR already in SubScription");
            }
            else
            {
                return RespValue::simple_string("OK");
            }
        }
        else if (cmd == "SUBSCRIBE") // SUBSCRIBE channel [channel...] 订阅channel 进入订阅模式
        {
            if (sock->getClientlevel() < 1)
            {
                return RespValue::error("ERR authentication required");
            }
            else if (args.size() < 2)
            {
                return RespValue::error("ERR wrong number of arguments for 'SUBSCRIBE'");
            }
            else if (!sock->beginSubScription())
            {
                return RespValue::error("ERR already in Transaction");
            }
            else
            {
                std::vector<RespValue> results;

                for (size_t i = 1; i < args.size(); i++)
                {
                    const std::string channel = args[i].str;
                    // 添加到连接的订阅列表
                    sock->addSubScriptionChannel(channel);

                    // 添加到全局订阅列表
                    m_server->getSubscription().addSubscriber(channel, sock);

                    // 返回订阅成功消息
                    std::vector<RespValue> msg;
                    msg.push_back(*RespValue::bulk_string("subscribe"));
                    msg.push_back(*RespValue::bulk_string(channel));
                    msg.push_back(*RespValue::integer(1)); // 当前订阅数

                    results.push_back(*RespValue::array(std::move(msg)));
                }
                return RespValue::array(std::move(results));
            }
        }
        return RespValue::error("ERR unknown mode command");
    }

    AutoRespValue CommandHandler::handlePublishCommand(const std::string &cmd,
                                                       std::vector<RespValue> &args,
                                                       MSocket::MSocketPtr sock,
                                                       const TimePoint &start)
    {
        if (sock->getClientlevel() < 1)
        {
            return RespValue::error("ERR authentication required");
        }
        else if (args.size() != 3)
        {
            return RespValue::error("ERR wrong number of arguments for 'PUBLISH'");
        }
        else
        {
            const std::string channel = args[1].str;
            const std::string message = args[2].str;

            // 获取订阅者并发送消息给所有订阅者
            int receiver_count = m_server->getSubscription().publishMessage(channel, message);

            return RespValue::integer(receiver_count);
        }
    }

    Generator<RespValue> CommandHandler::commandFlow(MSocket::MSocketPtr sock,
                                                     RespStreamParser &parser, const char *data, size_t size, bool &should_done)
    {

        if (!parser.feed({data, size}))
        {
            BLUE_LOG_ERROR(g_logger) << "[client " << sock->getSocketfd()
                                     << "] 缓冲区溢出，关闭连接";
            should_done = true;
            co_return;
        }

        RespValue cmd_Resp;

        while (parser.next(cmd_Resp))
        {
            auto copy_arr = cmd_Resp.arr;
            if (copy_arr.empty())
            {
                continue;
            }
            // 安全检查
            if (cmd_Resp.type == RespValue::Type::ARRAY && copy_arr.size() > 1000)
            {
                BLUE_LOG_WARN(g_logger) << "[client " << sock->getSocketfd()
                                        << "] 命令数组过大: " << copy_arr.size();
                co_yield *RespValue::error("ERR command too large");
                break;
            }

            // 命令cmd
            std::string cmd = copy_arr[0].str;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

            blue::RespValue response;
            bool is_special = false; // 是否需要特殊处理

            auto start = SteadyClock::now();
            std::chrono::_V2::steady_clock::time_point end;
            if (sock->inTransaction()) // 事务模式
            {
                is_special = true;
                response = *handleTransactionCommand(cmd, copy_arr, sock, start);
            }
            else if (sock->inSubScription()) // 订阅模式
            {
                is_special = true;
                response = *handleSubscriptionCommand(cmd, copy_arr, sock, start);
            }
            else if (cmd == "MULTI" || cmd == "SUBSCRIBE") // MULTI, 进入事务模式, SUBSCRIBE channel [channel...] 订阅channel 进入订阅模式
            {
                is_special = true;
                response = *handleModeSwitchCommand(cmd, copy_arr, sock, start);
            }
            else if (cmd == "PUBLISH") // PUBLISH channel message 发布channel 内容为message
            {
                is_special = true;
                response = *handlePublishCommand(cmd, copy_arr, sock, start);
            }
            else
            {
                response = m_table.executeTable(copy_arr, sock, m_server, true);
            }

            std::string cmd_str;
            for (size_t i = 0; i < cmd_Resp.arr.size(); i++)
            {
                if (i > 0)
                {
                    cmd_str += " ";
                }
                cmd_str += cmd_Resp.arr[i].str;
            }

            if (is_special)
            {
                auto end = SteadyClock::now();

                // 记录慢查询
                m_server->getSlowLog().pushEntry(cmd_str, sock, start, end);
            }
            if (response.str != "QUEUED" && m_server->getPushMonitor().load(std::memory_order_acquire))
            {
                // 推送消息给监控客户端
                m_server->getMonitor().pushToMonitor(cmd_str, sock);
            }

            co_yield response;
        }
    }
}