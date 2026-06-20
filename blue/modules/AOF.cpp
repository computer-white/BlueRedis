#include "AOF.h"
#include "blue/io_manager.h"
#include "blue/macro.h"
#include "blue/log.h"
#include "blue/await.h"

namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    void AOFModule::initAOF()
    {
        if (!m_aof_config.aof_enabled)
        {
            return;
        }

        if (m_aof_file_idx == 0)
        {
            m_aof_file_idx = 1;
            m_aof_current_filename = getAOFFilename(1);
        }

        m_aof_file.open(m_aof_current_filename, std::ios::app | std::ios::binary);
        if (!m_aof_file)
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to open AOF file: " << m_aof_current_filename;
            m_aof_config.aof_enabled = false;
            return;
        }

        BLUE_LOG_INFO(g_logger) << "AOF enabled, file: " << m_aof_current_filename;

        // 启动 AOF 刷新线程
        startAOFFlushThread();

        // 启动 AOF 同步协程(always 或 everysec)
        IOManager::GetThis()->schedule(aofSyncLoop());
    }

    void AOFModule::appendToAOF(const std::string &cmd)
    {
        if (!m_aof_config.aof_enabled || !m_aof_file.is_open())
        {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_aof_buffer.aof_mutex);
            m_aof_buffer.aof_buffer += cmd;
            m_aof_buffer.aof_buffer_size += cmd.size();
        }

        if (m_aof_buffer.aof_buffer_size.load(std::memory_order_acquire) > m_aof_max_buffer_size)
        {
            m_aof_buffer.aof_cv.notify_one();
        }
    }

    void AOFModule::loadAOF()
    {
        // 从所有现有的AOF文件加载
        std::vector<std::string> files;

        // 提前分配
        files.reserve(m_aof_config.aof_max_file_number);

        int goodidx = 1;
        int idx = 1;
        // 遍历1-max_file_number之间的文件索引
        for (; idx <= m_aof_config.aof_max_file_number; idx++)
        {
            std::string file_name = getAOFFilename(idx);
            std::ifstream test(file_name);
            if (test && test.good())
            {
                files.push_back(file_name);
                goodidx = idx;
                test.close();
            }
        }

        if (files.empty())
        {
            BLUE_LOG_INFO(g_logger) << "No existing AOF file";
            m_aof_file_idx = 1;
            m_aof_current_filename = getAOFFilename(1);
            return;
        }

        m_aof_file_idx = goodidx; // 必然有效(1-max_file_number)
        m_aof_current_filename = getAOFFilename(m_aof_file_idx);

        // 创建临时的socket对象(没有真正调用系统API创建socket fd)来载入
        auto temp_sock = MSocket::CreateTcpSocket();
        temp_sock->setClientlevel(1); // 跳过认证检查
        temp_sock->setClientId(0);    // 默认数据库 0

        int total_cmds = 0;
        int errors = 0;

        for (const auto &file_name : files)
        {
            std::ifstream file(file_name);
            if (!file)
            {
                BLUE_LOG_INFO(g_logger) << "No existing AOF file";
                continue;
            }

            BLUE_LOG_INFO(g_logger) << "Loading AOF: " << file_name;

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            file.close();

            if (content.empty())
            {
                BLUE_LOG_INFO(g_logger) << "AOF file is empty";
                continue;
            }

            RespStreamParser parser;

            if (!parser.feed(content))
            {
                BLUE_LOG_INFO(g_logger) << "Failed to parser AOF file";
                continue;
            }

            RespValue cmd_Resp;
            int count = 0;
            while (parser.next(cmd_Resp))
            {
                auto copy_arr = cmd_Resp.arr;
                if (copy_arr.empty() || cmd_Resp.type != RespValue::Type::ARRAY)
                {
                    continue;
                }
                try
                {
                    // 只载入数据,不计入AOF文件
                    if (m_executor)
                    {
                        m_executor(std::move(copy_arr), temp_sock, false);
                        count++;
                    }
                    else
                    {
                        BLUE_LOG_ERROR(g_logger) << "Executor not set";
                        errors++;
                    }
                }
                catch (const std::exception &e)
                {
                    BLUE_LOG_ERROR(g_logger) << "Failed to execute AOF command: " << e.what();
                    errors++;
                }
            }
            total_cmds += count;
            BLUE_LOG_INFO(g_logger) << "Loaded " << count << " commands from AOF " << file_name;
        }

        BLUE_LOG_INFO(g_logger) << "AOF loaded: " << total_cmds
                                    << " commands, errors: " << errors;
    }

    bool AOFModule::isWriteCommand(const std::string &cmd)
    {
        static const std::unordered_set<std::string> write_commands = {
            "SET", "SETNX", "MSET", "APPEND", "GETSET", "INCR", "INCRBY",
            "HSET", "HDEL",
            "LPUSH", "RPUSH", "LPOP", "RPOP", "LSET", "LINSERT", "LPOPRPUSH", "RPOPLPUSH",
            "SADD", "SREM", "SMOVE", "SPOP",
            "ZADD", "ZREM", "ZINCRBY", "ZREMRANGEBYSCORE", "ZINCRBYFLOAT",
            "DEL", "EXPIRE", "PEXPIRE", "PERSIST", "RENAME", "RENAMENX",
            "FLUSHDB", "FLUSHDBALL",
            // 处理select 来保证数据库会被正确初始化
            "SELECT"};
        // BLUE_LOG_INFO(g_logger) << "cmd: " << cmd << "inwrite_commands: " << (int)(write_commands.find(cmd) != write_commands.end());
        return (bool)(write_commands.find(cmd) != write_commands.end());
    }

    std::string AOFModule::formatCommand(const std::vector<RespValue> &args)
    {
        std::string result;
        result += "*" + std::to_string(args.size()) + "\r\n";
        for (const auto &arg : args)
        {
            result += "$" + std::to_string(arg.str.size()) + "\r\n";
            result += arg.str + "\r\n";
        }
        return result;
    }

    Task<void> AOFModule::aofSyncLoop()
    {
        if (!m_aof_config.aof_enabled)
        {
            co_return;
        }

        while (!m_stop.load(std::memory_order_acquire))
        {
            co_await sleepFor(2);

            if (m_aof_config.aof_sync == "everysec") // 每秒刷新
            {
                std::unique_lock<std::shared_mutex> lock(m_aof_mutex);
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_aof_sync).count();

                if (duration >= 1 && m_aof_file.is_open())
                {
                    m_aof_file.flush();
                    m_last_aof_sync = now;
                }
            }
            else if (m_aof_config.aof_sync == "always")
            {
                if (m_aof_file.is_open())
                {
                    m_aof_file.flush();
                }
            }
        }
        co_return;
    }

    void AOFModule::rotateAOF()
    {
        if (m_aof_config.aof_max_file_number == 0)
        {
            BLUE_LOG_ERROR(g_logger) << "max_file_number is 0, cannot rotate";
            return;
        }
        // 处在轮转中
        if (m_aof_rotating.load(std::memory_order_acquire))
        {
            BLUE_LOG_DEBUGE(g_logger) << "AOF rotation already in progress";
            return;
        }

        // 开始轮转
        m_aof_rotating.store(true, std::memory_order_release);

        BLUE_LOG_INFO(g_logger) << "AOF rotation started, current file: "
                                    << m_aof_current_filename
                                    << ", idx: " << m_aof_file_idx
                                    << ", max_file_number: " << m_aof_config.aof_max_file_number;

        std::unique_lock<std::shared_mutex> lock(m_aof_mutex);
        if (m_aof_file.is_open())
        {
            m_aof_file.flush();
            m_aof_file.close();
        }

        // 让索引落在1-max_file_number之间
        int next_idx = (m_aof_file_idx % m_aof_config.aof_max_file_number) + 1;

        // 索引文件会在1-max_file_number之间循环,所以当返回一个已经存在的文件名，表示需要删除了
        std::string new_file = getAOFFilename(next_idx);

        // 调用后,new_file文件名一定可以使用了,因为要么是旧的被删除了(可能删除失败),要么是新的
        bool can_used = cleanupOldAOFs(new_file);

        // 如果删除文件失败,截断文件内容后以app方式打开
        if (!can_used)
        {
            std::ofstream truncate_file(new_file, std::ios::trunc | std::ios::binary);
            if (truncate_file)
            {
                truncate_file.close();
                BLUE_LOG_INFO(g_logger) << "Truncated old AOF file: " << new_file;
            }
            else
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to truncate old AOF file: " << new_file;
                m_aof_config.aof_enabled = false;
                m_aof_rotating.store(false, std::memory_order_release);
                return;
            }
        }

        m_aof_file.open(new_file, std::ios::app | std::ios::binary);
        if (!m_aof_file)
        {
            BLUE_LOG_ERROR(g_logger) << "Failed to open new AOF file: " << new_file;
            m_aof_config.aof_enabled = false;
            m_aof_rotating.store(false, std::memory_order_release);
            return;
        }

        m_aof_file_idx = next_idx;         // 更新当前的文件索引
        m_aof_current_filename = new_file; // 更新当前的文件名
        m_aof_file.flush();
        lock.unlock();

        // 轮转结束
        m_aof_rotating.store(false, std::memory_order_release);
        BLUE_LOG_INFO(g_logger) << "AOF rotation completed, new file: " << new_file;
    }

    std::string AOFModule::getAOFFilename(int index)
    {
        if (index == 1)
        {
            return m_aof_config.aof_filename;
        }
        return m_aof_config.aof_filename + "." + std::to_string(index);
    }

    bool AOFModule::cleanupOldAOFs(const std::string &filename)
    {
        // 检查文件是否存在
        std::ifstream test(filename);
        if (test.good())
        {
            test.close();
            if (remove(filename.c_str()) == 0)
            {
                BLUE_LOG_INFO(g_logger) << "Removed old AOF file: " << filename;
            }
            else
            {
                BLUE_LOG_ERROR(g_logger) << "Failed to remove old AOF file: " << filename;
                return false;
            }
        }
        return true;
    }

    void AOFModule::startAOFFlushThread()
    {
        if (m_aof_flush_running.load(std::memory_order_acquire))
        {
            return;
        }

        // 开始异步刷新
        m_aof_flush_running.store(true, std::memory_order_release);

        m_aof_flush_thread = std::thread([this]()
                                         { aofFlushThread(); });

        BLUE_LOG_INFO(g_logger) << "AOF flush thread started";
    }

    void AOFModule::stopAOFFlushThread()
    {
        if (!m_aof_flush_running.load(std::memory_order_acquire))
        {
            return;
        }

        // 停止异步刷新
        m_aof_flush_running.store(false, std::memory_order_release);
        m_aof_buffer.aof_cv.notify_all();
        if (m_aof_flush_thread.joinable())
        {
            m_aof_flush_thread.join();
        }
        // 最后刷新一次
        flushAOFBuffer();
        BLUE_LOG_INFO(g_logger) << "AOF flush thread stopped";
    }

    void AOFModule::aofFlushThread()
    {
        while (m_aof_flush_running.load(std::memory_order_acquire) && !m_stop.load(std::memory_order_acquire))
        {
            std::unique_lock<std::mutex> lock(m_aof_buffer.aof_mutex);

            m_aof_buffer.aof_cv.wait_for(lock, std::chrono::milliseconds(100), [this]()
                                         { return m_aof_buffer.aof_buffer_size.load(std::memory_order_acquire) > 0 
                                            || !m_aof_flush_running.load(std::memory_order_acquire) 
                                            || m_stop.load(std::memory_order_acquire); });

            if (m_aof_buffer.aof_buffer_size.load(std::memory_order_acquire) == 0)
            {
                continue;
            }

            // 取出缓冲区内容

            BLUE_ASSERT(m_aof_buffer.aof_buffer.size() == m_aof_buffer.aof_buffer_size.load(std::memory_order_acquire));
            std::string data_to_write = std::move(m_aof_buffer.aof_buffer);
            size_t data_size = data_to_write.size();
            m_aof_buffer.aof_buffer.clear();
            m_aof_buffer.aof_buffer_size.store(0, std::memory_order_release);
            lock.unlock();

            // 实际写入文件
            std::unique_lock<std::shared_mutex> file_lock(m_aof_mutex);
            if (m_aof_file.is_open())
            {
                m_aof_file << data_to_write;

                if (m_aof_config.aof_sync == "always")
                {
                    m_aof_file.flush();
                }
                else if (m_aof_config.aof_sync == "everysec")
                {
                    auto now = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                                        now - m_last_aof_sync)
                                        .count();
                    if (duration >= 1)
                    {
                        m_aof_file.flush();
                        m_last_aof_sync = now;
                    }
                }

                size_t curr_size = m_aof_file.tellp();
                if (curr_size > m_aof_config.aof_max_file_size && !m_aof_rotating.load(std::memory_order_acquire))
                {
                    file_lock.unlock();
                    rotateAOF();
                }
            }
            file_lock.unlock();

            BLUE_LOG_DEBUGE(g_logger) << "AOF flushed " << data_size << " bytes";
        }
    }

    void AOFModule::flushAOFBuffer()
    {
        std::unique_lock<std::mutex> lock(m_aof_buffer.aof_mutex);
        if (m_aof_buffer.aof_buffer_size.load(std::memory_order_acquire) == 0)
        {
            return;
        }

        BLUE_ASSERT(m_aof_buffer.aof_buffer.size() == m_aof_buffer.aof_buffer_size.load(std::memory_order_acquire));
        std::string data_to_write = std::move(m_aof_buffer.aof_buffer);
        m_aof_buffer.aof_buffer.clear();
        m_aof_buffer.aof_buffer_size.store(0, std::memory_order_release);
        lock.unlock();

        std::unique_lock<std::shared_mutex> file_lock(m_aof_mutex);
        if (m_aof_file.is_open())
        {
            m_aof_file << data_to_write;
            m_aof_file.flush();
        }
    }
}