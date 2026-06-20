#include <iomanip>
#include <chrono>
#include <stdlib.h>
#include <execinfo.h>
#include <sys/time.h>
#include "util.h"
#include "log.h"

// 辅助函数
namespace blue
{
    blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    // 进程id
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    // 协程id
    uint32_t GetFiberID()
    {
        return 0;
    }

    // bt : 存放调用栈内容 size : 大小 skip : 跳过的行数(默认1,跳过一行)
    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **array = (void **)malloc(sizeof(void *) * size);
        ssize_t s = ::backtrace(array, size);

        char **string = ::backtrace_symbols(array, s);
        if (string == NULL)
        {
            BLUE_LOG_ERROR(g_logger) << "backtrace_symbols error";
            free(array);
            return;
        }

        for (size_t i = skip; i < s; i++)
        {
            bt.push_back(string[i]);
        }

        free(string);
        free(array);
    }

    // size : 大小 skip : 跳过的行数(默认2,跳过2行) prefix : 可选的前缀 return : string
    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        if (bt.empty())
        {
            return std::string();
        }
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); i++)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMs()
    {
        struct timeval v;
        gettimeofday(&v, NULL);
        return v.tv_sec * 1000ul + v.tv_usec / 1000ul;
    }
    uint64_t GetCurrentUs()
    {
        struct timeval v;
        gettimeofday(&v, NULL);
        return v.tv_sec * 1000ul * 1000ul + v.tv_usec;
    }
    uint64_t GetCurrentMsbyc()
    {
        struct timespec tsp;
        clock_gettime(CLOCK_MONOTONIC, &tsp);
        return tsp.tv_sec * 1000ul + tsp.tv_nsec / 1000000ul;
    }
    uint64_t GetCurrentUsbyc()
    {
        struct timespec tsp;
        clock_gettime(CLOCK_MONOTONIC, &tsp);
        return tsp.tv_sec * 1000ul * 1000ul + tsp.tv_nsec / 1000ul;
    }
    uint64_t GetCurrentNsbyc()
    {
        struct timespec tsp;
        clock_gettime(CLOCK_MONOTONIC, &tsp);
        return tsp.tv_sec * 1000ul * 1000ul * 1000ul + tsp.tv_nsec;
    }
    std::string GetCurrentBeiJingTime()
    {
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
        return os.str();
    }

}