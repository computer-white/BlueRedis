/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "blue/io_manager.h"
#include "blue/await.h"

namespace
{
    blue::Task<int> add_async(int a, int b) { co_return a + b; }

    blue::Task<void> set_flag(bool *f)
    {
        *f = true;
        co_return;
    }

    blue::Task<int> inner_val() { co_return 42; }

    blue::Task<int> outer_add_one()
    {
        int v = co_await inner_val();
        co_return v + 1;
    }

    blue::Task<int> throwing()
    {
        throw std::runtime_error("boom");
        co_return 0;
    }
}

TEST(TaskBasic, ReturnValue)
{
    auto t = add_async(2, 3);
    ASSERT_TRUE(static_cast<bool>(t));
    EXPECT_FALSE(t.done());
    t.resume();
    ASSERT_TRUE(t.done());
    EXPECT_EQ(t.get(), 5);
}

TEST(TaskBasic, VoidTask)
{
    bool f = false;
    auto t = set_flag(&f);
    EXPECT_FALSE(f);
    t.resume();
    EXPECT_TRUE(t.done());
    EXPECT_TRUE(f);
}

TEST(TaskBasic, DefaultConstructed)
{
    blue::Task<int> t;
    EXPECT_FALSE(static_cast<bool>(t));
    EXPECT_TRUE(t.done());
}

// 嵌套
TEST(TaskNested, AwaitInner)
{
    auto t = outer_add_one();
    t.resume();
    ASSERT_TRUE(t.done());
    EXPECT_EQ(t.get(), 43);
}

TEST(TaskNested, SingleResumeDrivesToCompletion)
{
    auto t = outer_add_one();
    t.resume();
    EXPECT_TRUE(t.done()) << "对称转移应一次驱动到底";
}

// 异常
TEST(TaskException, GetRethrows)
{
    auto t = throwing();
    try
    {
        t.resume();
    }
    catch (...)
    {
    }
    ASSERT_TRUE(t.done());
    EXPECT_THROW(t.get(), std::runtime_error);
}

// 移动
TEST(TaskMove, MoveCtor)
{
    auto t1 = add_async(1, 2);
    auto t2 = std::move(t1);
    // 移动后t1的handle应该为nullptr
    EXPECT_FALSE(static_cast<bool>(t1));
    EXPECT_TRUE(static_cast<bool>(t2));
    t2.resume();
    EXPECT_EQ(t2.get(), 3);
}

TEST(TaskMove, AssignNullptr)
{
    auto t = add_async(1, 2);
    t = nullptr;
    EXPECT_FALSE(static_cast<bool>(t));
}

// ---------- 状态 ----------
TEST(TaskState, InitialNotDone)
{
    auto t = add_async(1, 2);
    EXPECT_FALSE(t.done());
}

// ---------- 压测 ----------
TEST(TaskStress, ManySequential)
{
    for (int i = 0; i < 10000; ++i)
    {
        auto t = add_async(i, i);
        t.resume();
        ASSERT_TRUE(t.done());
        ASSERT_EQ(t.get(), i * 2);
    }
}

// 让ASAN捕获到没有完成的协程泄漏
TEST(TaskLeak, UnfinishedTaskDestroyed) {
    {
        auto t = add_async(1, 2);
        // 故意不 resume
    }   // ← 析构,如果泄漏 ASAN 会报(已修复task.h内部逻辑)
    SUCCEED();
}

// ---------- IOManager 集成 ----------
class CoroutineIOTest : public ::testing::Test
{
protected:
    void SetUp() override { iom_ = std::make_unique<blue::IOManager>(4); }
    void TearDown() override
    {
        iom_->wait_all();
        iom_.reset();
    }
    std::unique_ptr<blue::IOManager> iom_;
};

TEST_F(CoroutineIOTest, SleepCompletes)
{
    std::atomic<bool> done{false};
    iom_->schedule([&]() -> blue::Task<void>
                   {
        co_await blue::sleepForMs(10);
        done = true;
        co_return; }());
    iom_->wait_all();
    EXPECT_TRUE(done.load());
}