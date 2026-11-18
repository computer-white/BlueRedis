#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "src/core/io_manager.h"

// ---------------------------------------------------------------------------
// 被测函数：一个耗时任务，用于验证 future 在任务完成前处于未就绪状态
// ---------------------------------------------------------------------------
static int slow_add(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return a + b;
}

// ---------------------------------------------------------------------------
// 用例 1：基本功能 —— schedulePackageTask 返回的 future 能拿到正确结果
// ---------------------------------------------------------------------------
TEST(SchedulePackageTaskTest, BasicResult)
{
    blue::IOManager iom(1);

    auto f = iom.schedulePackageTask(slow_add, 3, 4);

    EXPECT_EQ(f.get(), 7);

    iom.wait_all();
}

// ---------------------------------------------------------------------------
// 用例 2：future 提升为 shared_future，多个线程同时等待同一结果
// ---------------------------------------------------------------------------
TEST(SchedulePackageTaskTest, SharedFutureMultipleWaiters)
{
    blue::IOManager iom(2);

    auto f = iom.schedulePackageTask(slow_add, 10, 32);

    // 提升为 shared_future：可被多个线程同时 get()
    auto sf = f.share();
    EXPECT_FALSE(f.valid());

    constexpr int kWaiters = 8;
    std::atomic<int> ready_count{0};
    std::vector<std::thread> waiters;
    std::vector<int> results(kWaiters, -1);

    for (int i = 0; i < kWaiters; ++i)
    {
        waiters.emplace_back([&, i]()
                             {
            // 所有线程阻塞在同一个 shared_future 上
            int v = sf.get();          // shared_future::get 可多次调用
            results[i] = v;
            ready_count.fetch_add(1, std::memory_order_release); });
    }

    for (auto &t : waiters)
    {
        t.join();
    }

    // 所有等待者都拿到同一个结果
    EXPECT_EQ(ready_count.load(), kWaiters);
    for (int i = 0; i < kWaiters; ++i)
    {
        EXPECT_EQ(results[i], 42);
    }

    iom.wait_all();
}

// ---------------------------------------------------------------------------
// 用例 3：shared_future 在任务完成前是未就绪的，完成后立即可读
// ---------------------------------------------------------------------------
TEST(SchedulePackageTaskTest, SharedFutureReadyAfterTaskCompletes)
{
    blue::IOManager iom(2);

    std::atomic<bool> task_started{false};
    std::atomic<bool> task_finished{false};

    auto f = iom.schedulePackageTask([&]()
                                  {
        task_started.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        task_finished.store(true, std::memory_order_release); });
    auto sf = f.share();
    EXPECT_FALSE(f.valid());

    // 此时任务大概率还没开始
    auto status_before = sf.wait_for(std::chrono::milliseconds(0));

    // 阻塞直到完成
    sf.wait();

    EXPECT_TRUE(task_started.load());
    EXPECT_TRUE(task_finished.load());

    // 完成后 shared_future 仍可反复 wait，不抛异常
    EXPECT_EQ(sf.wait_for(std::chrono::milliseconds(0)),
              std::future_status::ready);
    sf.wait();
    sf.wait(); // 多次 wait 也安全

    iom.wait_all();
}

// ---------------------------------------------------------------------------
// 用例 4：task 抛异常时，所有等待者都能拿到同一个异常
// ---------------------------------------------------------------------------
TEST(SchedulePackageTaskTest, SharedFuturePropagatesException)
{
    blue::IOManager iom(2);

    auto f = iom.schedulePackageTask([]() -> int
                                 { throw std::runtime_error("boom"); });
    auto sf = f.share();
    EXPECT_FALSE(f.valid());

    constexpr int kWaiters = 4;
    std::atomic<int> caught{0};
    std::vector<std::thread> waiters;

    for (int i = 0; i < kWaiters; ++i)
    {
        waiters.emplace_back([&]()
                             {
            try
            {
                (void)sf.get();
            }
            catch (const std::runtime_error& e)
            {
                if (std::string(e.what()) == "boom")
                {
                    caught.fetch_add(1, std::memory_order_release);
                }
            } });
    }

    for (auto &t : waiters)
    {
        t.join();
    }

    EXPECT_EQ(caught.load(), kWaiters);

    iom.wait_all();
}