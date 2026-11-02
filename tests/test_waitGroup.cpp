#include <gtest/gtest.h>
#include <atomic>
#include "src/core/wait_group.h"
#include "src/core/io_manager.h"
#include "src/core/await.h"

namespace
{
    blue::Task<void> delayed_increment(std::shared_ptr<std::atomic<int>> counter,
                                       std::shared_ptr<blue::WaitGroup> wg,
                                       int ms)
    {
        co_await blue::sleepForMs(ms);
        counter->fetch_add(1);
        wg->Done();
        co_return;
    }
}

TEST(WaitGroupTest, WaitAllComplete)
{
    blue::IOManager iom(2);
    auto counter = std::make_shared<std::atomic<int>>(0);
    auto wg = std::make_shared<blue::WaitGroup>();

    constexpr int N = 10;
    wg->Add(N);
    for (int i = 0; i < N; ++i)
    {
        iom.schedule(delayed_increment(counter, wg, 10 + i));
    }

    iom.schedule([wg, counter, N]() -> blue::Task<void> {
        co_await wg->Wait();
        EXPECT_EQ(counter->load(), N);
        co_return;
    }());

    iom.wait_all();
}

TEST(WaitGroupTest, WaitImmediate)
{
    blue::IOManager iom(2);
    auto wg = std::make_shared<blue::WaitGroup>();
    wg->Add(1);
    wg->Done();   // 计数归零

    iom.schedule([wg]() -> blue::Task<void> {
        co_await wg->Wait();   // 立即完成
        co_return;
    }());

    iom.wait_all();
}

TEST(WaitGroupTest, WaitWithAddDuringWait)
{
    blue::IOManager iom(2);
    auto wg = std::make_shared<blue::WaitGroup>();
    auto counter = std::make_shared<std::atomic<int>>(0);

    wg->Add(1);
    iom.schedule(delayed_increment(counter, wg, 50));

    iom.schedule([wg, &iom, counter]() -> blue::Task<void> {
        co_await wg->Wait();
        EXPECT_EQ(counter->load(), 1);
        co_return;
    }());

    iom.wait_all();
}
