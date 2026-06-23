#include "slowlog.h"

namespace blue
{
    void SlowLogModule::syncSlowLogs()
    {
        std::unique_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
        SlowLogEntry entry;
        size_t max_len = m_slow_log_max_len.load(std::memory_order_acquire);
        while (m_slow_logs.pop(entry))
        {
            m_slow_logs_cache.push_back(std::move(entry));
            if (m_slow_logs_cache.size() > max_len)
            {
                m_slow_logs_cache.erase(m_slow_logs_cache.begin());
            }
        }
    }

    void SlowLogModule::pushEntry(const std::string &cmd_str, MSocket::MSocketPtr sock, 
                            std::chrono::steady_clock::time_point start, 
                            std::chrono::steady_clock::time_point end)
    {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (duration.count() > m_slow_log_slower_than.load(std::memory_order_acquire))
        {
            SlowLogEntry entry{
                ++m_slow_log_id,
                std::chrono::system_clock::now(),
                duration,
                cmd_str,
                sock->getRemoteAddress()->toString()};

            m_slow_logs.push(entry);
        }
    }

    std::vector<RespValue> SlowLogModule::getSlowLogs(int64_t count)
    {
        std::vector<RespValue> results;
        std::shared_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
        size_t start = m_slow_logs_cache.size() - std::min((size_t)(count), m_slow_logs_cache.size());
        for (size_t i = start; i < m_slow_logs_cache.size(); i++)
        {
            const auto &entry = m_slow_logs_cache[i];
            std::vector<RespValue> log_entry;

            // ID
            log_entry.push_back(RespValue::integer(entry.id));

            // 时间戳微秒
            auto ts = std::chrono::duration_cast<std::chrono::microseconds>(
                            entry.timestamp.time_since_epoch())
                            .count();
            log_entry.push_back(RespValue::integer(ts));

            // 执行时间(微秒)
            log_entry.push_back(RespValue::integer(entry.duration.count()));

            // 命令
            log_entry.push_back(RespValue::bulk_string(entry.command));

            // 客户端ip
            log_entry.push_back(RespValue::bulk_string(entry.client_ip));

            results.push_back(RespValue::array(std::move(log_entry)));
        }
        return results;
    }

    size_t SlowLogModule::len() const
    {
        std::shared_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
        return m_slow_logs_cache.size();
    }

    void SlowLogModule::reset()
    {
        std::unique_lock<std::shared_mutex> lock(m_slow_logs_cache_mutex);
        m_slow_logs_cache.clear();
        // 清空 SPSCQueue
        SlowLogEntry entry;
        while (m_slow_logs.pop(entry))
        {
        }
        m_slow_log_id.store(0, std::memory_order_release);
    }
}