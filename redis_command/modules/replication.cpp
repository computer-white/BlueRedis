#include "blue/log.h"
#include "blue/io_manager.h"
#include "replication.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    void ReplicationModule::startReplication()
    {
        if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE)
        {
            return;
        }

        BLUE_LOG_INFO(g_logger) << "Starting replication to "
                                << m_repl_config.master_host << ":"
                                << m_repl_config.master_port;

        m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
        m_repl_queue_stop.store(false, std::memory_order_release);

        // 让上次的复制线程回来,然后启动新的
        if (m_relp_thread.joinable())
        {
            m_relp_thread.join();
        }

        m_relp_thread = std::thread([this]()
                                    { this->replicationLoop(); });

        // 启动消费者协程
        // static bool consumer_started = false;
        if (!m_consumer_started.exchange(true, std::memory_order_acq_rel))
        {
            // consumer_started = true;
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
            // blue::IOManager::GetThis()->schedule([this]() -> Task<void>
            //                                      {
            //     co_await this->processReplQueue();
            //     co_return; });
        }
    }

    void ReplicationModule::stopReplication()
    {
        BLUE_LOG_INFO(g_logger) << "Stopping replication";

        m_repl_state.store(REPL_STATE_NONE, std::memory_order_release);
        m_repl_queue_stop.store(true, std::memory_order_release);
        m_consumer_started.store(false, std::memory_order_release);
        m_repl_queue_cv.notify_all();

        if (m_repl_sock)
        {
            m_repl_sock->close();
            m_repl_sock.reset();
        }

        if (m_relp_thread.joinable())
        {
            m_relp_thread.join();
        }

        // 清空队列
        std::lock_guard<std::mutex> lock(m_repl_queue_mutex);
        while (!m_repl_queue.empty())
        {
            m_repl_queue.pop();
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
        while (m_repl_state.load(std::memory_order_acquire) != REPL_STATE_NONE && !m_server_stop.load(std::memory_order_acquire))
        {
            if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_NONE)
            {
                break;
            }

            // 连接主节点
            BLUE_LOG_INFO(g_logger) << "Connecting to master "
                                    << m_repl_config.master_host << ":"
                                    << m_repl_config.master_port;

            m_repl_state.store(REPL_STATE_CONNECTING, std::memory_order_release);
            std::string host_with_port = m_repl_config.master_host + ":" + std::to_string(m_repl_config.master_port);
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
            bool vailded = sock->setVaildFd(); // 使得fd有效
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
            m_repl_sock = sock;

            // 握手
            m_repl_state.store(REPL_STATE_HANDSHAKE, std::memory_order_release);
            if (!m_repl_config.master_password.empty())
            {
                // 构造消息
                std::vector<RespValue> auth_args;
                auth_args.push_back(RespValue::bulk_string("AUTH"));
                auth_args.push_back(RespValue::bulk_string(m_repl_config.master_password));
                std::string auth_cmd = RespValue::encode(RespValue::array(std::move(auth_args)));

                ssize_t send = ::send(sock->getSocketfd(), auth_cmd.data(), auth_cmd.size(), MSG_NOSIGNAL);
                if (send <= 0)
                {
                    BLUE_LOG_DEBUGE(g_logger) << "Failed to send AUTH";
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                char buf[128];
                ssize_t ret = ::recv(sock->getSocketfd(), buf, sizeof(buf), 0);
                if (ret <= 0)
                {
                    BLUE_LOG_DEBUGE(g_logger) << "Failed to recv AUTH response";
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                std::string resp(buf, ret);
                if (resp.find("+OK") == std::string::npos)
                {
                    BLUE_LOG_ERROR(g_logger) << "AUTH failed: " << resp;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }
                BLUE_LOG_INFO(g_logger) << "AUTH successful";
            }

            // 发送SYNC命令（需要携带密码发送）
            const char *sync_cmd = "*2\r\n$4\r\nAUTH\r\n$9\r\nclient123\r\n*1\r\n$4\r\nSYNC\r\n";
            ssize_t send = ::send(sock->getSocketfd(), sync_cmd, strlen(sync_cmd), MSG_NOSIGNAL);
            if (send <= 0)
            {
                BLUE_LOG_DEBUGE(g_logger) << "Failed to send SYNC";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

            BLUE_LOG_INFO(g_logger) << "Send SYNC, wait for RDB...";

            // 接收rdb回复
            // 格式: $<length>\r\n<data>
            m_repl_state.store(REPL_STATE_TRANSFER, std::memory_order_release);

            char buf[8192];
            std::string rdb_data;
            bool reading_length = true;
            size_t rdb_length = 0;
            size_t rdb_received = 0;
            bool rdb_error = false;

            while (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_TRANSFER)
            {
                ssize_t ret = ::recv(sock->getSocketfd(), buf, sizeof(buf) - 1, 0);
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

                if (reading_length)
                {
                    if (data[0] == '$')
                    {
                        size_t pos = data.find("\r\n");
                        if (pos != std::string::npos)
                        {
                            try
                            {
                                rdb_length = std::stoull(data.substr(1, pos - 1));
                            }
                            catch (...)
                            {
                                BLUE_LOG_ERROR(g_logger) << "Invalid RDB length";
                                rdb_error = true;
                                break;
                            }
                            rdb_data = data.substr(pos + 2);
                            rdb_received = rdb_data.size();
                            reading_length = false;

                            BLUE_LOG_INFO(g_logger) << "RDB length: " << rdb_length;

                            if (rdb_received >= rdb_length)
                            {
                                break; // 数据完整
                            }
                        }
                        else
                        {
                            // 数据不完整继续接收
                            continue;
                        }
                    }
                    else
                    {
                        BLUE_LOG_ERROR(g_logger) << "Invalid RDB format, expected '$', got '" << data[0] << "'";
                        rdb_error = true;
                        break;
                    }
                }
                else
                {
                    rdb_data += data;
                    rdb_received += ret;
                    // 完整数据
                    if (rdb_received >= rdb_length)
                    {
                        break;
                    }
                }
            }

            // 有错误或不完整数据
            if (rdb_error || rdb_received < rdb_length)
            {
                BLUE_LOG_ERROR(g_logger) << "RDB transfer failed";
                sock->close();
                m_repl_sock.reset();
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

            if (!rdb_data.empty())
            {
                BLUE_LOG_INFO(g_logger) << "RDB received: " << rdb_received << " bytes";
                // 加载 RDB 数据
                if (!loadRDBFromMemory(rdb_data))
                {
                    BLUE_LOG_ERROR(g_logger) << "Failed to load RDB data";
                    sock->close();
                    m_repl_sock.reset();
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }
                BLUE_LOG_INFO(g_logger) << "RDB loaded successfully";
            }

            // 进入在线模式
            m_repl_state.store(REPL_STATE_ONLINE, std::memory_order_release);
            BLUE_LOG_INFO(g_logger) << "Replication online";

            // 非阻塞接收数据
            std::string buffer;
            sock->setNoBlocking();
            while (m_repl_state.load(std::memory_order_acquire) == REPL_STATE_ONLINE && !m_server_stop.load(std::memory_order_acquire))
            {
                ssize_t ret = ::recv(sock->getSocketfd(), buf, sizeof(buf), 0);
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

                // 解析
                buffer.append(buf, ret);
                // BLUE_LOG_INFO(g_logger) << "buffer: " << buffer;

                RespStreamParser temp_parser;
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

                            m_repl_config.repl_offset++;
                        }
                    }
                }
                else
                {
                    if (buffer.size() > 1024 * 1024)
                    {
                        BLUE_LOG_ERROR(g_logger) << "Buffer too large, protocol error";
                        break;
                    }
                    BLUE_LOG_ERROR(g_logger) << "will break, feed error";
                    break;
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
        std::shared_lock<std::shared_mutex> lock(m_slaves_mutex);

        if (m_slaves.empty())
        {
            BLUE_LOG_INFO(g_logger) << "slaves is empty";
            return;
        }

        for (auto it = m_slaves.begin(); it != m_slaves.end();)
        {
            // 拿到MSocketPtr
            auto slave = it->lock();
            if (!slave || !slave->isConnected())
            {
                it = m_slaves.erase(it);
                continue;
            }

            // 非阻塞同步发送
            slave->setNoBlocking();
            ssize_t sent = ::send(slave->getSocketfd(), cmd.data(), cmd.size(), MSG_NOSIGNAL);
            if (sent <= 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    BLUE_LOG_WARN(g_logger) << "Send buffer full, will retry";
                    ++it;
                    continue;
                }
                BLUE_LOG_WARN(g_logger) << "Failed to send command to slave";
                lock.unlock();
                remove(slave);
                lock.lock();
                it = m_slaves.begin(); // 重新开始
                continue;
            }
            ++it;
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

                m_executor(std::move(cmd.args), temp_sock, false);
            }

            // 挂起,让出cpu
            // co_await std::suspend_always{};  // 不能挂起，因为没人会再去恢复它
        }

        BLUE_LOG_INFO(g_logger) << "Replication queue consumer stopped";
        co_return;
    }

    bool ReplicationModule::loadRDBFromMemory(const std::string &data)
    {
        BLUE_LOG_INFO(g_logger) << "Loading RDB from memory, size=" << data.size();

        RespStreamParser parser;
        if (!parser.feed(data))
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to parse RDB data";
            return false;
        }

        // 创建临时 socket 用于加载（不需要认证）
        auto temp_sock = MSocket::CreateTcpSocket();
        temp_sock->setClientlevel(1);
        temp_sock->setClientId(0);

        int count = 0;
        RespValue cmd;
        while (parser.next(cmd))
        {
            if (cmd.type == RespValue::Type::ARRAY && !cmd.arr.empty())
            {
                // 执行命令（不记录 AOF，不推送Monitor）
                m_executor(std::move(cmd.arr), temp_sock, false);
                count++;
            }
        }

        BLUE_LOG_INFO(g_logger) << "Loaded " << count << " commands from RDB";
        return true;
    }

    void ReplicationModule::remove(MSocket::MSocketPtr sock)
    {
        std::unique_lock<std::shared_mutex> lock(m_slaves_mutex);
        auto it = std::find_if(m_slaves.begin(), m_slaves.end(),
                               [sock](const auto &weak)
                               {
                                   auto ptr = weak.lock();
                                   return ptr && ptr.get() == sock.get();
                               });
        if (it != m_slaves.end())
        {
            m_slaves.erase(it);
        }
    }

    std::string ReplicationModule::slavesToString() const noexcept
    {
        std::string result;
        uint32_t idx = 0;
        std::shared_lock<std::shared_mutex> lock(m_slaves_mutex);
        
        if (m_slaves.empty())
        {
            BLUE_LOG_INFO(g_logger) << "slaves empty";
            return result;
        }

        for (auto it = m_slaves.begin(); it != m_slaves.end(); )
        {
            if (it->expired())
            {
                continue;
            }
            auto ptr = it->lock();
            if (ptr)
            {
                // 格式: slave0:addr=127.0.0.1:6666state=online
                result += "slave" + std::to_string(idx++) + ":";
                result += "addr=" + ptr->getRemoteAddress()->toString();
                result += "state=online\r\n";
            }
            ++it;
        }
        return result;
    }

    size_t ReplicationModule::slavesCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_slaves_mutex);
        size_t count = 0;
        for (const auto &weak : m_slaves)
        {
            if (!weak.expired())
            {
                count++;
            }
        }
        return count;
    }
}