#pragma once
#include <optional>
#include "command_handler_base.h"
#include "redis_command/modules/replication.h"

namespace blue
{
    template <typename T>
    class CommandHandlerIfelse : public CommandHandlerBased<T>
    {
    public:
        using SteadyClock = std::chrono::steady_clock;
        using TimePoint = SteadyClock::time_point;

    public:
        /**
         * @brief ifelse处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeIfelse(std::vector<RespValue> args, MSocket::MSocketPtr sock, std::shared_ptr<ServerData<T>> self,
                                        bool RecordAOF = true) override;

        /**
         * @brief 命令表处理命令
         * @param args 命令列表
         * @param sock 客户端sock
         * @param self 服务器数据
         * @param RecordAOF 是否记录AOF
         */
        virtual RespValue executeTable(std::vector<RespValue> args, MSocket::MSocketPtr sock, std::shared_ptr<ServerData<T>> self,
                                       bool RecordAOF = true) override { return RespValue{}; }
    };

    template <typename T>
    inline RespValue CommandHandlerIfelse<T>::executeIfelse(std::vector<RespValue> args, MSocket::MSocketPtr sock,
                                                            std::shared_ptr<ServerData<T>> self, bool RecordAOF)
    {
        auto start = SteadyClock::now();

        auto return_with_slowlog = [&](RespValue resp) -> RespValue
        {
            auto end = std::chrono::steady_clock::now();
            std::string cmd_str;
            for (size_t i = 0; i < args.size(); i++)
            {
                if (i > 0)
                {
                    cmd_str += " ";
                }
                cmd_str += args[i].str;
            }
            self->getSlowLog().pushEntry(cmd_str, sock, start, end);

            // 写命令记录进入AOF(异步)
            if (args.empty())
            {
                return resp;
            }
            std::string cmd = args[0].str;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            if (RecordAOF && self->getAOF().isWriteCommand(cmd))
            {
                std::string aof_cmds = self->getAOF().formatCommand(args);
                self->getAOF().appendToAOF(aof_cmds);

                // 如果是主节点，广播给从节点
                if (self->getReplication().getisMaster() && !self->getReplication().slavesEmpty()) 
                {
                    self->getReplication().broadcastToSlaves(aof_cmds);
                }
            }

            // // 每300条命令异步保存进入rbg(测试时注释掉)
            // if (self->getCommands().load(std::memory_order_acquire) % 300 == 0)
            // {
            //     std::weak_ptr<ServerData<T>> weak_self = self;
            //     std::thread([weak_self]()
            //                 { 
            //                     auto ptr = weak_self.lock();
            //                     if (ptr)
            //                     {
            //                         ptr->saveToFile(); 
            //                     } })
            //         .detach();
            // }
            return resp;
        };

        // 不记录AOF模式,设置推送给monitor为false,即不推送
        if (!RecordAOF)
        {
            self->getPushMonitor().store(false, std::memory_order_release);
        }
        else
        {
            self->getPushMonitor().store(true, std::memory_order_release);
        }

        if (args.empty())
        {
            return return_with_slowlog(RespValue::error("ERR empty command"));
        }

        std::string cmd = args[0].str;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "PING") // PING [message]
        {
            BLUE_LOG_INFO(xx::g_logger) << "if-else模式";
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 1 || args.size() > 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'PING'"));
            }
            if (args.size() == 1)
                return return_with_slowlog(RespValue::simple_string("PONG"));
            else
                return return_with_slowlog(RespValue::bulk_string(args[1].str));
        }
        else if (cmd == "AUTH") // AUTH password
        {
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'AUTH'"));
            }
            if (args[1].str == sock->getClientPassword())
            {
                if (sock->getClientlevel() != 0)
                {
                    return return_with_slowlog(RespValue::error("ERR this connection already have been logged by others"));
                }
                sock->setClientlevel(1);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            if (args[1].str == self->getPassword())
            {
                if (!self->getAdminSocket().expired())
                {
                    return return_with_slowlog(RespValue::error("ERR admin already logged in elsewhere"));
                }
                if (sock->getClientlevel() == 1)
                {
                    return return_with_slowlog(RespValue::error("ERR this connection already have been logged by client"));
                }
                self->setAdminSocket(sock);
                sock->setClientlevel(2);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            return RespValue::error("ERR invalid password");
        }
        else if (cmd == "SELECT") // SELECT [count]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SELECT'"));
            }
            int db;
            try
            {
                db = std::stoi(args[1].str);
                if (db < 0 || db >= DB_COUNT)
                {
                    return return_with_slowlog(RespValue::error("ERR DB index is out of range"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR invalid DB index"));
            }
            sock->setClientId(db);
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "CLIENT") // CLIENT GETNAME/(SETNAME [name])
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT'"));
            }
            std::string subcmd = args[1].str;
            std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
            if (subcmd == "SETNAME")
            {
                if (args.size() != 3)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT SETNAME'"));
                }
                sock->setClientName(args[2].str);
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else if (subcmd == "GETNAME")
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT GETNAME'"));
                }
                if (sock->getClientName().empty())
                {
                    return return_with_slowlog(RespValue::null_bulk());
                }
                return return_with_slowlog(RespValue::bulk_string(sock->getClientName()));
            }
            else if (subcmd == "LIST")
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CLIENT LIST'"));
                }
                std::string result;
                result += "name=" + sock->getClientName() + " ";
                result += "addr=" + sock->getRemoteAddress()->toString() + " ";
                result += "subScription=" + std::to_string((int)(sock->inSubScription())) + " ";
                result += "transaction=" + std::to_string((int)(sock->inTransaction())) + " ";
                result += "monitor=" + std::to_string((int)(sock->inMonitorMode())) + "\r\n";
                return return_with_slowlog(RespValue::bulk_string(result));
            }
            return return_with_slowlog(RespValue::error("ERR wrong arguments for 'CLIENT'"));
        }
        else if (cmd == "CONFIG") // CONFIG (GET [...])/SET [...], 获取或设置客户端的配置信息
        {
            // 同一时刻只能存在一个管理员，并且由于CommandHandler只有一个实例化，所以修改和获取不需要锁
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CONFIG'"));
            }
            std::string subcmd = args[1].str;
            std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
            if (subcmd == "GET")
            {
                if (args.size() != 3)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CONFIG GET'"));
                }
                std::string pattern = args[2].str;
                std::vector<RespValue> result;

                if (pattern == "*" || pattern == "clientpass")
                {
                    result.push_back(RespValue::bulk_string("clientpass"));
                    result.push_back(RespValue::bulk_string(sock->getClientPassword()));
                }
                if (pattern == "*" || pattern == "maxclients")
                {
                    result.push_back(RespValue::bulk_string("maxclients"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getMaxClientCount())));
                }
                if (pattern == "*" || pattern == "timeout")
                {
                    result.push_back(RespValue::bulk_string("timeout"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getTimeoutS())));
                }
                if (pattern == "*" || pattern == "slowlog-log-slower-than" || pattern == "slowlog-*")
                {
                    result.push_back(RespValue::bulk_string("slowlog-log-slower-than"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getSlowLog().getSlowLogThan())));
                }
                if (pattern == "*" || pattern == "slowlog-max-len" || pattern == "slowlog-*")
                {
                    result.push_back(RespValue::bulk_string("slowlog-max-len"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getSlowLog().getSlowMaxLen())));
                }
                if (pattern == "*" || pattern == "aof-enabled" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-enabled"));
                    result.push_back(RespValue::bulk_string(self->getAOF().getConfig_AOFEnabled() ? "yes" : "no"));
                }
                if (pattern == "*" || pattern == "aof-filename" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-filename"));
                    result.push_back(RespValue::bulk_string(self->getAOF().getConfig_AOFFilename()));
                }
                if (pattern == "*" || pattern == "aof-sync" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-sync"));
                    result.push_back(RespValue::bulk_string(self->getAOF().getConfig_AOFSync()));
                }
                if (pattern == "*" || pattern == "aof-max_file_size" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_file_size"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getAOF().getConfig_AOFMaxFileSize())));
                }
                if (pattern == "*" || pattern == "aof-max_file_number" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_file_number"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getAOF().getConfig_AOFMaxFileNumber())));
                }
                if (pattern == "*" || pattern == "aof-max_buffer_size" || pattern == "aof-*")
                {
                    result.push_back(RespValue::bulk_string("aof-max_buffer_size"));
                    result.push_back(RespValue::bulk_string(std::to_string(self->getAOF().getMaxAOFBufferSize())));
                }
                return return_with_slowlog(RespValue::array(std::move(result)));
            }
            else if (subcmd == "SET")
            {
                if (args.size() != 4)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'CONFIG SET'"));
                }

                const std::string &param = args[2].str;
                const std::string &value = args[3].str;

                if (param == "clientpass") // 客户端密码
                {
                    sock->setClientPassword(value);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "slowlog-log-slower-than") // 慢查询的限制时长(超过这个时长记录慢查询)
                {
                    try
                    {
                        int64_t val = std::stoll(value);
                        if (val < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value must be >= 0"));
                        }
                        self->getSlowLog().setSlowLogThan(val);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "slowlog-max-len") // 慢查询记录最大条数
                {
                    try
                    {
                        size_t val = std::stoul(value);
                        if (val <= 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value must be > 0"));
                        }
                        self->getSlowLog().setSlowMaxLen(val);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "aof-enabled") // 开启aof记录
                {
                    if (value == "yes" || value == "1")
                    {
                        self->getAOF().setConfig_AOFEnabled(true);
                        self->getAOF().initAOF();
                    }
                    else if (value == "no" || value == "0")
                    {
                        self->getAOF().setConfig_AOFEnabled(false);
                        self->getAOF().closeAOF();
                    }
                    else
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid value"));
                    }
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-sync") // aof策略
                {
                    if (value == "always" || value == "everysec" || value == "no")
                    {
                        self->getAOF().setConfig_AOFSync(value);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    return return_with_slowlog(RespValue::error("ERR invalid sync mode"));
                }
                if (param == "aof-filename") // aof文件名(模板文件名)
                {
                    if (value.empty())
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid filename"));
                    }
                    self->getAOF().setConfig_AOFFilename(value);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_file_size") // 每个aof文件大小
                {
                    int64_t val;
                    try
                    {
                        val = std::stoi(value);
                        if (val < 1024 * 1024)
                        {
                            return return_with_slowlog(RespValue::error("ERR max_file_size value too small"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }

                    self->getAOF().setConfig_AOFMaxFileSize(val);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_file_number") // 最多保留多少aof文件
                {
                    int val;
                    try
                    {
                        val = std::stoi(value);
                        if (val < 5)
                        {
                            return return_with_slowlog(RespValue::error("ERR max_file_number value too small"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                    self->getAOF().setConfig_AOFMaxFileNumber(val);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                if (param == "aof-max_buffer_size") // aof缓冲区大小
                {
                    int64_t val;
                    try
                    {
                        val = std::stoll(value);
                        if (val < 1024 * 1024)
                        {
                            return return_with_slowlog(RespValue::error("ERR max_file_number value too small"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                    self->getAOF().setMaxAOFBufferSize(val);
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                // 以下只允许管理员设置
                if (!self->isAdmin(sock))
                {
                    return return_with_slowlog(RespValue::error("ERR authentication required"));
                }
                if (param == "maxclients") // 服务器最大支持的客户端数量
                {
                    try
                    {
                        int newmax = std::stoi(value);
                        if (newmax <= 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR invalid maxclients value"));
                        }
                        if (newmax < self->getConnection())
                        {
                            return return_with_slowlog(RespValue::error("ERR maxclients can't be less than current connections"));
                        }
                        self->setMaxClientCount(newmax);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                if (param == "timeout") // 每个客户端会话的超时时长
                {
                    try
                    {
                        int timeout = std::stoi(value);
                        if (timeout < 0)
                        {
                            return RespValue::error("ERR invalid timeout value");
                        }
                        self->setTimeoutS(timeout);
                        return return_with_slowlog(RespValue::simple_string("OK"));
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR invalid integer value"));
                    }
                }
                return return_with_slowlog(RespValue::error("ERR Unsupported CONFIG parameter: " + param));
            }
            return return_with_slowlog(RespValue::error("ERR wrong arguments for 'CONFIG'"));
        } // string
        else if (cmd == "SET") // SET key val [EX [s]]/[PX [ms]]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SET'"));
            }
            const std::string &key = args[1].str;
            const std::string &val = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            std::optional<TimePoint> timepoint;
            // shards.store[key] = val;
            if (args.size() >= 5)
            {
                std::string subcmd = args[3].str;
                std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
                if (subcmd == "EX")
                {
                    int64_t seconds;
                    try
                    {
                        seconds = std::stoll(args[4].str);
                        if (seconds < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value can't be nagative"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                    }
                    timepoint = SteadyClock::now() + std::chrono::seconds(seconds);
                }
                else if (subcmd == "PX")
                {
                    int64_t milliseconds;
                    try
                    {
                        milliseconds = std::stoll(args[4].str);
                        if (milliseconds < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR value can't be nagative"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                    }
                    timepoint = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
                }
            }
            shards.store[key] = DataShard::StoreData(val, timepoint);
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "GET") // GET key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'GET'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            if (it->second.is_expired())
            {
                lock.unlock();
                std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
                shards.store.erase(key);
                return return_with_slowlog(RespValue::null_bulk());
            }
            const std::string &val = it->second.val;
            return return_with_slowlog(RespValue::bulk_string(val));
        }
        else if (cmd == "MSET") // MSET key val [key val]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3 || (args.size() & 1) != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'MSET'"));
            }

            for (size_t i = 1; i < args.size(); i += 2)
            {
                const std::string &key = args[i].str;
                const std::string &val = args[i + 1].str;
                auto &shards = self->getShard(key, sock);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                shards.store[key] = DataShard::StoreData(val);
            }
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "MGET") // MGET key [key...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'MGET'"));
            }
            std::vector<RespValue> results;

            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string &key = args[i].str;
                auto &shards = self->getShard(key, sock);
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                auto it = shards.store.find(key);
                if (it == shards.store.end())
                {
                    results.push_back(RespValue::null_bulk());
                }
                else
                {
                    if (it->second.is_expired())
                    {
                        lock.unlock();
                        std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
                        shards.store.erase(key);
                        results.push_back(RespValue::null_bulk());
                    }
                    else
                    {
                        results.push_back(RespValue::bulk_string(it->second.val));
                    }
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "GETSET") // GETSET key val 返回旧值设置新值
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'GETSET'"));
            }
            const std::string &key = args[1].str;
            const std::string &val = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = DataShard::StoreData(val);
                return return_with_slowlog(RespValue::bulk_string(val));
            }
            std::string ans = it->second.val;
            it->second.val = val;
            return return_with_slowlog(RespValue::bulk_string(ans));
        }
        else if (cmd == "APPEND") // APPEND key val
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'APPEND'"));
            }
            const std::string &key = args[1].str;
            const std::string &val = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = DataShard::StoreData(val);
            }
            else
            {
                it->second.val.append(val);
            }
            return return_with_slowlog(RespValue::integer(shards.store[key].val.size()));
        }
        else if (cmd == "SETNX") // SETNX key val, 没有才设置
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'SETNX'"));
            }
            const std::string &key = args[1].str;
            const std::string &val = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                shards.store[key] = DataShard::StoreData(val);
                return return_with_slowlog(RespValue::integer(1));
            }
            return return_with_slowlog(RespValue::integer(0));
        }
        else if (cmd == "EXISTS") // EXISTS key [key...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong of arguments for 'EXISTS'"));
            }
            int64_t count = 0;
            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string &key = args[i].str;
                auto &shards = self->getShard(key, sock);
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                if (shards.store.find(key) != shards.store.end() ||
                    shards.hash.find(key) != shards.hash.end() ||
                    shards.lists.find(key) != shards.lists.end() ||
                    shards.sets.find(key) != shards.sets.end() ||
                    shards.zset.find(key) != shards.zset.end())
                {
                    count++;
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "DEL") // DEL key [key...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'DEL'"));
            }
            int count = 0;
            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string &key = args[i].str;
                auto &shards = self->getShard(key, sock);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto it = shards.store.find(key);
                if (it != shards.store.end())
                {
                    shards.store.erase(it);
                    count++;
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "HSET") // HSET key field value [field value ...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HSET'"));
            }
            int count = 0;
            const std::string &key = args[1].str;
            for (size_t i = 2; i < args.size(); i += 2)
            {
                auto &shards = self->getShard(key, sock);
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                auto &field = args[i].str;
                auto &value = args[i + 1].str; // size 是偶数所以不会出界
                if (shards.hash[key].find(field) == shards.hash[key].end())
                {
                    ++count; // 新字段
                }
                shards.hash[key][field] = value;
            }
            return return_with_slowlog(RespValue::integer(count));
        } // 哈希集合操作
        else if (cmd == "HGET") // HGET key field
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HGET'"));
            }
            const std::string &key = args[1].str;
            const std::string &field = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto fit = it->second.find(field);
            if (fit == it->second.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            const std::string &val = fit->second;
            return return_with_slowlog(RespValue::bulk_string(val));
        }
        else if (cmd == "HGETALL") // HGETALL key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HGETALL'"));
            }
            std::vector<RespValue> result;
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return return_with_slowlog(RespValue::array(std::move(result)));
            }
            for (auto &[field, value] : it->second)
            {
                result.push_back(RespValue::bulk_string(field));
                result.push_back(RespValue::bulk_string(value));
            }
            return return_with_slowlog(RespValue::array(std::move(result)));
        }
        else if (cmd == "HDEL") // HDEL key field [field ...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HDEL'"));
            }
            int count = 0;
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                return return_with_slowlog(RespValue::integer(count));
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                count += it->second.erase(args[i].str);
            }
            if (it->second.empty())
            {
                shards.hash.erase(it);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "HLEN") // HLEN key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HLEN'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> rdlock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "HEXISTS") // HEXISTS key field
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HEXISTS'"));
            }
            const std::string &key = args[1].str;
            const std::string &field = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }
                return return_with_slowlog(RespValue::integer(0));
            }
            auto filed_it = it->second.find(field);
            if (filed_it == it->second.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "HKEYS") // HKEYS key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HKEYS'"));
            }
            const std::string &key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                return return_with_slowlog(RespValue::array(std::move(results)));
            }
            for (auto &[field, _] : it->second)
            {
                results.push_back(RespValue::bulk_string(field));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "HVALS") // HVALS key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'HKEYS'"));
            }
            const std::string &key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.hash.find(key);
            if (it == shards.hash.end())
            {
                if (shards.store.find(key) != shards.store.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                return return_with_slowlog(RespValue::array(std::move(results)));
            }
            for (auto &[_, val] : it->second)
            {
                results.push_back(RespValue::bulk_string(val));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "KEYS") // KEYS *
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'KEYS'"));
            }

            // 把 glob 风格转成 regex
            std::string pattern = args[1].str;
            std::string regex_str;
            for (char c : pattern)
            {
                if (c == '*')
                {
                    regex_str += ".*";
                }
                else if (c == '?')
                {
                    regex_str += ".";
                }
                else if (c == '.' || c == '+' || c == '[' || c == ']' ||
                         c == '(' || c == ')' || c == '\\')
                {
                    regex_str += '\\';
                    regex_str += c;
                }
                else
                {
                    regex_str += c;
                }
            }

            constexpr size_t MAX_KEYS = 10000;
            std::regex re(regex_str);
            std::vector<RespValue> result;
            result.reserve(MAX_KEYS);

            // 遍历所有分片
            for (auto &shard : self->getDBs()[sock->getClientId()])
            {
                if (result.size() >= MAX_KEYS)
                {
                    BLUE_LOG_WARN(xx::g_logger) << "KEYS command truncated at " << MAX_KEYS << " keys";
                    break;
                }
                std::shared_lock<std::shared_mutex> lock(shard.mutex);

                for (auto &[key, value] : shard.store)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // Hash 类型的 key
                for (auto &[key, fields] : shard.hash)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // List 类型的 key
                for (auto &[key, list] : shard.lists)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // Set 类型的 key
                for (auto &[key, set] : shard.sets)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
                // ZSet 类型的 key
                for (auto &[key, zset] : shard.zset)
                {
                    if (std::regex_match(key, re))
                    {
                        result.push_back(RespValue::bulk_string(key));
                    }
                }
            }

            return return_with_slowlog(RespValue::array(std::move(result)));
        } // 链表操作
        else if (cmd == "LPUSH") // LPUSH key val [val...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LPUSH'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &lhs = shards.lists[key];
            for (size_t i = 2; i < args.size(); i++)
            {
                lhs.push_front(args[i].str);
            }
            return return_with_slowlog(RespValue::integer(lhs.size()));
        }
        else if (cmd == "RPUSH") // RPUSH key val [val...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RPUSH'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto &lhs = shards.lists[key];
            for (size_t i = 2; i < args.size(); i++)
            {
                lhs.push_back(args[i].str);
            }
            return return_with_slowlog(RespValue::integer(lhs.size()));
        }
        else if (cmd == "LPOP") // LPOP key [count]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 || args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LPOP'"));
            }
            int count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return return_with_slowlog(RespValue::error("ERR count can't be nagative"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end() || it->second.empty())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            std::vector<RespValue> results;
            for (int i = 0; i < count && !it->second.empty(); i++)
            {
                results.push_back(RespValue::bulk_string(it->second.front()));
                it->second.pop_front();
            }
            if (it->second.empty())
            {
                shards.lists.erase(it);
            }
            if (results.size() == 1 && args.size() == 2)
            {
                return return_with_slowlog(results[0]);
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "RPOP") // RPOP key [count]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 || args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LPOP'"));
            }
            int count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return return_with_slowlog(RespValue::error("ERR count can't be nagative"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end() || it->second.empty())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            std::vector<RespValue> results;
            for (int i = 0; i < count && !it->second.empty(); i++)
            {
                results.push_back(RespValue::bulk_string(it->second.back()));
                it->second.pop_back();
            }
            if (it->second.empty())
            {
                shards.lists.erase(it);
            }
            if (results.size() == 1 && args.size() == 2)
            {
                return return_with_slowlog(results[0]);
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "LLEN") // LLEN key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LLEN'"));
            }
            const std::string &key = args[1].str;
            auto &shard = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "LINSERT") // LINSERT key [BEFORE/AFTER] pivot val, pivot 不存在返回-1
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 5)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LINSERT'"));
            }
            const std::string &key = args[1].str;
            const std::string &pos = args[2].str;
            const std::string &pivot = args[3].str;
            const std::string &val = args[4].str;

            auto &shard = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            std::list<std::string> &lists = it->second;
            auto list_it = lists.begin();
            for (; list_it != lists.end(); list_it++)
            {
                if (*list_it == pivot)
                {
                    break;
                }
            }

            if (list_it == lists.end())
            {
                return return_with_slowlog(RespValue::integer(-1)); // pivot不存在
            }
            if (pos == "AFTER")
            {
                list_it++;
            }
            lists.insert(list_it, val);
            return return_with_slowlog(RespValue::integer(lists.size()));
        }
        else if (cmd == "LINDEX") // LINDEX key index
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LINDEX'"));
            }
            const std::string &key = args[1].str;
            int64_t idx = 0;
            try
            {
                idx = std::stoll(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            auto &shard = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            size_t size = it->second.size();
            if (idx < 0)
            {
                idx += size;
            }
            if (idx < 0)
            {
                idx = 0;
            }
            auto list_it = it->second.begin();
            while (idx--)
            {
                list_it++;
            }
            return return_with_slowlog(RespValue::bulk_string(*list_it));
        }
        else if (cmd == "LSET") // LSET key index val
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LSET'"));
            }
            const std::string &key = args[1].str;
            const std::string &val = args[3].str;
            int64_t idx = 0;
            try
            {
                idx = std::stoll(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            auto &shard = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.lists.find(key);
            if (it == shard.lists.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            size_t size = it->second.size();
            if (idx < 0)
            {
                idx += size;
            }
            if (idx < 0)
            {
                idx = 0;
            }
            auto list_it = it->second.begin();
            while (idx--)
            {
                list_it++;
            }
            *list_it = val;
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "RPOPLPUSH" || cmd == "LPOPRPUSH") // RPOPLPUSH source destination
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RPOPLPUSH'"));
            }
            const std::string &source_key = args[1].str;
            const std::string &dest_key = args[2].str;
            if (source_key == dest_key)
            {
                return return_with_slowlog(RespValue::integer(1));
            }
            int src_idx = self->getShardIndex(source_key);
            int dest_idx = self->getShardIndex(dest_key);
            if (src_idx > dest_idx)
            {
                std::swap(src_idx, dest_idx);
            }
            auto &src_shard = self->getShard(source_key, sock);
            auto &dest_shard = self->getShard(dest_key, sock);
            std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (src_idx != dest_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
            }
            auto src_it = src_shard.lists.find(source_key);
            if (src_it == src_shard.lists.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }

            auto dest_it = dest_shard.lists.find(dest_key);
            if (dest_it == dest_shard.lists.end())
            {
                dest_shard.lists[dest_key] = std::list<std::string>();
                dest_it = dest_shard.lists.find(dest_key);
            }
            if (cmd == "RPOPLPUSH")
            {
                const std::string tem = src_it->second.back();
                src_it->second.pop_back();
                dest_it->second.push_front(tem);
            }
            else
            {
                const std::string tem = src_it->second.front();
                src_it->second.pop_front();
                dest_it->second.push_back(tem);
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "LRANGE") // LRANGE key start stop
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LRANGE'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            int start, stop;
            try
            {
                start = std::stoi(args[2].str);
                stop = std::stoi(args[3].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.lists.find(key);
            if (it == shards.lists.end())
            {
                return return_with_slowlog(RespValue::array({}));
            }
            int size = it->second.size();
            if (start < 0)
            {
                start += size; // 尽量偏移到正数
            }
            if (stop < 0)
            {
                stop += size;
            }
            if (start < 0) // 还是小于0就从0开始
            {
                start = 0;
            }
            if (stop >= size)
            {
                stop = size - 1;
            }
            if (start > stop)
            {
                return return_with_slowlog(RespValue::array({}));
            }
            std::vector<RespValue> result;
            auto iter = it->second.begin();
            std::advance(iter, start);
            for (int i = start; i <= stop && iter != it->second.end(); i++, iter++)
            {
                result.push_back(RespValue::bulk_string(*iter));
            }
            return return_with_slowlog(RespValue::array(std::move(result)));
        } // 有序集合操作
        else if (cmd == "ZADD") // ZADD key score member [score member]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() % 2 != 0)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZADD'"));
            }
            int count = 0;
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);

            shards.zset_score.try_emplace(key);
            shards.zset.try_emplace(key);
            auto &score_map = shards.zset_score[key];
            auto &skiplist = shards.zset[key];

            for (size_t i = 2; i < args.size(); i += 2)
            {
                double score;
                try
                {
                    score = std::stod(args[i].str);
                }
                catch (const std::exception &e)
                {
                    return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
                }
                const std::string &member = args[i + 1].str;

                auto it = score_map.find(member);
                if (it != score_map.end())
                {
                    // 已存在，删掉旧的
                    ZSetKey old_key(it->second, it->first);
                    skiplist.remove(old_key);
                }
                else
                {
                    count++;
                }
                score_map[member] = score;
                ZSetKey newkey(score, member);
                skiplist.insert(newkey, member);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "ZRANGE") // ZRANGE key start stop [WITHSCORES]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() > 5)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZRANGE'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            int start, stop;
            try
            {
                start = std::stoi(args[2].str);
                stop = std::stoi(args[3].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.zset.find(key);
            if (it == shards.zset.end())
            {
                return return_with_slowlog(RespValue::array({}));
            }
            bool withscores = (args.size() == 5 && (args[4].str == "WITHSCORES" || args[4].str == "withscores"));
            auto &skiplist_map = it->second;
            int size = skiplist_map.size();
            if (start < 0)
            {
                start += size;
            }
            if (stop < 0)
            {
                stop += size;
            }
            if (start < 0)
            {
                start = 0;
            }
            if (stop >= size)
            {
                stop = size - 1;
            }
            if (start > stop)
            {
                return return_with_slowlog(RespValue::array({}));
            }
            std::vector<RespValue> results;
            for (int i = start; i <= stop; i++)
            {
                auto *node = skiplist_map.getByIndex(i);
                if (!node)
                {
                    break;
                }
                results.push_back(RespValue::bulk_string(node->val));
                if (withscores)
                {
                    results.push_back(RespValue::bulk_string(self->format_score(node->key.score)));
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "ZREM") // ZREM key member [member...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZREM'"));
            }
            int count = 0;
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset.find(key);
            if (it == shards.zset.end())
            {
                return return_with_slowlog(RespValue::integer(count));
            }
            auto sit = shards.zset_score.find(key);
            if (sit != shards.zset_score.end())
            {
                auto &skiplist_map = it->second;
                auto &scores = sit->second;

                for (size_t i = 2; i < args.size(); i++)
                {
                    const std::string &member = args[i].str;
                    auto member_score = scores.find(member);
                    if (member_score != scores.end())
                    {
                        // key = {score,member}
                        skiplist_map.remove({member_score->second, member_score->first});
                        scores.erase(member_score);
                        count++;
                    }
                }
                if (skiplist_map.empty())
                {
                    shards.zset.erase(it);
                    shards.zset_score.erase(key);
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "ZSCORE") // // ZSCORE key member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ASCORE'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset_score.find(key);
            if (it == shards.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &skiplist_map = it->second;
            auto sit = skiplist_map.find(args[2].str);
            if (sit != skiplist_map.end())
            {
                return return_with_slowlog(RespValue::bulk_string(self->format_score(sit->second)));
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZRANK") // ZRANK key member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZRANK'"));
            }
            const std::string &key = args[1].str, member = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.zset_score.find(key);
            if (it == shards.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &score_map = it->second;
            auto sit = score_map.find(member);
            if (sit != score_map.end())
            {
                // key  = {score,member}
                int rank = shards.zset[key].getRank({sit->second, member});
                if (rank >= 0)
                {
                    return return_with_slowlog(RespValue::integer(rank));
                }
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZINCRBY") // ZINCRBY key incr member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZINCRBY'"));
            }
            const std::string &key = args[1].str;
            const std::string &member = args[3].str;
            int64_t incr;
            try
            {
                incr = std::stoll(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
            }
            auto &shard = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset.find(key);
            if (it == shard.zset.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto sit = shard.zset_score.find(key);
            if (sit == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &skiplist = it->second;
            auto &scores_map = sit->second;
            if (scores_map.find(member) != scores_map.end())
            {
                ZSetKey old_val(scores_map[member], member);
                scores_map[member] += incr;
                skiplist.remove(old_val);
                skiplist.insert({scores_map[member], member}, member);
                return return_with_slowlog(RespValue::bulk_string(self->format_score(scores_map[member])));
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZINCRBYFLOAT") // ZINCRBYFLOAT
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZINCRBY'"));
            }
            const std::string &key = args[1].str;
            const std::string &member = args[3].str;
            double incr;
            try
            {
                incr = std::stod(args[2].str);
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a float or out of range"));
            }
            auto &shard = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset.find(key);
            if (it == shard.zset.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto sit = shard.zset_score.find(key);
            if (sit == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            auto &skiplist = it->second;
            auto &scores_map = sit->second;
            if (scores_map.find(member) != scores_map.end())
            {
                ZSetKey old_val(scores_map[member], member);
                scores_map[member] += incr;
                skiplist.remove(old_val);
                skiplist.insert({scores_map[member], member}, member);
                return return_with_slowlog(RespValue::bulk_string(self->format_score(scores_map[member])));
            }
            return return_with_slowlog(RespValue::null_bulk());
        }
        else if (cmd == "ZCOUNT") // ZCOUNT key min max
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZCOUNT'"));
            }
            const std::string &key = args[1].str;
            double min = 0, max = 0;
            try
            {
                min = std::stod(args[2].str);
                max = std::stod(args[3].str);
                if (min > max)
                {
                    return return_with_slowlog(RespValue::error("ERR min can't greater than max"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
            }
            int64_t count = 0;
            auto &shard = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset_score.find(key);
            if (it == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            for (const auto &[_, score] : it->second)
            {
                if (score >= min && score <= max)
                {
                    count++;
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "ZRANGEBYSCORE") // ZRANGEBYSCORE key min max [WITHSCORE]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 4 || args.size() > 5)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZRANGEBYSCORE'"));
            }
            const std::string &key = args[1].str;
            double min = 0, max = 0;
            try
            {
                min = std::stod(args[2].str);
                max = std::stod(args[3].str);
                if (min > max)
                {
                    return return_with_slowlog(RespValue::error("ERR min can't greater than max"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
            }
            std::vector<RespValue> results;
            bool withscore = (args.size() == 5 && (args[4].str == "WITHSCORE" || args[4].str == "withscore"));
            auto &shard = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset_score.find(key);
            if (it == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::array({}));
            }
            for (const auto &[member, score] : it->second)
            {
                if (score >= min && score <= max)
                {
                    results.push_back(RespValue::bulk_string(member));
                    if (withscore)
                    {
                        results.push_back(RespValue::bulk_string((self->format_score(score))));
                    }
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "ZREMRANGEBYSCORE") // ZREMRANGEBYSCORE key min max
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ZREMRANGEBYSCORE'"));
            }
            const std::string &key = args[1].str;
            double min = 0, max = 0;
            try
            {
                min = std::stod(args[2].str);
                max = std::stod(args[3].str);
                if (min > max)
                {
                    return return_with_slowlog(RespValue::error("ERR min can't greater than max"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not a double or out of range"));
            }
            int64_t count = 0;
            auto &shard = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            auto it = shard.zset.find(key);
            if (it == shard.zset.end())
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            auto sit = shard.zset_score.find(key);
            if (sit == shard.zset_score.end())
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            auto &skiplist = it->second;
            auto &scores = sit->second;
            std::vector<std::string> tem;
            for (const auto &[member, score] : scores)
            {
                if (score >= min && score <= max)
                {
                    ZSetKey old_val(score, member);
                    skiplist.remove(old_val);
                    tem.push_back(member);
                    count++;
                }
            }
            for (auto &mem : tem)
            {
                scores.erase(mem);
            }
            if (scores.empty())
            {
                shard.zset.erase(it);
                shard.zset_score.erase(sit);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "INCR") // INCR key 自增
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'INCR'"));
            }
            const std::string &key = args[1].str;
            int64_t val = 0;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> rdlock(shards.mutex);
            auto it = shards.store.find(key);
            if (it != shards.store.end())
            {
                try
                {
                    val = std::stoll(it->second.val);
                }
                catch (...)
                {
                    return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                }
            }
            val++;
            shards.store[key].val = std::to_string(val);
            return return_with_slowlog(RespValue::integer(val));
        }
        else if (cmd == "INCRBY") // INCR key integer 自增或自减 integer
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'INCRBY'"));
            }
            const std::string &key = args[1].str;
            int64_t increment;
            try
            {
                increment = std::stoll(args[2].str);
            }
            catch (const std::exception &e)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            int64_t val = 0;
            if (it != shards.store.end())
            {
                try
                {
                    val = std::stoll(it->second.val);
                }
                catch (...)
                {
                    return return_with_slowlog(RespValue::error("ERR value is not a integer or out of range"));
                }
            }
            val += increment;
            shards.store[key].val = std::to_string(val);
            return return_with_slowlog(RespValue::integer(val));
        }
        else if (cmd == "STRLEN") // STRLEN key, 返回key对于val的长度
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'STRLEN'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                if (shards.hash.find(key) != shards.hash.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.lists.find(key) != shards.lists.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.zset.find(key) != shards.zset.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                if (shards.sets.find(key) != shards.sets.end())
                {
                    return return_with_slowlog(RespValue::error("WRONGTYPE Operation against a key holding the wrong kind of value"));
                }

                return return_with_slowlog(RespValue::integer(0));
            }
            auto& data = it->second;
            if (data.expire.has_value() && data.expire < SteadyClock::now())
            {
                lock.unlock();
                std::unique_lock<std::shared_mutex> lock(shards.mutex);
                shards.store.erase(key);
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.val.size()));
        }
        else if (cmd == "TYPE") // TYPE key, key的类型
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'TYPE'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            if (shards.store.find(key) != shards.store.end())
            {
                return return_with_slowlog(RespValue::bulk_string("string"));
            }
            if (shards.hash.find(key) != shards.hash.end())
            {
                return return_with_slowlog(RespValue::bulk_string("hash"));
            }
            if (shards.lists.find(key) != shards.lists.end())
            {
                return return_with_slowlog(RespValue::bulk_string("list"));
            }
            if (shards.zset.find(key) != shards.zset.end())
            {
                return return_with_slowlog(RespValue::bulk_string("zset"));
            }
            if (shards.sets.find(key) != shards.sets.end())
            {
                return return_with_slowlog(RespValue::bulk_string("set"));
            }
            return return_with_slowlog(RespValue::null_bulk());
        } // 无序集合操作
        else if (cmd == "SADD") // SADD key member [member...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of argument for 'SADD'"));
            }
            const std::string &key = args[1].str;
            int32_t count = 0;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string &member = args[i].str;
                if (shards.sets[key].find(member) == shards.sets[key].end())
                {
                    auto [_, res] = shards.sets[key].insert(member);
                    if (res)
                    {
                        count++;
                    }
                }
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "SMEMBERS") // SMEMBERS key, 返回key所有的成员
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SMEMBERS'"));
            }
            const std::string &key = args[1].str;
            std::vector<RespValue> results;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::array(std::move(results)));
            }
            for (auto &member : it->second)
            {
                results.push_back(RespValue::bulk_string(member));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SREM") // SREM key member [member...]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SREM'"));
            }
            const std::string &key = args[1].str;
            int32_t count = 0;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                count += it->second.erase(args[i].str);
            }
            if (it->second.empty())
            {
                shards.sets.erase(it);
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "SISMEMBER") // SISMEMBER key member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR number wrong of arguments for 'SISMEMBER'"));
            }
            const std::string &key = args[1].str;
            const std::string &member = args[2].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.count(member)));
        }
        else if (cmd == "SCARD") // SCARD key, key的集合大小
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR number wrong of arguments for 'SCARD'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            return return_with_slowlog(RespValue::integer(it->second.size()));
        }
        else if (cmd == "SRANDMEMBER") // SRANDMEMBER key [count], 随机返回count个member,count < 0可包含重复值, > 0不重复
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 && args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SRANDMEMBER'"));
            }
            int32_t count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 0;
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                if (args.size() == 2)
                {
                    return return_with_slowlog(RespValue::null_bulk());
                }
                else
                {
                    return return_with_slowlog(RespValue::array({}));
                }
            }
            const auto &set = it->second;
            std::vector<std::string> members(set.begin(), set.end());
            std::vector<RespValue> results;
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                return return_with_slowlog(RespValue::bulk_string(members[idx]));
            }

            if (count >= 0)
            {
                // 正数：不重复
                int num = std::min(count, (int32_t)(members.size()));
                std::shuffle(members.begin(), members.end(), std::mt19937(std::random_device()()));
                for (int i = 0; i < num; i++)
                {
                    results.push_back(RespValue::bulk_string(members[i]));
                }
            }
            else
            {
                // 负数：可重复
                int num = -count;
                for (int i = 0; i < num; i++)
                {
                    int idx = rand() % members.size();
                    results.push_back(RespValue::bulk_string(members[idx]));
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SPOP") // SPOP key [count], 随机返回并删除count个member,count只能大于0
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2 && args.size() > 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SRANDMEMBER'"));
            }
            int32_t count;
            try
            {
                count = args.size() == 3 ? std::stoi(args[2].str) : 1;
                if (count < 0)
                {
                    return return_with_slowlog(RespValue::error("ERR value can't be nagative"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> wrlock(shards.mutex);
            auto it = shards.sets.find(key);
            if (it == shards.sets.end())
            {
                if (args.size() == 2)
                {
                    return return_with_slowlog(RespValue::null_bulk());
                }
                else
                {
                    return return_with_slowlog(RespValue::array({}));
                }
            }
            auto &set = it->second;
            std::vector<std::string> members(set.begin(), set.end());
            std::vector<RespValue> results;
            if (count == 0)
            {
                return RespValue::array({});
            }
            if (members.empty())
            {
                return RespValue::null_bulk();
            }
            if (args.size() == 2)
            {
                int idx = rand() % members.size();
                set.erase(members[idx]);
                if (set.empty())
                {
                    shards.sets.erase(it);
                }
                return return_with_slowlog(RespValue::bulk_string(members[idx]));
            }

            if (count > 0)
            {
                // 正数：不重复
                int num = std::min(count, (int32_t)(members.size()));
                std::shuffle(members.begin(), members.end(), std::mt19937(std::random_device()()));
                for (int i = 0; i < num; i++)
                {
                    set.erase(members[i]);
                    results.push_back(RespValue::bulk_string(members[i]));
                }
                if (set.empty())
                {
                    shards.sets.erase(it);
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SDIFF") // SDIFF key [key...] 差集
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SDIFF'"));
            }
            const std::string &key = args[1].str;
            auto &shard = self->getShard(key, sock);
            std::vector<RespValue> results;
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            for (const auto &member : shard.sets[key])
            {
                bool ok = true;
                for (size_t i = 2; i < args.size(); i++)
                {
                    const std::string &tem_key = args[i].str;
                    auto &tem_shard = self->getShard(tem_key, sock);
                    std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                    auto &tem_members = tem_shard.sets[tem_key];
                    if (tem_members.contains(member))
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                {
                    results.push_back(RespValue::bulk_string(member));
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SINTER") // SINTER key [key...] 交集
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SINTER'"));
            }
            const std::string &key = args[1].str;
            auto &shard = self->getShard(key, sock);
            std::vector<RespValue> results;
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            for (const auto &member : shard.sets[key])
            {
                bool ok = true;
                for (size_t i = 2; i < args.size(); i++)
                {
                    const std::string &tem_key = args[i].str;
                    auto &tem_shard = self->getShard(tem_key, sock);
                    std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                    auto &tem_members = tem_shard.sets[tem_key];
                    if (!tem_members.contains(member))
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                {
                    results.push_back(RespValue::bulk_string(member));
                }
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SUNION") // SUNION key [key...] 并集
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SUNION'"));
            }
            const std::string &key = args[1].str;
            auto &shard = self->getShard(key, sock);
            std::unordered_set<std::string> results_set;
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            for (const auto &member : shard.sets[key])
            {
                results_set.insert(member);
            }
            for (size_t i = 2; i < args.size(); i++)
            {
                const std::string &tem_key = args[i].str;
                auto &tem_shard = self->getShard(tem_key, sock);
                std::shared_lock<std::shared_mutex> tem_lock(tem_shard.mutex);
                auto &tem_members = tem_shard.sets[tem_key];
                for (const auto &tem_member : tem_members)
                {
                    results_set.insert(tem_member);
                }
            }
            lock.unlock();
            std::vector<RespValue> results;
            for (const auto &member : results_set)
            {
                results.push_back(RespValue::bulk_string(member));
            }
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "SMOVE") // SMOVE source destination member
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 4)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SMOVE'"));
            }
            const std::string &source_key = args[1].str;
            const std::string &destination_key = args[2].str;
            if (source_key == destination_key)
            {
                return return_with_slowlog(RespValue::integer(1));
            }
            const std::string &member = args[3].str;
            int src_shard_idx = self->getShardIndex(source_key);
            int dest_shard_idx = self->getShardIndex(destination_key);
            if (src_shard_idx > dest_shard_idx)
            {
                std::swap(src_shard_idx, dest_shard_idx);
            }
            auto &src_shard = self->getShard(source_key, sock);
            auto &dest_shard = self->getShard(destination_key, sock);

            std::unique_lock<std::shared_mutex> lock1(src_shard.mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (src_shard_idx != dest_shard_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(dest_shard.mutex);
            }

            auto src_it = src_shard.sets.find(source_key);
            if (src_it == src_shard.sets.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }

            auto dest_it = dest_shard.sets.find(destination_key);
            if (dest_it == dest_shard.sets.end())
            {
                dest_shard.sets[destination_key] = std::unordered_set<std::string>();
                dest_it = dest_shard.sets.find(destination_key);
            }
            // 检查member是否在源集合中
            if (src_it->second.find(member) == src_it->second.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            src_it->second.erase(member);
            dest_it->second.insert(member);
            if (src_it->second.empty())
            {
                src_shard.sets.erase(src_it);
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "FLUSHDB") // FLUSHDB [confirm], 清空数据库(还没有持久化)
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (self->isAdmin(sock))
            {
                if (args.size() < 1 || args.size() > 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'FLUSHDB'"));
                }
                for (auto &shards : self->getDBs()[sock->getClientId()])
                {
                    std::unique_lock<std::shared_mutex> lock(shards.mutex);
                    shards.store.clear();
                    shards.lists.clear();
                    shards.hash.clear();
                    shards.sets.clear();
                    shards.zset.clear();
                    shards.zset_score.clear();
                }
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else if (sock->getClientlevel() == 1)
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR maybe need 'FLUSHAD CONFIRM"));
                }
                std::string confirm = args[1].str;
                std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::toupper);
                if (confirm == "CONFIRM")
                {
                    for (auto &shards : self->getDBs()[sock->getClientId()])
                    {
                        std::unique_lock<std::shared_mutex> lock(shards.mutex);
                        shards.store.clear();
                        shards.lists.clear();
                        shards.hash.clear();
                        shards.sets.clear();
                        shards.zset.clear();
                        shards.zset_score.clear();
                    }
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                return return_with_slowlog(RespValue::error("ERR maybe need 'FLUSHDB CONFIRM'"));
            }
            return return_with_slowlog(RespValue::error("ERR authentication required"));
        }
        else if (cmd == "FLUSHDBALL") // FLUSHDBALL, 清空所有数据库
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (self->isAdmin(sock))
            {
                if (args.size() < 1 || args.size() > 2)
                {
                    return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'FLUSHDBALL'"));
                }
                for (int db = 0; db < DB_COUNT; db++)
                {
                    for (auto &shards : self->getDBs()[db])
                    {
                        std::unique_lock<std::shared_mutex> lock(shards.mutex);
                        shards.store.clear();
                        shards.lists.clear();
                        shards.hash.clear();
                        shards.sets.clear();
                        shards.zset.clear();
                        shards.zset_score.clear();
                    }
                }
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else if (sock->getClientlevel() == 1)
            {
                if (args.size() != 2)
                {
                    return return_with_slowlog(RespValue::error("ERR maybe need 'FLUSHADALL CONFIRM"));
                }
                std::string confirm = args[1].str;
                std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::toupper);
                if (confirm == "CONFIRM")
                {
                    for (int db = 0; db < DB_COUNT; db++)
                    {
                        for (auto &shards : self->getDBs()[db])
                        {
                            std::unique_lock<std::shared_mutex> lock(shards.mutex);
                            shards.store.clear();
                            shards.lists.clear();
                            shards.hash.clear();
                            shards.sets.clear();
                            shards.zset.clear();
                            shards.zset_score.clear();
                        }
                    }
                    return return_with_slowlog(RespValue::simple_string("OK"));
                }
                return return_with_slowlog(RespValue::error("ERR authentication required, maybe need 'FLUSHADALL CONFIRM"));
            }
            return return_with_slowlog(RespValue::error("ERR authentication required"));
        }
        else if (cmd == "DBSIZE") // DBSIZE, 当前数据库大小
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'DBSIZE'"));
            }
            int64_t count = 0;
            for (auto &shards : self->getDBs()[sock->getClientId()])
            {
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                count += shards.store.size() + shards.lists.size() + shards.hash.size() + shards.zset.size();
            }
            return return_with_slowlog(RespValue::integer(count));
        }
        else if (cmd == "EXPIRE") // EXPIRE key seconds, 设置key的过期时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'EXPIRE'"));
            }
            int64_t second;
            try
            {
                second = std::stoll(args[2].str);
                if (second <= 0)
                {
                    return return_with_slowlog(RespValue::error("ERR invalid expire time"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            it->second.expire = SteadyClock::now() + std::chrono::seconds(second);
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "TTL") // TTL key, 查看key的过期时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'TTL'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(-2));
            }
            else if (!(it->second.expire.has_value()))
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            auto &data = it->second;
            // 计算剩余秒数
            auto now = SteadyClock::now();
            if (now >= data.expire.value())
            {
                return return_with_slowlog(RespValue::integer(-2));
            }

            auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                                 data.expire.value() - now)
                                 .count();

            return return_with_slowlog(RespValue::integer(remaining));
        }
        else if (cmd == "PEXPIRE") // PEXPIRE key milliseconds
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'PEXPIRE'"));
            }
            int64_t milliseconds;
            try
            {
                milliseconds = std::stoll(args[2].str);
                if (milliseconds <= 0)
                {
                    return return_with_slowlog(RespValue::error("ERR invalid expire time"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            shards.store[key].expire = SteadyClock::now() + std::chrono::milliseconds(milliseconds);
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "PTTL") // PTTL key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'PTTL'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::shared_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            if (it == shards.store.end())
            {
                return return_with_slowlog(RespValue::integer(-2));
            }
            else if (!(it->second.expire.has_value()))
            {
                return return_with_slowlog(RespValue::integer(-1));
            }
            auto &data = it->second;
            // 计算剩余秒数
            auto now = SteadyClock::now();
            if (now >= data.expire.value_or(TimePoint::max()))
            {
                return return_with_slowlog(RespValue::integer(-2));
            }

            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 data.expire.value() - now)
                                 .count();

            return return_with_slowlog(RespValue::integer(remaining));
        }
        else if (cmd == "PERSIST") // PERSIST key, 撤销key的过期时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of argument for 'PERSIST'"));
            }
            const std::string &key = args[1].str;
            auto &shards = self->getShard(key, sock);
            std::unique_lock<std::shared_mutex> lock(shards.mutex);
            auto it = shards.store.find(key);
            // 不存在或没有过期时间
            if (it == shards.store.end() ||  !(it->second.expire.has_value()))
            {
                return return_with_slowlog(RespValue::integer(0));
            }
            it->second.expire = std::nullopt;
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "RENAME") // RENAME key newkey
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RENAME'"));
            }
            const std::string &key = args[1].str;
            const std::string &newkey = args[2].str;

            if (key == newkey)
            {
                return return_with_slowlog(RespValue::simple_string("OK"));
            }

            int old_shard_idx = self->getShardIndex(key);
            int new_shard_idx = self->getShardIndex(newkey);

            // 按顺序锁，避免死锁
            int first = old_shard_idx;
            int second = new_shard_idx;
            if (first > second)
            {
                std::swap(first, second);
            }
            auto &m_shards = self->getDBs()[sock->getClientId()];
            std::unique_lock<std::shared_mutex> lock1(m_shards[first].mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (old_shard_idx != new_shard_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(m_shards[second].mutex);
            }

            auto &old_shard = m_shards[old_shard_idx];
            auto &new_shard = m_shards[new_shard_idx];

            // 检查 key 是否存在
            bool exists = false;
            int type = -1; // 0:string, 1:hash, 2:list, 3:set, 4:zset

            if (old_shard.store.find(key) != old_shard.store.end())
            {
                exists = true;
                type = 0;
            }
            else if (old_shard.hash.find(key) != old_shard.hash.end())
            {
                exists = true;
                type = 1;
            }
            else if (old_shard.lists.find(key) != old_shard.lists.end())
            {
                exists = true;
                type = 2;
            }
            else if (old_shard.sets.find(key) != old_shard.sets.end())
            {
                exists = true;
                type = 3;
            }
            else if (old_shard.zset.find(key) != old_shard.zset.end())
            {
                exists = true;
                type = 4;
            }

            if (!exists)
            {
                return return_with_slowlog(RespValue::error("ERR no such key"));
            }

            // 删除 newkey（如果存在）
            if (new_shard.store.find(newkey) != new_shard.store.end())
            {
                new_shard.store.erase(newkey);
            }
            else if (new_shard.hash.find(newkey) != new_shard.hash.end())
            {
                new_shard.hash.erase(newkey);
            }
            else if (new_shard.lists.find(newkey) != new_shard.lists.end())
            {
                new_shard.lists.erase(newkey);
            }
            else if (new_shard.sets.find(newkey) != new_shard.sets.end())
            {
                new_shard.sets.erase(newkey);
            }
            else if (new_shard.zset.find(newkey) != new_shard.zset.end())
            {
                new_shard.zset.erase(newkey);
                new_shard.zset_score.erase(newkey);
            }
            // new_shard.expire.erase(newkey);

            // 移动数据
            if (old_shard_idx == new_shard_idx)
            {
                // 同分片：直接移动
                if (type == 0)
                {
                    new_shard.store[newkey] = std::move(old_shard.store[key]);
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = std::move(old_shard.hash[key]);
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = std::move(old_shard.lists[key]);
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = std::move(old_shard.sets[key]);
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                // // 移动过期时间
                // auto expire_it = old_shard.expire.find(key);
                // if (expire_it != old_shard.expire.end())
                // {
                //     new_shard.expire[newkey] = expire_it->second;
                //     old_shard.expire.erase(expire_it);
                // }
            }
            else
            {
                // 跨分片：复制到新分片，删除旧分片
                if (type == 0)
                {
                    new_shard.store[newkey] = old_shard.store[key];
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = old_shard.hash[key];
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = old_shard.lists[key];
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = old_shard.sets[key];
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                // auto expire_it = old_shard.expire.find(key);
                // if (expire_it != old_shard.expire.end())
                // {
                //     new_shard.expire[newkey] = expire_it->second;
                //     old_shard.expire.erase(expire_it);
                // }
            }
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "RENAMENX") // RENAMENX key newkey 当newkey不存在时创建
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RENAME'"));
            }
            const std::string &key = args[1].str;
            const std::string &newkey = args[2].str;

            if (key == newkey)
            {
                return return_with_slowlog(RespValue::integer(1));
            }

            int old_shard_idx = self->getShardIndex(key);
            int new_shard_idx = self->getShardIndex(newkey);

            // 按顺序锁，避免死锁
            int first = old_shard_idx;
            int second = new_shard_idx;
            if (first > second)
            {
                std::swap(first, second);
            }
            auto &m_shards = self->getDBs()[sock->getClientId()];
            std::unique_lock<std::shared_mutex> lock1(m_shards[first].mutex);
            std::unique_lock<std::shared_mutex> lock2;
            if (old_shard_idx != new_shard_idx)
            {
                lock2 = std::unique_lock<std::shared_mutex>(m_shards[second].mutex);
            }

            auto &old_shard = m_shards[old_shard_idx];
            auto &new_shard = m_shards[new_shard_idx];

            // 检查 key 是否存在
            bool exists = false;
            int type = -1; // 0:string, 1:hash, 2:list, 3:set, 4:zset

            if (old_shard.store.find(key) != old_shard.store.end())
            {
                exists = true;
                type = 0;
            }
            else if (old_shard.hash.find(key) != old_shard.hash.end())
            {
                exists = true;
                type = 1;
            }
            else if (old_shard.lists.find(key) != old_shard.lists.end())
            {
                exists = true;
                type = 2;
            }
            else if (old_shard.sets.find(key) != old_shard.sets.end())
            {
                exists = true;
                type = 3;
            }
            else if (old_shard.zset.find(key) != old_shard.zset.end())
            {
                exists = true;
                type = 4;
            }

            if (!exists)
            {
                return return_with_slowlog(RespValue::error("ERR no such key"));
            }

            // 查看newkey是否存在
            bool newkey_exists = false;
            if (new_shard.store.find(newkey) != new_shard.store.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.hash.find(newkey) != new_shard.hash.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.lists.find(newkey) != new_shard.lists.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.sets.find(newkey) != new_shard.sets.end())
            {
                newkey_exists = true;
            }
            else if (new_shard.zset.find(newkey) != new_shard.zset.end())
            {
                newkey_exists = true;
            }
            if (newkey_exists)
            {
                return return_with_slowlog(RespValue::integer(0));
            }

            // 移动数据
            if (old_shard_idx == new_shard_idx)
            {
                // 同分片：直接移动
                if (type == 0)
                {
                    new_shard.store[newkey] = std::move(old_shard.store[key]);
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = std::move(old_shard.hash[key]);
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = std::move(old_shard.lists[key]);
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = std::move(old_shard.sets[key]);
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                // // 移动过期时间
                // auto expire_it = old_shard.expire.find(key);
                // if (expire_it != old_shard.expire.end())
                // {
                //     new_shard.expire[newkey] = expire_it->second;
                //     old_shard.expire.erase(expire_it);
                // }
            }
            else
            {
                // 跨分片：复制到新分片，删除旧分片
                if (type == 0)
                {
                    new_shard.store[newkey] = old_shard.store[key];
                    old_shard.store.erase(key);
                }
                else if (type == 1)
                {
                    new_shard.hash[newkey] = old_shard.hash[key];
                    old_shard.hash.erase(key);
                }
                else if (type == 2)
                {
                    new_shard.lists[newkey] = old_shard.lists[key];
                    old_shard.lists.erase(key);
                }
                else if (type == 3)
                {
                    new_shard.sets[newkey] = old_shard.sets[key];
                    old_shard.sets.erase(key);
                }
                else if (type == 4)
                {
                    new_shard.zset[newkey] = std::move(old_shard.zset[key]);
                    new_shard.zset_score[newkey] = std::move(old_shard.zset_score[key]);
                    old_shard.zset.erase(key);
                    old_shard.zset_score.erase(key);
                }

                // auto expire_it = old_shard.expire.find(key);
                // if (expire_it != old_shard.expire.end())
                // {
                //     new_shard.expire[newkey] = expire_it->second;
                //     old_shard.expire.erase(expire_it);
                // }
            }
            return return_with_slowlog(RespValue::integer(1));
        }
        else if (cmd == "RANDOMKEY") // RANDOMKEY, 随机返回一个key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'RANDOMKEY'"));
            }
            std::vector<std::string> all_keys;
            for (auto &shards : self->getDBs()[sock->getClientId()])
            {
                std::shared_lock<std::shared_mutex> lock(shards.mutex);
                // string
                for (auto &[key, _] : shards.store)
                {
                    all_keys.push_back(key);
                }
                // hash
                for (auto &[key, _] : shards.hash)
                {
                    all_keys.push_back(key);
                }
                // lists
                for (auto &[key, _] : shards.lists)
                {
                    all_keys.push_back(key);
                }
                // sets
                for (auto &[key, _] : shards.sets)
                {
                    all_keys.push_back(key);
                }
                // zset
                for (auto &[key, _] : shards.zset)
                {
                    all_keys.push_back(key);
                }
            }
            if (all_keys.size() == 0)
            {
                return return_with_slowlog(RespValue::null_bulk());
            }
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, all_keys.size() - 1);
            return return_with_slowlog(RespValue::bulk_string(all_keys[dis(gen)]));
        }
        else if (cmd == "INFO") // INFO 返回服务器信息
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'INFO'"));
            }
            std::string info;
            // Server
            info += "# Server\r\n";
            info += "redis_version:1.0.0\r\n";
            info += "tcp_port:6666\r\n";
            info += "\r\n";

            // Client
            info += "# Client\r\n";
            info += "connections:" + std::to_string(self->getConnection()) + "\r\n";
            info += "maxclient:" + std::to_string(self->getMaxClientCount()) + "\r\n";
            info += "reject_connections:" + std::to_string(self->getRejectConnection()) + "\r\n";
            info += "\r\n";

            // AOF
            info += "# AOF\r\n";
            info += "aof_enabled:" + std::string(self->getAOF().getConfig_AOFEnabled() ? "1" : "0") + "\r\n";
            info += "aof_sync:" + self->getAOF().getConfig_AOFSync() + "\r\n";
            info += "aof_current_file:" + self->getAOF().getCurrentFileName() + "\r\n";
            info += "aof_file_index:" + std::to_string(self->getAOF().getCurrentFileIdx()) + "\r\n";
            info += "aof_current_size:" + std::to_string(self->getAOF().getCurrentFileSize()) + "\r\n";
            info += "aof_max_file_size:" + std::to_string(self->getAOF().getConfig_AOFMaxFileSize()) + "\r\n";
            info += "aof_max_files:" + std::to_string(self->getAOF().getConfig_AOFMaxFileNumber()) + "\r\n";
            info += "aof_max_buffer_size" + std::to_string(self->getAOF().getMaxAOFBufferSize()) + "\r\n";
            info += "\r\n";

            // Replication
            info += "# Replication\r\n";
            
            if (self->getReplication().getisMaster()) 
            {
                info += "role:master\r\n";
                info += "connected_slaves:" + std::to_string(self->getReplication().slavesCount()) + "\r\n";
                
                // 列出所有从节点
                auto slaves_info = self->getReplication().slavesToString();
                if (!slaves_info.empty()) 
                {
                    info += slaves_info;
                }
            } 
            else 
            {
                info += "role:slave\r\n";
                info += "master_host:" + self->getReplication().getMasterHost() + "\r\n";
                info += "master_port:" + std::to_string(self->getReplication().getMasterPort()) + "\r\n";
                info += "master_link_status:" + std::string(
                    self->getReplication().getReplState() == self->getReplication().getOnline() ? "up" : "down"
                ) + "\r\n";
                info += "slave_repl_offset:" + std::to_string(self->getReplication().getReplOffset()) + "\r\n";
            }

            info += "\r\n";

            // Monitor
            info += "# Monitor\r\n";
            info += "monitor_clients:" + std::to_string(self->getMonitor().size()) + "\r\n";
            info += "\r\n";

            // Stats
            info += "# Stats\r\n";
            info += "total_connections_received:" + std::to_string(self->getConnection()) + "\r\n";
            info += "total_commands_processed:" + std::to_string(self->getCommands().load(std::memory_order_acquire)) + "\r\n";
            info += "\r\n";

            // Memory
            info += "# Memory\r\n";
            size_t total_keys = 0;
            for (auto &shard : self->getDBs()[sock->getClientId()])
            {
                std::shared_lock<std::shared_mutex> lock(shard.mutex);
                total_keys += shard.store.size() + shard.hash.size() + shard.lists.size() + shard.sets.size() + shard.zset.size();
            }
            info += "total_keys:" + std::to_string(total_keys) + "\r\n";
            return return_with_slowlog(RespValue::bulk_string(info));
        }
        else if (cmd == "SAVE") // SAVE 持久化
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SAVE'"));
            }
            self->saveToFile();
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "BGSAVE") // BGSAVE 后台异步保存
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'BGSAVE'"));
            }
            if (self->getBgSaveRunning().load(std::memory_order_acquire))
            {
                return return_with_slowlog(RespValue::error("ERR Background save already in progress"));
            }
            self->setBgSaveRunning(true);
            std::weak_ptr<ServerData<int>> weak_self = self;
            std::thread([weak_self]
                        {
                auto ptr = weak_self.lock();
                if (ptr)
                {
                    ptr->saveToFile();
                    ptr->setBgSaveRunning(false);
                    BLUE_LOG_INFO(xx::g_logger) << "BGSAVE completed";
                }
                else
                {
                    BLUE_LOG_INFO(xx::g_logger) << "BGSAVE: ServerData already destroyed";
                } })
                .detach();
            return return_with_slowlog(RespValue::simple_string("Background saving started"));
        }
        else if (cmd == "LASTSAVE") // LASTSAVE 上一次保存成功时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LASTSAVE'"));
            }
            return return_with_slowlog(RespValue::integer(self->getLastSaveTime()));
        }
        else if (cmd == "LASTSAVE1") // LASTVATE1 上一次保存成功时间(北京时间)
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LASTSAVE1'"));
            }
            std::time_t beijing_t = self->getLastSaveTime() + 8 * 3600;
            std::tm time_local;
#ifdef _WIN32
            gmtime_s(&time_local, &beijing_t);
#else
            gmtime_r(&beijing_t, &time_local);
#endif
            std::ostringstream os;
            os << std::put_time(&time_local, "%Y-%m-%d %H:%M:%S");
            return return_with_slowlog(RespValue::bulk_string(os.str()));
        }
        else if (cmd == "COMMAND")
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'COMMAND'"));
            }

            std::vector<RespValue> commands;

            // 返回所有支持的命令列表
            static const std::vector<std::string> cmd_list = {
                // 连接
                "PING", "AUTH", "SELECT", "CLIENT", "CONFIG",
                // string
                "SET", "GET", "MSET", "MGET", "APPEND", "SETNX",
                "INCR", "INCRBY", "DEL", "EXISTS", "STRLEN", "TYPE",
                "GETSET",
                // hash
                "HSET", "HGET", "HGETALL", "HDEL", "HLEN", "HEXISTS", "HKEYS", "HVALS",
                // list
                "LPUSH", "RPUSH", "LPOP", "RPOP", "LRANGE", "LLEN", "LINSERT", "LINDEX", "LSET",
                "LPOPRPUSH", "RPOPLPUSH",
                // set
                "SADD", "SMEMBERS", "SREM", "SISMEMBER", "SCARD", "SRANDMEMBER", "SPOP", "SDIFF", "SINTER", "SUNION", "SMOVE",
                // zset
                "ZADD", "ZRANGE", "ZREM", "ZSCORE", "ZRANK", "ZINCRBY", "ZCOUNT", "ZRANGEBYSCORE", "ZREMRANGEBYSCORE",
                "ZINCRBYFLOAT",
                // server
                "KEYS", "FLUSHDB", "FLUSHDBALL", "DBSIZE", "INFO", "SAVE", "BGSAVE", "LASTSAVE",
                "LASTSAVE1", "ECHO", "TIME", "LOCALTIME", "SHUTDOWN", "COMMAND",
                "RENAME", "RENAMENX", "RANDOMKEY", "EXPIRE", "TTL", "PEXPIRE", "PTTL",
                "PERSIST",
                // 事务模式
                "MULTI", "EXEC", "DISCARD", "WATCH", "UNWATCH",
                // 订阅模式
                "SUBSCRIBE", "PUBLISH", "UNSUBSCRIBE",
                // 慢查询
                "SLOWLOG",
                // 监控模式
                "MONITOR",
                // 主从复制
                "REPLICAOF", "SLAVEOF", "SYNC"};

            for (const auto &name : cmd_list)
            {
                commands.push_back(RespValue::bulk_string(name));
            }

            return return_with_slowlog(RespValue::array(std::move(commands)));
        }
        else if (cmd == "ECHO") // ECHO meaasge
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'ECHO'"));
            }
            return return_with_slowlog(RespValue::bulk_string(args[1].str));
        }
        else if (cmd == "TIME") // TIME, 返回当前服务器的秒和微妙
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'TIME'"));
            }
            auto now = SteadyClock::now();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
            std::vector<RespValue> results;
            results.push_back(RespValue::bulk_string(std::to_string(seconds)));
            results.push_back(RespValue::bulk_string(std::to_string(microseconds)));
            return return_with_slowlog(RespValue::array(std::move(results)));
        }
        else if (cmd == "LOCALTIME") // LOCALTIME, 返回当前北京时间
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'LOCALTIME'"));
            }
            auto now = std::chrono::system_clock::now();
            auto now_t = std::chrono::system_clock::to_time_t(now);

            std::time_t beijing_t = now_t + 8 * 3600;
            std::tm time_local;
#ifdef _WIN32
            gmtime_s(&time_local, &beijing_t);
#else
            gmtime_r(&beijing_t, &time_local);
#endif
            std::ostringstream os;
            os << std::put_time(&time_local, "%Y-%m-%d %H:%M:%S");
            return return_with_slowlog(RespValue::bulk_string(os.str()));
        }
        else if (cmd == "WATCH") // WATCH key [key...], 监视key
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'WATCH'"));
            }
            sock->clearWatchedKey();

            for (size_t i = 1; i < args.size(); i++)
            {
                const std::string key = args[i].str;
                sock->addWatchKey(key, self->getKeyVersion(key, sock));
            }
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "UNWATCH") // UNWATCH, 取消所有监视
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'UNWATCH'"));
            }
            sock->clearWatchedKey();
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "SLOWLOG") // SLOWLOG [GET/LEN/RESET]
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() < 2)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SLOWLOG'"));
            }
            std::string sub_cmd = args[1].str;
            std::transform(sub_cmd.begin(), sub_cmd.end(), sub_cmd.begin(), ::toupper);
            if (sub_cmd == "GET")
            {
                // 同步数据
                self->getSlowLog().syncSlowLogs();

                int64_t count = 10;
                if (args.size() >= 3)
                {
                    try
                    {
                        count = std::stoll(args[2].str);
                        if (count < 0)
                        {
                            return return_with_slowlog(RespValue::error("ERR count must be >= 0"));
                        }
                    }
                    catch (...)
                    {
                        return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
                    }
                }

                std::vector<RespValue> results = self->getSlowLog().getSlowLogs(count);
                return return_with_slowlog(RespValue::array(std::move(results)));
            }

            else if (sub_cmd == "LEN")
            {
                self->getSlowLog().syncSlowLogs();
                return return_with_slowlog(RespValue::integer(self->getSlowLog().len()));
            }
            else if (sub_cmd == "RESET")
            {
                self->getSlowLog().reset();
                return return_with_slowlog(RespValue::simple_string("OK"));
            }
            else
            {
                return return_with_slowlog(RespValue::error("ERR unknown SLOWLOG subcommand"));
            }
        }
        else if (cmd == "MONITOR") // MONITOR   实时监控和调试服务器
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'MONITOR'"));
            }

            // 添加monitor client
            self->getMonitor().addMonitorClient(sock);

            sock->setMonitorMode(true);

            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "AOFROTATE") // AOFROTATE 异步轮转AOF日志(不阻塞协程)
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'AOFROTATE'"));
            }
            if (!self->getAOF().getConfig_AOFEnabled())
            {
                return return_with_slowlog(RespValue::error("ERR AOF is disabled"));
            }
            if (self->getAOF().getAOFRotating())
            {
                return return_with_slowlog(RespValue::error("ERR AOF rotation already in progress"));
            }
            std::weak_ptr<ServerData<int>> weak_self = self;
            std::thread([weak_self]
                        {
                auto ptr = weak_self.lock();
                if (ptr)
                {
                    ptr->getAOF().rotateAOF();
                    BLUE_LOG_INFO(xx::g_logger) << "AOF rotation finished"; 
                } })
                .detach();
            return return_with_slowlog(RespValue::simple_string("AOF rotation started"));
        }
        else if (cmd == "REPLICAOF" || cmd == "SLAVEOF")
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR permission denied"));
            }
            if (args.size() != 3)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'REPLICAOF'"));
            }
            std::string host = args[1].str;
            std::string port_str = args[2].str;

            // REPLICAOF NO ONE 取消复制
            if (host == "NO" && port_str == "ONE")
            {
                if (!self->getReplication().getisMaster())
                {
                    self->getReplication().stopReplication();
                    self->getReplication().setisMaster(true);
                    BLUE_LOG_INFO(xx::g_logger) << "Replication stopped, now master";
                }
                return return_with_slowlog(RespValue::simple_string("OK"));
            }

            int32_t port;
            try
            {
                port = std::stoi(port_str);
                if (port < 0 || port > UINT16_MAX)
                {
                    return return_with_slowlog(RespValue::error("ERR value is invalid"));
                }
            }
            catch (...)
            {
                return return_with_slowlog(RespValue::error("ERR value is not an integer or out of range"));
            }

            // 如果已经是从节点且连接到同一个主节点，忽略
            if (!self->getReplication().getisMaster() &&
                self->getReplication().getMasterHost() == host &&
                self->getReplication().getMasterPort() == port)
            {
                return return_with_slowlog(RespValue::simple_string("OK"));
            }

            // 停止旧的复制
            if (!self->getReplication().getisMaster())
            {
                self->getReplication().stopReplication();
            }

            // 设置新的配置
            self->getReplication().setisMaster(false);
            self->getReplication().setMasterHost(host);
            self->getReplication().setMasterPort(static_cast<uint16_t>(port));
            self->getReplication().setReplOffset(0);

            // 启动复制
            self->getReplication().startReplication();

            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "SYNC") // SYNC 主节点处理
        {
            if (sock->getClientlevel() < 1)
            {
                return return_with_slowlog(RespValue::error("ERR authentication required"));
            }

            // 只有主节点接收SYNC命令
            if (!(self->getReplication().getisMaster()))
            {
                return return_with_slowlog(RespValue::error("ERR not master"));
            }

            BLUE_LOG_INFO(xx::g_logger) << "SYNC request from slave, fd=" << sock->getSocketfd();

            // 主节点保存从节点连接
            self->getReplication().addSlaves(sock);

            // 生成 RDB 数据并发送
            std::string rdb_data = self->generateRDB(); // 拷贝一份

            // 发送 RDB 格式: $<length>\r\n<data>
            std::string response = "$" + std::to_string(rdb_data.size()) + "\r\n" + rdb_data;

            // 非阻塞同步发送(在tcpServer中的startAccept中对sock fd设置了非阻塞)
            ssize_t sent = ::send(sock->getSocketfd(), response.data(), response.size(), MSG_NOSIGNAL);
            if (sent <= 0)
            {
                BLUE_LOG_ERROR(xx::g_logger) << "Failed to send RDB to slave";

                // 从从节点列表中删除从节点
                self->getReplication().remove(sock);

                // 返回失败给从节点
                return return_with_slowlog(RespValue::error("ERR failed to send RDB"));
            }

            BLUE_LOG_INFO(xx::g_logger) << "SYNC: RDB sent to slave, " << rdb_data.size() << " bytes";
            return return_with_slowlog(RespValue::simple_string("OK"));
        }
        else if (cmd == "SHUTDOWN") // SHUTDOWN 关闭服务器,如果连接数为0
        {
            if (!self->isAdmin(sock))
            {
                return return_with_slowlog(RespValue::error("ERR permission denied"));
            }
            if (args.size() != 1)
            {
                return return_with_slowlog(RespValue::error("ERR wrong number of arguments for 'SHUTDOWN'"));
            }
            self->setShutdown(true);
            return return_with_slowlog(RespValue::bulk_string("OK - waiting for clients to disconnect"));
        }
        return return_with_slowlog(RespValue::error("ERR unknown command"));
    }
}