/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include "blue/task.h"
#include "blue/io_manager.h"
#include "blue/await.h"
#include "blue/defer.h"

TEST(DeferBasicTest, RunsAtScopeExit)
{
    std::atomic<bool> tem{false};
    {
        defer(tem.store(true));
        EXPECT_FALSE(tem.load());
    }
    EXPECT_TRUE(tem.load());
}

TEST(DeferBasicTest, MultiCounterTask)
{
    std::atomic<int> counter{0};
    {
        defer(counter.fetch_add(1));
        defer(counter.fetch_add(1));
        defer(counter.fetch_add(1));
        EXPECT_EQ(counter.load(), 0);
    }
    EXPECT_EQ(counter.load(), 3);
}

TEST(DeferOrderTest, ListOrder)
{
    std::vector<int> tem;
    {
        defer(tem.push_back(1));
        defer(tem.push_back(2));
        defer(tem.push_back(3));
    }
    EXPECT_EQ(tem, (std::vector<int>{3, 2, 1}));
}

TEST(DeferOrderTest, StringOrder)
{
    std::string tem = "";
    {
        defer(tem += "a");
        defer(tem += "b");
        defer(tem += "c");
    }
    EXPECT_EQ(tem, "cba");
}

TEST(DeferExceptionTest, RunOnException)
{
    std::atomic<bool> tem{false};
    {
        try
        {
            defer(tem.store(true));
            throw std::runtime_error("error");
        }
        catch (...)
        {
        }
    }
    EXPECT_TRUE(tem.load());
}

TEST(DeferExceptionTest, RunsOnStdException)
{
    std::atomic<int> counter{0};

    EXPECT_THROW({
        defer(counter.fetch_add(1));
        throw std::invalid_argument("test"); }, std::invalid_argument);

    EXPECT_EQ(counter.load(), 1);
}

TEST(DeferExceptionTest, SwallowsExceptionInDefer)
{
    std::atomic<bool> second_called{false};

    {
        defer(throw std::runtime_error("from defer"));
        defer(second_called.store(true));
    }

    // 第二个 defer 应该仍然执行(即使第一个抛了)
    EXPECT_TRUE(second_called.load());
}

TEST(DeferControlFlowTest, RunOnEarlyReturn)
{
    std::atomic<bool> called{false};

    auto func = [&](bool early)
    {
        defer(called.store(true));
        if (early)
        {
            return;
        }
    };

    func(true);
    EXPECT_TRUE(called.load());

    func(false);
    EXPECT_TRUE(called.load());
}

TEST(DeferControlFlowTest, RunOnGoto)
{
    std::atomic<bool> tem{false};

    {
        defer(tem.store(true));
        goto end;
    }
end:
    EXPECT_TRUE(tem.load());
}

TEST(DeferCaptureTest, CaptureByReference)
{
    int x = 0;
    {
        defer(x = 10);
    }
    EXPECT_EQ(x, 10);
}

TEST(DeferCaptureTest, SeensUpdatedCapture)
{
    int x = 0;
    {
        defer(x += 5);
        x = 10;
        EXPECT_EQ(x, 10);
    }
    EXPECT_EQ(x, 15);
}

// 清理资源
TEST(DeferResourceTest, ClosedFds)
{
    int fd = ::open("/dev/null", O_RDONLY);
    ASSERT_GT(fd, 0);

    {
        defer(::close(fd));
        char buf[1];
        EXPECT_EQ(::read(fd, buf, 1), 0);
    }

    // 已经被关闭
    EXPECT_EQ(::close(fd), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(DeferREsourceTest, DeletePointer)
{
    auto *p = new int(42);

    {
        defer(delete p);
        EXPECT_EQ(*p, 42);
    }
    SUCCEED();
}

TEST(DeferResourceTest, FreesWithMultiStatement)
{
    auto *buf = std::malloc(100);
    ASSERT_NE(buf, nullptr);

    {
        defer({
            std::free(buf);
            buf = nullptr;
        });
        std::memset(buf, 0, 100);
    }

    EXPECT_EQ(buf, nullptr);
}

TEST(DeferNestedTest, InnerBeforeOuter)
{
    std::vector<int> order;

    {
        defer(order.push_back(1)); // 外层,后执行

        {
            defer(order.push_back(2)); // 内层,先执行
            defer(order.push_back(3));
        }

        order.push_back(100); // 内层已执行完
    }

    EXPECT_EQ(order, (std::vector<int>{3, 2, 100, 1}));
}

TEST(DeferMoveTest, MovedDeferStillRuns)
{
    std::atomic<bool> called{false};

    {
        blue::Defer d1([&]
                       { called.store(true); });
        blue::Defer d2(std::move(d1));
        // d1 被移空,不应执行
        // d2 拥有函数,应执行
    }

    EXPECT_TRUE(called.load());
}

blue::Task<void> coroutine_with_defer(std::shared_ptr<std::atomic<bool>> flag, int ms)
{
    defer(flag->store(true));

    co_await blue::sleepForMs(ms);

    EXPECT_FALSE(flag->load()) << "defer 在挂起期间不应触发";

    co_return; // ← 触发 defer
}

TEST(DeferCoroutineTest, RunWhenCoroutineCompletes)
{
    auto iom = std::make_unique<blue::IOManager>(2);
    auto flag = std::make_shared<std::atomic<bool>>(false);

    iom->schedule(coroutine_with_defer(flag, 10));
    iom->wait_all();

    EXPECT_TRUE(flag->load());

    iom.reset();
}

blue::Task<void> child(std::shared_ptr<std::atomic<bool>> f)
{
    defer(f->store(true));
    co_await blue::sleepForMs(5);
    co_return;
}

blue::Task<void> parent(std::shared_ptr<std::atomic<bool>> p, std::shared_ptr<std::atomic<bool>> f)
{
    defer(p->store(true));
    co_await child(f);
    co_return;
}

TEST(DeferCoroutineTest, RunsOnNestedCoroutine)
{
    auto iom = std::make_unique<blue::IOManager>(2);

    auto parent_flag = std::make_shared<std::atomic<bool>>(false);
    auto child_flag = std::make_shared<std::atomic<bool>>(false);

    iom->schedule(parent(parent_flag, child_flag));

    iom->wait_all();

    EXPECT_TRUE(parent_flag->load());
    EXPECT_TRUE(child_flag->load());

    iom.reset();
}

blue::Task<void> exception_tem(std::shared_ptr<std::atomic<bool>> f, std::shared_ptr<std::exception_ptr> ep)
{
    try
    {
        defer(f->store(true));
        co_await blue::sleepForMs(5);
        throw std::runtime_error("boom");
    }
    catch (...)
    {
        *ep = std::current_exception();
    }
    co_return;
}

TEST(DeferCoroutineTest, RunsOnExceptionInCoroutine)
{
    auto iom = std::make_unique<blue::IOManager>(2);
    auto flag = std::make_shared<std::atomic<bool>>(false);
    auto ep = std::make_shared<std::exception_ptr>(nullptr);

    iom->schedule(exception_tem(flag, ep));

    iom->wait_all();

    EXPECT_TRUE(flag->load()) << "协程异常时 defer 也应执行";
    ASSERT_NE(*ep, nullptr);
    EXPECT_THROW(std::rethrow_exception(*ep), std::runtime_error);

    iom.reset();
}

blue::Task<void> order_tem(std::shared_ptr<std::vector<int>> order)
{
    defer(order->push_back(1));
    defer(order->push_back(2));
    defer(order->push_back(3));
    co_await blue::sleepForMs(5);
    co_return;
}

TEST(DeferCoroutineTest, LifoInCoroutine)
{
    auto iom = std::make_unique<blue::IOManager>(2);
    auto order = std::make_shared<std::vector<int>>();

    iom->schedule(order_tem(order));

    iom->wait_all();

    EXPECT_EQ(*order, (std::vector<int>{3, 2, 1}));

    iom.reset();
}
