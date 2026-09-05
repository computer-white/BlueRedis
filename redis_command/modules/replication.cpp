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
#include "blue/log.h"
#include "blue/io_manager.h"
#include "blue/configinit.h"
#include "blue/configinit.h"
#include "replication.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    void ReplicationModule::startReplication()
    {
        // 只要上一个主从复制没有结束，就不能开启新的
        if (!m_repl_stop.load(std::memory_order_acquire))
        {
            return;
        }

        BLUE_LOG_INFO(g_logger) << "Starting replication to "
                                << s_repl_master_addr.load(std::memory_order_acquire) << ":"
                                << s_repl_master_port.load(std::memory_order_acquire);

        m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
        m_repl_queue_stop.store(false, std::memory_order_release);
        m_repl_stop.store(false, std::memory_order_release);

        // 让上次的复制线程回来,然后启动新的
        if (m_relp_thread.joinable())
        {
            m_relp_thread.join();
        }

        m_relp_thread = std::thread([this]()
                                    { this->replicationLoop(); });

        // 启动消费者协程;
        if (!m_consumer_started.exchange(true, std::memory_order_acq_rel))
        {
            BLUE_LOG_INFO(g_logger) << "Scheduling replication queue consumer";
            auto *iom = blue::IOManager::GetThis();
            if (!iom)
            {
                BLUE_LOG_ERROR(g_logger) << "IOManager is null!";
                return;
            }
            BLUE_LOG_INFO(g_logger) << "IOManager found: " << iom;
            iom->schedule(this->processReplQueue());
            BLUE_LOG_INFO(g_logger) << "Replication queue consumer scheduled";
        }
    }

    void ReplicationModule::stopReplication()
    {
        BLUE_LOG_INFO(g_logger) << "Stopping replication";

        m_repl_stop.store(true, std::memory_order_release);
        // 设置processReplQueue停止标识
        m_repl_queue_stop.store(true, std::memory_order_release);
        // 通知所有等待的线程
        m_repl_queue_cv.notify_all();

        // 先停止供应商
        // 停止复制线程
        if (m_repl_sock)
        {
            m_repl_sock->close();
            m_repl_sock.reset();
        }

        if (m_relp_thread.joinable())
        {
            m_relp_thread.join();
        }

        // 等待消费者协程停止
        while (m_consumer_started.load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        {
            // 清空消费者协程可能残留的任务队列
            std::lock_guard<std::mutex> lock(m_repl_queue_mutex);
            while (!m_repl_queue.empty())
            {
                m_repl_queue.pop();
            }
        }

        BLUE_LOG_INFO(g_logger) << "Replication stopped";
    }

    void ReplicationModule::addSlaves(MSocket::MSocketPtr sock)
    {
        BLUE_LOG_INFO(g_logger) << "add sock slave";
        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);
        m_slaves.push_back(sock);
    }

    void ReplicationModule::replicationLoop()
    {
        BLUE_LOG_INFO(g_logger) << "Replication Loop start";
        RespStreamParser parser;
        int retry_count = 0;
        while (!m_repl_stop.load(std::memory_order_acquire) && !m_server_stop.load(std::memory_order_acquire))
        {
            // 已经处于在线模式，就等待结束复制或服务器停止
            if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            // 重试还是连不上后直接退出
            if (retry_count == s_repl_retry_count.load(std::memory_order_acquire))
            {
                // stopReplication(); // 哈哈哈，遇到问题了，不能在线程运行中的函数中调用一个包含join这个线程的函数，线程死锁
                // 选择直接退出，如果后序有什么好的想法再来改
                break;
            }
            retry_count++;

            // 连接主节点
            const std::string master_addr = *s_repl_master_addr.load(std::memory_order_acquire);
            const std::string master_port = std::to_string(s_repl_master_port.load(std::memory_order_acquire));
            BLUE_LOG_INFO(g_logger) << "Connecting to master "
                                    << master_addr << ":"
                                    << master_port;

            m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
            std::string host_with_port = master_addr + ":" + master_port;
            auto addr = Address::LookupAnyIpAddress(host_with_port);
            if (!addr)
            {
                BLUE_LOG_DEBUGE(g_logger) << "Invalid master address";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            BLUE_LOG_INFO(g_logger) << "addr successful: " << addr->toString();
            auto sock = MSocket::CreateTcp(addr);
            if (!sock)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            BLUE_LOG_INFO(g_logger) << "sock successful";
            bool vailded = sock->setValidFd(); // 使得fd有效
            if (!vailded)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            BLUE_LOG_INFO(g_logger) << "sock vaild successful";
            sock->setBlocking();
            // 同步阻塞连接
            ssize_t conn = ::connect(sock->getSocketfd(), addr->getAddr(), addr->getAddrLen());
            if (conn != 0)
            {
                BLUE_LOG_ERROR(g_logger) << "connect failed: errno=" << errno
                                         << " (" << strerror(errno) << ")";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            BLUE_LOG_INFO(g_logger) << "connection successful";
            sock->setNoBlocking();
            sock->setConnection(); // 会标记sock fd为已经连接上，并拿到本端和远端地址
            BLUE_LOG_INFO(g_logger) << "Connected to master, fd=" << sock->getSocketfd();
            m_repl_sock = sock; // 引用计数加一

            // 握手
            {
                m_repl_state.store(REPL_STATE_HANDSHAKE, std::memory_order_release);
                // 构造消息
                std::vector<RespValue> auth_args;
                auth_args.push_back(*RespValue::bulk_string("AUTH"));
                if (!(s_repl_master_password.load(std::memory_order_acq_rel))->empty())
                {
                    auth_args.push_back(*RespValue::bulk_string(*s_repl_master_password.load(std::memory_order_acquire)));
                }
                else
                {
                    auth_args.push_back(*RespValue::bulk_string("client123"));
                }
                std::string auth_cmd = RespValue::encode(*RespValue::array(std::move(auth_args)));
                std::vector<RespValue> sync_args;
                sync_args.push_back(*RespValue::bulk_string("SYNC"));
                std::string sync_cmd = RespValue::encode(*RespValue::array(std::move(sync_args)));
                std::string cmd = auth_cmd + sync_cmd;
                ssize_t send = ::send(sock->getSocketfd(), cmd.data(), cmd.size(), MSG_NOSIGNAL);
                if (send <= 0)
                {
                    BLUE_LOG_DEBUGE(g_logger) << "Failed to send AUTH and SYNC";
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }
                BLUE_LOG_INFO(g_logger) << "Send SYNC, wait for RDB...";
            }

            // 流式接收RDB数据并同步到本节点
            {
                // 格式: $<length>\r\n<data>
                m_repl_state.store(REPL_STATE_TRANSFER, std::memory_order_release);

                // 有错误，重新连接
                if (!loadRDBFromMemory(sock) && m_repl_state.load(std::memory_order_acquire) == REPL_STATE_RETRY)
                {
                    sock->close();
                    m_repl_sock.reset();
                    m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                // 加载完没有错误可能是停止了主从复制或服务器停止了
                if (m_repl_stop.load(std::memory_order_acquire) || m_server_stop.load(std::memory_order_acquire))
                {
                    break;
                }
            }

            // 在线模式，持续接收来自主节点的广播写命令
            {
                // 进入在线模式
                m_repl_state.store(REPL_STATE_ONLINE, std::memory_order_release);
                BLUE_LOG_INFO(g_logger) << "Replication online";

                // 非阻塞接收数据
                sock->setNoBlocking();
                RespStreamParser temp_parser;
                while (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE &&
                    !m_server_stop.load(std::memory_order_acquire) &&
                    !m_repl_stop.load(std::memory_order_acquire))
                {
                    char buf[8192];
                    ssize_t ret = ::recv(sock->getSocketfd(), buf, sizeof(buf), MSG_NOSIGNAL);
                    if (ret <= 0)
                    {
                        if (ret == 0)
                        {
                            BLUE_LOG_INFO(g_logger) << "Master closed connection";
                            break;
                        }
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // 没有数据，等待一下
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            continue;
                        }
                        BLUE_LOG_ERROR(g_logger) << "recv error: " << strerror(errno);
                        break;
                    }

                    std::string buffer(buf, ret);
                    // BLUE_LOG_INFO(g_logger) << "buffer: " << buffer;
                    // 解析命令
                    if (temp_parser.feed(buffer))
                    {
                        RespValue cmd;
                        while (temp_parser.next(cmd))
                        {
                            // BLUE_LOG_INFO(g_logger) << "next successful";
                            if (cmd.type == RespValue::Type::ARRAY && !cmd.arr.empty())
                            {
                                {
                                    // BLUE_LOG_INFO(g_logger) << "push to repl_queue";
                                    std::lock_guard<std::mutex> lock(m_repl_queue_mutex);
                                    m_repl_queue.push(ReplCommand{std::move(cmd.arr)});
                                }
                                // BLUE_LOG_INFO(g_logger) << "notify one";
                                m_repl_queue_cv.notify_one();
                            }
                        }
                    }
                    else
                    {
                        BLUE_LOG_ERROR(g_logger) << "feed error, buffer.size(): " << buffer.size() << "overflow";
                        break;
                    }
                }
            }

            // 断开连接，等待重连
            sock->close();
            m_repl_sock.reset();
            m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
            BLUE_LOG_INFO(g_logger) << "Disconnected from master, reconnecting in 1s...";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        BLUE_LOG_INFO(g_logger) << "Replication loop ended";
    }

    void ReplicationModule::broadcastToSlaves(const std::string &cmd)
    {
        BLUE_LOG_INFO(g_logger) << "broadcast" << cmd << "to slaves";

        std::vector<MSocket::MSocketPtr> to_remove;

        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);

        if (m_slaves.empty())
        {
            BLUE_LOG_INFO(g_logger) << "slaves is empty";
            return;
        }

        for (auto it = m_slaves.begin(); it != m_slaves.end();)
        {
            if (it->expired())
            {
                it = m_slaves.erase(it);
                continue;
            }

            // 拿到MSocketPtr
            auto slave = it->lock();
            if (!slave || !slave->isConnected())
            {
                to_remove.push_back(slave);
                ++it;
                continue;
            }

            // 非阻塞同步发送
            slave->setNoBlocking();
            ssize_t sent = ::send(slave->getSocketfd(), cmd.data(), cmd.size(), MSG_NOSIGNAL);
            if (sent <= 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                BLUE_LOG_WARN(g_logger) << "Failed to send command to slave";
                to_remove.push_back(slave);
            }
            ++it;
        }

        lock.unlock();

        for (auto &slave : to_remove)
        {
            this->remove(slave);
        }
    }

    Task<void> ReplicationModule::processReplQueue()
    {
        BLUE_LOG_INFO(g_logger) << "Replication queue consumer started";

        // 刚开始默认数据库0，后序跟随主节点进行实现命令，包括select 切换数据库，并且后序协程会跑在while循环内，也就是
        // 说不会因为默认数据库0而造成开启主从复制后的数据不一致

        // 创建临时的socket对象(没有真正调用系统API创建socket fd)来实现复制
        auto temp_sock = MSocket::CreateTcpSocket();
        temp_sock->setClientlevel(1); // 跳过认证检查
        temp_sock->setClientId(0);    // 默认数据库 0

        while (!m_repl_queue_stop.load(std::memory_order_acquire) && !m_server_stop.load(std::memory_order_acquire))
        {
            ReplCommand cmd;
            bool has_cmd = false;

            {
                std::unique_lock<std::mutex> lock(m_repl_queue_mutex);
                m_repl_queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this]()
                                         { return !m_repl_queue.empty() ||
                                                  m_repl_queue_stop.load(std::memory_order_acquire) ||
                                                  m_server_stop.load(std::memory_order_acquire); });

                if (m_repl_queue_stop.load(std::memory_order_acquire) ||
                    m_server_stop.load(std::memory_order_acquire))
                {
                    break;
                }

                if (!m_repl_queue.empty())
                {
                    cmd = std::move(m_repl_queue.front());
                    m_repl_queue.pop();
                    has_cmd = true;
                }
            }

            if (has_cmd)
            {
                // 执行命令（不记录 AOF, 不推送Monitor）
                // BLUE_LOG_INFO(g_logger) << "has_cmd, m_executor: " << (m_executor ? "exists" : "null");

                try
                {
                    m_executor(std::move(cmd.args), temp_sock, false);
                    s_repl_offset.fetch_add(1, std::memory_order_acq_rel);
                }
                catch (const std::exception &e)
                {
                    BLUE_LOG_ERROR(g_logger) << "Executor threw exception: " << e.what();
                }
            }

            // 挂起,让出cpu
            // co_await std::suspend_always{};  // 不能挂起，因为没人会再去恢复它
        }

        BLUE_LOG_INFO(g_logger) << "Replication queue consumer stopped";
        m_consumer_started.store(false, std::memory_order_release); // 消费者协程停止
        co_return;
    }

    Generator<std::string> ReplicationModule::recvRDBData(std::shared_ptr<MSocket> sock)
    {
        std::string rdb_data;
        bool reading_length = true;
        size_t rdb_length = 0;
        size_t rdb_received = 0;
        bool rdb_error = false;
        while (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_TRANSFER &&
               !m_repl_stop.load(std::memory_order_acquire) &&
               !m_server_stop.load(std::memory_order_acquire))
        {
            char buf[8 * 1024];
            sock->setNoBlocking();
            ssize_t ret = ::recv(sock->getSocketfd(), buf, sizeof(buf) - 1, MSG_NOSIGNAL);
            if (ret <= 0)
            {
                if (ret == 0)
                {
                    BLUE_LOG_ERROR(g_logger) << "Master closed connection during RDB transfer";
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // BLUE_LOG_DEBUGE(g_logger) << "errno = EAGIN";
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                else
                {
                    BLUE_LOG_ERROR(g_logger) << "recv error: " << strerror(errno);
                }
                rdb_error = true;
                break;
            }

            buf[ret] = '\0';
            std::string data(buf, ret);
            // BLUE_LOG_INFO(g_logger) << "data: " << data;

            // 处理来自主节点的SYNC命令的回复
            while (data.substr(0, 5) == "+OK\r\n") // 可能包含SYNC或AUTH命令的OK
            {
                // +OK\r\n
                data = data.erase(0, 5);
            }

            // BLUE_LOG_INFO(g_logger) << "removed '+OK', data: " << data;
            bool stop = (data.find("EOF") != std::string::npos);
            co_yield std::move(data);
            if (stop)
            {
                break;
            }
        }
        if (rdb_error)
        {
            m_repl_state.store(REPL_STATE_RETRY, std::memory_order_release);
        }
        BLUE_LOG_INFO(g_logger) << "recvRDBData finish!";
    }

    bool ReplicationModule::loadRDBFromMemory(std::shared_ptr<MSocket> sock)
    {
        // 创建临时 socket 用于加载（不需要认证）
        auto temp_sock = MSocket::CreateTcpSocket();
        temp_sock->setClientlevel(1);
        temp_sock->setClientId(0);

        int count = 0;
        RespStreamParser parser(s_max_command_size.load(std::memory_order_acquire));
        auto generator = this->recvRDBData(sock);
        for (const auto &data : generator)
        {
            BLUE_LOG_INFO(g_logger) << "Loading RDB from memory, size=" << data.size();
            if (!parser.feed(data))
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to parse RDB data";
                return false;
            }

            RespValue cmd;
            while (parser.next(cmd))
            {
                if (cmd.type == RespValue::Type::ARRAY && !cmd.arr.empty())
                {
                    try
                    {
                        // 执行命令（不记录 AOF，不推送Monitor）
                        m_executor(std::move(cmd.arr), temp_sock, false);
                        count++;
                    }
                    catch (const std::exception &e)
                    {
                        BLUE_LOG_ERROR(g_logger) << "Executor threw exception: " << e.what();
                    }
                }
            }
        }
        BLUE_LOG_INFO(g_logger) << "Loaded " << count << " commands from RDB";
        if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_RETRY)
        {
            return false;
        }
        return true;
    }

    void ReplicationModule::remove(MSocket::MSocketPtr sock)
    {
        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);
        m_slaves.erase(std::remove_if(m_slaves.begin(), m_slaves.end(),
                                      [sock](const auto &weak)
                                      {
                                          auto ptr = weak.lock();
                                          return !ptr || ptr.get() == sock.get();
                                      }),
                       m_slaves.end());
    }

    std::string ReplicationModule::slavesToString()
    {
        std::string result;
        uint32_t idx = 0;
        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);
        if (m_slaves.empty())
        {
            BLUE_LOG_INFO(g_logger) << "slaves empty";
            return result;
        }

        for (auto it = m_slaves.begin(); it != m_slaves.end();)
        {
            if (it->expired())
            {
                it = m_slaves.erase(it);
                continue;
            }
            auto ptr = it->lock();
            if (ptr)
            {
                // 格式: slave0:ip=127.0.0.1,port=6666,state=online,offset=12345,lag=0
                auto addr = ptr->getRemoteAddress();
                if (addr)
                {
                    auto ip_addr = std::dynamic_pointer_cast<blue::IPAddress>(addr);
                    if (ip_addr)
                    {
                        result += "slave" + std::to_string(idx++) + ":";
                        result += "ip=" + ip_addr->getIp() + ",";
                        result += "port=" + std::to_string(ip_addr->getPort()) + ",";
                        result += "state=online,";
                        result += "offset=" + std::to_string(s_repl_offset.load(std::memory_order_acquire)) + ",";
                        result += "lag=0\r\n";
                    }
                }
            }
            ++it;
        }
        return result;
    }

    size_t ReplicationModule::slavesCount()
    {
        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);
        for (auto it = m_slaves.begin(); it != m_slaves.end();)
        {
            if (it->expired())
            {
                it = m_slaves.erase(it);
                continue;
            }
            ++it;
        }
        return m_slaves.size();
    }
}