#include "blue/blue.h"
#include "http/httpconnection.h"
#include "blue/configinit.h"
#include "blue/io_manager.h"
#include "blue/task.h"
#include "blue/await.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <memory>


static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

// ======================== 数据结构 ========================

struct BenchResult
{
    std::string name;
    int concurrency;
    int worker_threads;
    int total_requests;
    int success;
    int failed;
    long long time_ms;
    double qps;
    double avg_latency_ms;
    double p50_latency_ms;
    double p99_latency_ms;
    double throughput_mbps;
    double error_rate;
};

double calc_percentile(std::vector<long long> &latencies, double percentile)
{
    if (latencies.empty()) return 0;
    std::sort(latencies.begin(), latencies.end());
    size_t idx = static_cast<size_t>(latencies.size() * percentile / 100.0);
    if (idx >= latencies.size()) idx = latencies.size() - 1;
    return latencies[idx] / 1000.0;
}

// ======================== 毫秒级 sleep ========================

struct SleepAwaiterMs
{
    uint64_t ms;
    SleepAwaiterMs(uint64_t m) : ms(m) {}
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h)
    {
        blue::IOManager::GetThis()->addTimer(ms, h, nullptr);
    }
    void await_resume() const noexcept {}
};

inline SleepAwaiterMs sleepForMs(uint64_t ms)
{
    return SleepAwaiterMs{ms};
}

// ======================== 共享状态 ========================

struct RequestState
{
    std::string url;
    std::string name;
    std::atomic<int> success{0};
    std::atomic<int> failed{0};
    std::atomic<long long> total_bytes{0};
    std::atomic<long long> total_latency{0};
    std::atomic<int> done{0};
    std::vector<long long> all_latencies;
    std::mutex latency_mutex;
    int total_requests;
    int max_concurrency;
    std::atomic<int> current_concurrency{0};
    std::mutex concurrency_mutex;
    std::condition_variable concurrency_cv;
    std::vector<std::string> errors;
    std::mutex error_mutex;

    RequestState(std::string u, std::string n, int total, int concurrency)
        : url(std::move(u)), name(std::move(n)), total_requests(total), max_concurrency(concurrency)
    {
        all_latencies.reserve(total);
        errors.reserve(10);
    }
};

// ======================== 单次 benchmark ========================

std::vector<BenchResult> results;
std::mutex results_mutex;

// 单个请求的执行函数
blue::Task<void> execute_request(std::shared_ptr<RequestState> state, std::shared_ptr<blue::http::HttpConnectionPool> pool)
{
    // 等待并发控制
    {
        std::unique_lock<std::mutex> lock(state->concurrency_mutex);
        state->concurrency_cv.wait(lock, [&state] { 
            return state->current_concurrency < state->max_concurrency; 
        });
        state->current_concurrency++;
    }

    auto req_start = std::chrono::steady_clock::now();
    
    auto result = co_await pool->doGet(state->url, 30000);
    
    auto req_end = std::chrono::steady_clock::now();
    long long latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
        req_end - req_start).count();

    if (result->result == 0 && result->response)
    {
        state->success.fetch_add(1, std::memory_order_relaxed);
        state->total_bytes.fetch_add(result->response->getBody().size(), std::memory_order_relaxed);
        state->total_latency.fetch_add(latency_us, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(state->latency_mutex);
            state->all_latencies.push_back(latency_us);
        }
    }
    else
    {
        state->failed.fetch_add(1, std::memory_order_relaxed);
        
        // 收集错误信息
        {
            std::lock_guard<std::mutex> lock(state->error_mutex);
            if (state->errors.size() < 10) {
                std::string error_msg = "result=" + std::to_string(result->result) + 
                                       " error=" + result->error;
                state->errors.push_back(error_msg);
            }
        }
    }
    
    state->done.fetch_add(1, std::memory_order_relaxed);
    
    // 释放并发控制
    {
        std::unique_lock<std::mutex> lock(state->concurrency_mutex);
        state->current_concurrency--;
        state->concurrency_cv.notify_one();
    }
    
    co_return;
}

blue::Task<void> benchmark(std::string name, std::string url,
                           int concurrency, int total_requests,
                        std::shared_ptr<blue::http::HttpConnectionPool> pool)
{
    BLUE_LOG_INFO(g_logger) << "[" << name << "] Starting... concurrency="
                            << concurrency << " total=" << total_requests;

    auto state = std::make_shared<RequestState>(url, name, total_requests, concurrency);

    auto start = std::chrono::steady_clock::now();
    auto *iom = blue::IOManager::GetThis();

    // 使用小批次调度
    int batch_size = std::min(concurrency / 2, 25);
    for (int i = 0; i < total_requests; i += batch_size)
    {
        int current_batch = std::min(batch_size, total_requests - i);
        
        for (int j = 0; j < current_batch; j++)
        {
            iom->schedule(execute_request(state,pool));
        }

        co_await sleepForMs(10);
    }

    // 等待所有请求完成
    int timeout_count = 0;
    int max_timeout = 600; // 最多等待60秒
    while (state->done.load(std::memory_order_relaxed) < total_requests && timeout_count < max_timeout)
    {
        co_await sleepForMs(100);
        timeout_count++;
        
        if (timeout_count % 50 == 0 && state->done.load() > 0)
        {
            BLUE_LOG_INFO(g_logger) << "[" << name << "] Progress: " 
                                   << state->done.load() << "/" << total_requests;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    BenchResult r;
    r.name = name;
    r.concurrency = concurrency;
    r.worker_threads = iom->GetThreadCount();
    r.total_requests = total_requests;
    r.success = state->success.load();
    r.failed = state->failed.load();
    r.time_ms = total_ms;
    r.qps = (total_ms > 0) ? (r.success * 1000.0 / total_ms) : 0;
    r.avg_latency_ms = (r.success > 0) ? (state->total_latency.load() / r.success / 1000.0) : 0;
    r.p50_latency_ms = calc_percentile(state->all_latencies, 50.0);
    r.p99_latency_ms = calc_percentile(state->all_latencies, 99.0);
    r.throughput_mbps = (total_ms > 0) ? (state->total_bytes.load() / (total_ms / 1000.0) / 1024 / 1024) : 0;
    r.error_rate = (r.total_requests > 0) ? (r.failed * 100.0 / r.total_requests) : 0;

    {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(r);
    }

    BLUE_LOG_INFO(g_logger) << "\n┌──── " << name << " ────┐";
    BLUE_LOG_INFO(g_logger) << "│ Total:   " << std::setw(8) << total_requests << "              │";
    BLUE_LOG_INFO(g_logger) << "│ Success: " << std::setw(8) << r.success << " (" 
                            << std::fixed << std::setprecision(1) << (100.0 - r.error_rate) << "%)  │";
    BLUE_LOG_INFO(g_logger) << "│ Failed:  " << std::setw(8) << r.failed << "              │";
    BLUE_LOG_INFO(g_logger) << "│ Time:    " << std::setw(8) << total_ms << " ms           │";
    BLUE_LOG_INFO(g_logger) << "│ QPS:     " << std::setw(8) << std::fixed << std::setprecision(1) 
                            << r.qps << " req/s     │";
    if (r.success > 0) {
        BLUE_LOG_INFO(g_logger) << "│ Avg:     " << std::setw(8) << r.avg_latency_ms << " ms        │";
        BLUE_LOG_INFO(g_logger) << "│ P50:     " << std::setw(8) << r.p50_latency_ms << " ms        │";
        BLUE_LOG_INFO(g_logger) << "│ P99:     " << std::setw(8) << r.p99_latency_ms << " ms        │";
        BLUE_LOG_INFO(g_logger) << "│ TP:      " << std::setw(8) << std::fixed << std::setprecision(2) 
                                << r.throughput_mbps << " MB/s     │";
    }
    BLUE_LOG_INFO(g_logger) << "└──────────────────────────────┘";

    if (r.failed > 0 && !state->errors.empty())
    {
        BLUE_LOG_WARN(g_logger) << "[" << name << "] First few errors:";
        for (size_t i = 0; i < state->errors.size() && i < 5; i++)
        {
            BLUE_LOG_WARN(g_logger) << "  Error " << (i+1) << ": " << state->errors[i];
        }
    }

    co_return;
}

// ======================== 导出报告 ========================

void export_report(const std::vector<BenchResult> &results, const std::string &filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return;
    ofs << "Name,Concurrency,Threads,Total,Success,Failed,Error%,Time_ms,QPS,Avg_ms,P50_ms,P99_ms,TP_MBps\n";
    for (auto &r : results)
    {
        ofs << r.name << "," << r.concurrency << "," << r.worker_threads << ","
            << r.total_requests << "," << r.success << "," << r.failed << ","
            << std::fixed << std::setprecision(1) << r.error_rate << ","
            << r.time_ms << "," << r.qps << ","
            << r.avg_latency_ms << "," << r.p50_latency_ms << "," << r.p99_latency_ms << ","
            << std::fixed << std::setprecision(2) << r.throughput_mbps << "\n";
    }
    ofs.close();
    BLUE_LOG_INFO(g_logger) << "Report exported to " << filename;
}

// ======================== 汇总打印 ========================

void print_summary(const std::vector<BenchResult> &results)
{
    BLUE_LOG_INFO(g_logger) << "";
    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║                        STRESS TEST SUMMARY                       ║";
    BLUE_LOG_INFO(g_logger) << "╠══════════════════════════╦══════╦══════╦══════╦══════╦══════╦══════╣";
    BLUE_LOG_INFO(g_logger) << "║ " << std::left << std::setw(24) << "Name"
                            << "║" << std::right << std::setw(5) << "Total"
                            << "║" << std::setw(5) << "OK"
                            << "║" << std::setw(5) << "Fail"
                            << "║" << std::setw(5) << "QPS"
                            << "║" << std::setw(5) << "Avg"
                            << "║" << std::setw(5) << "P99" << " ║";
    BLUE_LOG_INFO(g_logger) << "╠══════════════════════════╬══════╬══════╬══════╬══════╬══════╬══════╣";

    for (auto &r : results)
    {
        char buf[512];
        if (r.success > 0) {
            snprintf(buf, sizeof(buf),
                     "║ %-24s ║ %4d ║ %4d ║ %4d ║ %4.0f ║ %4.1f ║ %4.1f ║",
                     r.name.c_str(), r.total_requests, r.success, r.failed,
                     r.qps, r.avg_latency_ms, r.p99_latency_ms);
        } else {
            snprintf(buf, sizeof(buf),
                     "║ %-24s ║ %4d ║ %4d ║ %4d ║ %4s ║ %4s ║ %4s ║",
                     r.name.c_str(), r.total_requests, r.success, r.failed,
                     "N/A", "N/A", "N/A");
        }
        BLUE_LOG_INFO(g_logger) << buf;
    }
    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════╩══════╩══════╩══════╩══════╩══════╩══════╝";
}

// ======================== main ========================

int main()
{
    blue::http::IniteConfig();

    unsigned int cpu_cores = std::thread::hardware_concurrency();
    int worker_threads = std::min(cpu_cores * 2, 16u);
    BLUE_LOG_INFO(g_logger) << "CPU Cores: " << cpu_cores << ", Worker Threads: " << worker_threads;

    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║   BLUE HTTP CLIENT STRESS TEST   ║";
    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════════════╝";

    blue::IOManager iom(worker_threads);

    // std::string url_http = "http://127.0.0.1:8080";
    // std::string url_https = "https://127.0.0.1:8080";

    std::string url_http = "http://www.baidu.com";
    std::string url_https = "https://www.baidu.com";

    // HTTP 连接池
    auto http_pool = std::make_shared<blue::http::HttpConnectionPool>(
        "www.baidu.com", "", 80, 60000, 100, "http",10);

    // HTTPS 连接池
    auto https_pool = std::make_shared<blue::http::HttpConnectionPool>(
        "www.baidu.com", "", 443, 60000, 100,"https", 10);

    // ================== HTTP 测试 ==================
    BLUE_LOG_INFO(g_logger) << "\n========== HTTP Benchmark ==========";
    
    {
        auto t1 = benchmark("HTTP-100",  url_http, 100,  1000,http_pool);
        iom.schedule(std::move(t1));
        iom.wait_all();
    }
    {
        auto t2 = benchmark("HTTP-200", url_http, 200, 2000, http_pool);
        iom.schedule(std::move(t2));
        iom.wait_all();
    }
    {
        auto t3 = benchmark("HTTP-500", url_http, 500, 5000, http_pool);
        iom.schedule(std::move(t3));
        iom.wait_all();
    }
    {
        auto t4 = benchmark("HTTP-1000", url_http, 1000, 10000, http_pool);
        iom.schedule(std::move(t4));
        iom.wait_all();
    }

    // ================== HTTPS 测试 ==================
    BLUE_LOG_INFO(g_logger) << "\n========== HTTPS Benchmark ==========";
    
    {
        auto t5 = benchmark("HTTPS-100",  url_https, 100,  1000,https_pool);
        iom.schedule(std::move(t5));
        iom.wait_all();
    }
    {
        auto t6 = benchmark("HTTPS-200", url_https, 200, 2000,https_pool);
        iom.schedule(std::move(t6));
        iom.wait_all();
    }
    {
        auto t7 = benchmark("HTTPS-500", url_https, 500, 5000,https_pool);
        iom.schedule(std::move(t7));
        iom.wait_all();
    }
    {
        auto t8 = benchmark("HTTPS-1000", url_https, 1000, 10000,https_pool);
        iom.schedule(std::move(t8));
        iom.wait_all();
    }

    print_summary(results);

    auto valid_results = std::find_if(results.begin(), results.end(),
                                      [](auto &r) { return r.success > 0; });
    
    if (valid_results != results.end()) {
        auto best = std::max_element(results.begin(), results.end(),
                                     [](auto &a, auto &b) { 
                                         return (a.success > 0 ? a.qps : 0) < (b.success > 0 ? b.qps : 0); 
                                     });
        if (best != results.end() && best->success > 0)
            BLUE_LOG_INFO(g_logger) << "Best QPS: " << best->name << " = " << best->qps;

        auto lowest = std::min_element(results.begin(), results.end(),
                                       [](auto &a, auto &b) { 
                                           double lat_a = a.success > 0 ? a.avg_latency_ms : 999999;
                                           double lat_b = b.success > 0 ? b.avg_latency_ms : 999999;
                                           return lat_a < lat_b; 
                                       });
        if (lowest != results.end() && lowest->success > 0)
            BLUE_LOG_INFO(g_logger) << "Lowest Latency: " << lowest->name 
                                    << " = " << lowest->avg_latency_ms << " ms";
    }

    export_report(results, "benchmark_report.csv");
    return 0;
}
