#include "async_logger.h"
#include "logentry.h"
#include <cstring>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace blue
{

    AsyncLogger::AsyncLogger(const std::string &file_pattern,
                             size_t max_file_size,
                             size_t queue_size)
        : m_filePattern(file_pattern), m_maxFileSize(max_file_size), m_queue(queue_size)
    {
        rotate();
        m_worker = std::thread(&AsyncLogger::run, this);
    }

    AsyncLogger::~AsyncLogger()
    {
        stop();
    }

    void AsyncLogger::stop()
    {
        m_stop.store(true, std::memory_order_release);
        if (m_worker.joinable())
        {
            m_worker.join();
        }
        if (m_file.is_open())
        {
            m_file.flush();
            m_file.close();
        }
    }

    void AsyncLogger::enqueue(LogEntry &&entry)
    {
        // 尝试放入队列，失败则丢弃
        if (!m_queue.push(std::move(entry)))
        {
            // 可选：记录丢弃数量
            static std::atomic<uint64_t> dropped{0};
            dropped.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void AsyncLogger::run()
    {
        constexpr size_t BATCH_SIZE = 64;
        LogEntry batch[BATCH_SIZE];
        std::string buffer;
        buffer.reserve(65536);

        while (!m_stop.load(std::memory_order_acquire))
        {
            size_t count = m_queue.popBatch(batch, BATCH_SIZE);

            if (count == 0)
            {
                // 队列空，短暂休眠
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // 批量处理
            for (size_t i = 0; i < count; ++i)
            {
                std::string formatted = batch[i].format();

                // 写入文件
                writeToFile(formatted);

                // 额外回调（如控制台输出）
                if (m_callback)
                {
                    m_callback(batch[i]);
                }
            }
        }

        // 退出前清空队列
        size_t remaining;
        while ((remaining = m_queue.popBatch(batch, BATCH_SIZE)) > 0)
        {
            for (size_t i = 0; i < remaining; ++i)
            {
                writeToFile(batch[i].format());
            }
        }
        m_file.flush();
    }

    void AsyncLogger::writeToFile(const std::string &data)
    {
        if (!m_file.is_open())
        {
            rotate();
        }
        m_file << data;
        m_currentSize += data.size();

        // 检查是否需要轮转
        if (m_currentSize >= m_maxFileSize)
        {
            m_file.flush();
            rotate();
        }
    }

    void AsyncLogger::rotate()
    {
        if (m_file.is_open())
        {
            m_file.close();
        }

        std::string filename = generateFileName();
        std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
        m_file.open(filename, std::ios::app);
        m_currentSize = 0;

        if (!m_file.is_open())
        {
            // 回退：输出到 stderr
            fprintf(stderr, "AsyncLogger: failed to open %s\n", filename.c_str());
        }
    }

    std::string AsyncLogger::generateFileName() const
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&t, &tm);

        char buf[256];
        // 简单替换 %Y%m%d_%H%M%S
        std::string pattern = m_filePattern;

        auto replace = [&](const char *key, const char *val)
        {
            size_t pos = pattern.find(key);
            if (pos != std::string::npos)
            {
                pattern.replace(pos, strlen(key), val);
            }
        };

        snprintf(buf, sizeof(buf), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        replace("%Y%m%d", buf);

        snprintf(buf, sizeof(buf), "%02d%02d%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        replace("%H%M%S", buf);

        return pattern;
    }

    uint64_t AsyncLogger::currentTimeMs()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
    }

} // namespace blue