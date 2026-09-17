/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
// tests/test_iomanager.cpp
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include "blue/io_manager.h"
#include "blue/await.h"

namespace
{

    constexpr auto kLongWait = std::chrono::seconds(5);

    template <typename Pred>
    bool waitUntil(Pred &&pred, std::chrono::milliseconds timeout)
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (pred())
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return pred();
    }

    blue::Task<void> sleep_then_set(std::shared_ptr<std::atomic<bool>> flag, int ms)
    {
        co_await blue::sleepForMs(ms);
        flag->store(true);
        co_return;
    }

    blue::Task<void> increment(std::shared_ptr<std::atomic<int>> counter)
    {
        counter->fetch_add(1);
        co_return;
    }

    blue::Task<void> sleep_then_increment(std::shared_ptr<std::atomic<int>> counter, int ms)
    {
        co_await blue::sleepForMs(ms);
        counter->fetch_add(1);
        co_return;
    }

    blue::Task<void> set_flag(std::shared_ptr<std::atomic<bool>> flag)
    {
        flag->store(true);
        co_return;
    }

    blue::Task<void> wait_fd_readable(blue::IOManager *iom, int fd,
                                      std::shared_ptr<std::atomic<bool>> done)
    {
        // 这里用一个简单的 awaiter:注册 fd,挂起
        struct FdAwaiter
        {
            blue::IOManager *iom;
            int fd;
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) const
            {
                iom->addEvent(fd, blue::IOManager::Event::READ, h);
            }
            void await_resume() const noexcept {}
        };

        co_await FdAwaiter{iom, fd};
        done->store(true);
        co_return;
    }

    blue::Task<void> wait_fd_counter(blue::IOManager *iom, int fd, std::shared_ptr<std::atomic<int>> counter)
    {
        // 这里用一个简单的 awaiter:注册 fd,挂起
        struct FdAwaiter
        {
            blue::IOManager *iom;
            int fd;
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) const
            {
                iom->addEvent(fd, blue::IOManager::Event::READ, h);
            }
            void await_resume() const noexcept {}
        };

        co_await FdAwaiter{iom, fd};
        counter->fetch_add(1);
        co_return;
    }

} // namespace

class IOTimerTest : public ::testing::Test
{
protected:
    void SetUp() override { iom_ = std::make_unique<blue::IOManager>(2); }
    void TearDown() override
    {
        iom_.reset();
    }
    std::unique_ptr<blue::IOManager> iom_;
};

TEST_F(IOTimerTest, SleepCompletes)
{
    auto done = std::make_shared<std::atomic<bool>>(false);
    iom_->schedule(sleep_then_set(done, 10));
    iom_->wait_all();
    EXPECT_TRUE(done->load());
}

TEST_F(IOTimerTest, SleepAtLeastRequestedTime)
{
    auto done = std::make_shared<std::atomic<bool>>(false);
    auto start = std::chrono::steady_clock::now();
    iom_->schedule(sleep_then_set(done, 30));
    iom_->wait_all();
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_TRUE(done->load());
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
              30);
}

TEST_F(IOTimerTest, ManySleepsConcurrently)
{
    constexpr int N = 20;
    constexpr int kMs = 50;

    auto counter = std::make_shared<std::atomic<int>>(0);
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < N; ++i)
    {
        iom_->schedule(sleep_then_increment(counter, kMs));
    }
    iom_->wait_all();

    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_EQ(counter->load(), N);
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
              N * kMs / 2)
        << "任务可能被串行执行";
}

TEST_F(IOTimerTest, ManyTasksAcrossThreads)
{
    constexpr int N = 200;
    auto counter = std::make_shared<std::atomic<int>>(0);

    for (int i = 0; i < N; ++i)
    {
        iom_->schedule(sleep_then_increment(counter, 0)); // 立即完成
    }
    iom_->wait_all();
    EXPECT_EQ(counter->load(), N);
}

TEST_F(IOTimerTest, WaitAllWaitsForSlowest)
{
    auto fast = std::make_shared<std::atomic<bool>>(false);
    auto slow = std::make_shared<std::atomic<bool>>(false);

    iom_->schedule(set_flag(fast));
    iom_->schedule(sleep_then_set(slow, 50));

    iom_->wait_all();
    EXPECT_TRUE(fast->load());
    EXPECT_TRUE(slow->load()) << "wait_all 提前返回了";
}

TEST_F(IOTimerTest, WaitAllOnIdleReturnsImmediately)
{
    auto start = std::chrono::steady_clock::now();
    iom_->wait_all();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - start)
                          .count();
    EXPECT_LT(elapsed_ms, 100);
}

// 测试add/del/cancel/Event

class IOFdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        iom_ = std::make_unique<blue::IOManager>(2);
        efd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        ASSERT_GT(efd_, 0) << "eventfd 创建失败";
    }

    void TearDown() override
    {
        if (efd_ >= 0)
        {
            ::close(efd_);
            efd_ = -1;
        }
        iom_.reset();
    }

    // 测试用的空 cb
    static void noop()
    {
        return;
    }

    void writeEventfd(uint64_t v = 1)
    {
        ssize_t n = ::write(efd_, &v, sizeof(v));
        ASSERT_EQ(n, static_cast<ssize_t>(sizeof(v)));
    }

    std::unique_ptr<blue::IOManager> iom_;
    int efd_ = -1;
};

// ------------------------------------------------------------
// 2.1 基本的添加和删除
// ------------------------------------------------------------
TEST_F(IOFdTest, AddReadEventSucceeds)
{
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
}

TEST_F(IOFdTest, AddWriteEventSucceeds)
{
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::WRITE,
                             nullptr, noop),
              0);
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::WRITE));
}

TEST_F(IOFdTest, AddReadAndWriteEvents)
{
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::WRITE,
                             nullptr, noop),
              0);
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::WRITE));
}

// ------------------------------------------------------------
// 2.2 重复注册应失败
// ------------------------------------------------------------
TEST_F(IOFdTest, AddSameEventTwiceFails)
{
    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              -1)
        << "重复注册同一事件应返回 -1";
    iom_->delEvent(efd_, blue::IOManager::Event::READ);
}

// ------------------------------------------------------------
// 2.3 删除未注册的事件
// ------------------------------------------------------------
TEST_F(IOFdTest, DelUnregisteredEventFails)
{
    EXPECT_FALSE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
}

TEST_F(IOFdTest, DelWrongEventTypeFails)
{
    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_FALSE(iom_->delEvent(efd_, blue::IOManager::Event::WRITE));
    iom_->delEvent(efd_, blue::IOManager::Event::READ);
}

// ------------------------------------------------------------
// 2.4 非法 fd
// ------------------------------------------------------------
TEST_F(IOFdTest, AddInvalidFdFails)
{
    EXPECT_EQ(iom_->addEvent(-1, blue::IOManager::Event::READ,
                             nullptr, noop),
              -1);
}

TEST_F(IOFdTest, DelInvalidFdFails)
{
    EXPECT_FALSE(iom_->delEvent(-1, blue::IOManager::Event::READ));
}

TEST_F(IOFdTest, CancelInvalidFdFails)
{
    EXPECT_FALSE(iom_->cancelEvent(-1, blue::IOManager::Event::READ));
    EXPECT_FALSE(iom_->cancelAll(-1));
}

// ------------------------------------------------------------
// 2.5 重复删除
// ------------------------------------------------------------
TEST_F(IOFdTest, DelTwiceFailsSecond)
{
    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
    EXPECT_FALSE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
}

// ------------------------------------------------------------
// 2.6 add -> del -> add 复用
// ------------------------------------------------------------
TEST_F(IOFdTest, ReAddAfterDel)
{
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, noop),
              0);
    EXPECT_TRUE(iom_->delEvent(efd_, blue::IOManager::Event::READ));
}

// ------------------------------------------------------------
// 2.7 无 cb / 无 handle 应被拒绝
// ------------------------------------------------------------
TEST_F(IOFdTest, AddEventWithoutHandlerFails)
{
    EXPECT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, nullptr),
              -1);
}

// 测试fd 就绪触发cb或handle的回调

// ------------------------------------------------------------
// 3.1 写 eventfd,触发 cb
// ------------------------------------------------------------
TEST_F(IOFdTest, WriteTriggersCallback)
{
    auto called = std::make_shared<std::atomic<int>>(0);

    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr,
                             [called]
                             { called->fetch_add(1); }),
              0);

    writeEventfd();

    // cb 在调度器线程执行,等它完成
    EXPECT_TRUE(waitUntil([&]
                          { return called->load() > 0; },
                          std::chrono::seconds(2)))
        << "cb 没被调用";
    EXPECT_EQ(called->load(), 1);
}

// ------------------------------------------------------------
// 3.2 触发一次后 cb 不重复调用
// ------------------------------------------------------------
TEST_F(IOFdTest, CallbackCalledOnce)
{
    auto count = std::make_shared<std::atomic<int>>(0);

    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr,
                             [count]
                             { count->fetch_add(1); }),
              0);

    writeEventfd();

    EXPECT_TRUE(waitUntil([&]
                          { return count->load() > 0; },
                          std::chrono::seconds(2)));
    // 再等一会儿确认没有重复调用
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(count->load(), 1) << "cb 被调用了多次";
}

// ------------------------------------------------------------
// 3.3 cancelEvent 触发 cb
// ------------------------------------------------------------
TEST_F(IOFdTest, CancelTriggersCallback)
{
    auto called = std::make_shared<std::atomic<bool>>(false);

    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr,
                             [called]
                             { called->store(true); }),
              0);

    EXPECT_TRUE(iom_->cancelEvent(efd_, blue::IOManager::Event::READ));

    EXPECT_TRUE(waitUntil([&]
                          { return called->load(); },
                          std::chrono::seconds(2)))
        << "cancel 后 cb 没被调用";
}

// ------------------------------------------------------------
// 3.4 cancelAll 同时触发 read / write 的 cb
// ------------------------------------------------------------
TEST_F(IOFdTest, CancelAllTriggersAllCallbacks)
{
    auto read_called = std::make_shared<std::atomic<bool>>(false);
    auto write_called = std::make_shared<std::atomic<bool>>(false);

    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr,
                             [read_called]
                             { read_called->store(true); }),
              0);
    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::WRITE,
                             nullptr,
                             [write_called]
                             { write_called->store(true); }),
              0);

    EXPECT_TRUE(iom_->cancelAll(efd_));

    EXPECT_TRUE(waitUntil([&]
                          { return read_called->load() && write_called->load(); },
                          std::chrono::seconds(2)))
        << "cancelAll 没触发所有 cb";
}

// ------------------------------------------------------------
// 3.5 多个 fd 同时触发
// ------------------------------------------------------------
TEST_F(IOFdTest, MultipleFdsTriggered)
{
    int efd2 = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT_GT(efd2, 0);
    // 用 RAII 保证 fd 关闭
    struct FdGuard
    {
        int fd;
        ~FdGuard()
        {
            if (fd >= 0)
                ::close(fd);
        }
    } guard{efd2};

    auto c1 = std::make_shared<std::atomic<int>>(0);
    auto c2 = std::make_shared<std::atomic<int>>(0);

    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, [c1]
                             { c1->fetch_add(1); }),
              0);
    ASSERT_EQ(iom_->addEvent(efd2, blue::IOManager::Event::READ,
                             nullptr, [c2]
                             { c2->fetch_add(1); }),
              0);

    uint64_t x = 1;
    // 两个 fd 都写
    ASSERT_EQ(::write(efd_, &x, sizeof(x)), 8);
    ASSERT_EQ(::write(efd2, &x, sizeof(x)), 8);

    EXPECT_TRUE(waitUntil([&]
                          { return c1->load() > 0 && c2->load() > 0; },
                          std::chrono::seconds(2)));
    EXPECT_EQ(c1->load(), 1);
    EXPECT_EQ(c2->load(), 1);
}

// ------------------------------------------------------------
// 3.6 触发后 epoll 状态被清理(再次写仍会触发)
// ------------------------------------------------------------
TEST_F(IOFdTest, CanTriggerMultipleTimesAfterReAdd)
{
    auto count = std::make_shared<std::atomic<int>>(0);

    // 触发一次
    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, [count]
                             { count->fetch_add(1); }),
              0);
    writeEventfd();
    EXPECT_TRUE(waitUntil([&]
                          { return count->load() == 1; },
                          std::chrono::seconds(2)));

    // 再注册一次,再触发
    ASSERT_EQ(iom_->addEvent(efd_, blue::IOManager::Event::READ,
                             nullptr, [count]
                             { count->fetch_add(1); }),
              0);
    // 消费 eventfd 里的数据
    uint64_t v;
    ::read(efd_, &v, sizeof(v));
    writeEventfd();
    EXPECT_TRUE(waitUntil([&]
                          { return count->load() == 2; },
                          std::chrono::seconds(2)));
}

// 加上协程测试

TEST_F(IOFdTest, FdTriggersCoroutine)
{
    auto done = std::make_shared<std::atomic<bool>>(false);

    // 用一个 getThis 拿到 iom 指针(测试线程不在调度器里)
    iom_->schedule(wait_fd_readable(iom_.get(), efd_, done));

    // 给协程一点时间注册 fd
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_FALSE(done->load()); // 协程挂起但是未完成

    // 写 fd 触发
    writeEventfd();

    EXPECT_TRUE(waitUntil([&]
                          { return done->load(); },
                          std::chrono::seconds(2)))
        << "协程没被恢复";

    // 等所有任务完成,防止 TearDown 前还有残留
    iom_->wait_all();
}

TEST_F(IOFdTest, CancelEventResumesCoroutine)
{
    auto done = std::make_shared<std::atomic<bool>>(false);

    iom_->schedule(wait_fd_readable(iom_.get(), efd_, done));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 主动取消,应触发协程恢复
    EXPECT_TRUE(iom_->cancelEvent(efd_, blue::IOManager::Event::READ));

    EXPECT_TRUE(waitUntil([&]
                          { return done->load(); },
                          std::chrono::seconds(2)));

    iom_->wait_all();
}

TEST_F(IOFdTest, MultipleCoroutinesWaitDifferentFds)
{
    int efd2 = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT_GT(efd2, 0);

    struct FdGuard
    {
        int fd;
        ~FdGuard()
        {
            if (fd >= 0)
                ::close(fd);
        }
    } guard{efd2};

    auto done1 = std::make_shared<std::atomic<bool>>(false);
    auto done2 = std::make_shared<std::atomic<bool>>(false);

    iom_->schedule(wait_fd_readable(iom_.get(), efd_, done1));
    iom_->schedule(wait_fd_readable(iom_.get(), efd2, done2));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    uint64_t x = 1;
    // 只触发第二个
    ASSERT_EQ(::write(efd2, &x, sizeof(x)), 8);

    EXPECT_TRUE(waitUntil([&]
                          { return done2->load(); },
                          std::chrono::seconds(2)));
    EXPECT_FALSE(done1->load());

    // 触发第一个
    writeEventfd();
    EXPECT_TRUE(waitUntil([&]
                          { return done1->load(); },
                          std::chrono::seconds(2)));

    iom_->wait_all();
}

class IOConcurrencyTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        iom_ = std::make_unique<blue::IOManager>(2);
    }
    void TearDown() override
    {
        iom_.reset();
    }

    std::unique_ptr<blue::IOManager> iom_;
};

TEST_F(IOConcurrencyTest, ManyImmediateTask)
{
    constexpr int N = 1000;
    auto counter = std::make_shared<std::atomic<int>>(0);
    for (int i = 0; i < N; i++)
    {
        iom_->schedule(increment(counter));
    }
    iom_->wait_all();

    EXPECT_EQ(counter->load(), N);
}

TEST_F(IOConcurrencyTest, ManyTimeTask)
{
    constexpr int N = 200;
    auto counter = std::make_shared<std::atomic<int>>(0);
    for (int i = 0; i < N; i++)
    {
        iom_->schedule(sleep_then_increment(counter, 5));
    }
    iom_->wait_all();
    EXPECT_EQ(counter->load(), N);
}

TEST_F(IOConcurrencyTest, ManyTriggerFdsCorroutines)
{
    constexpr int N = 50;
    std::vector<int> fds;
    std::vector<std::shared_ptr<std::atomic<bool>>> dones;

    for (int i = 0; i < N; i++)
    {
        int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        ASSERT_GT(fd, 0);
        fds.push_back(fd);
        dones.push_back(std::make_shared<std::atomic<bool>>(false));
    }

    struct FdCleaner
    {
        std::vector<int> &fds;
        ~FdCleaner()
        {
            for (int x : fds)
            {
                if (x >= 0)
                {
                    ::close(x);
                }
            }
        }
    } cleaner{fds};

    // 启动协程
    for (int i = 0; i < N; i++)
    {
        iom_->schedule(wait_fd_readable(iom_.get(), fds[i], dones[i]));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // 触发
    uint64_t x = 1;
    for (int i = 0; i < N; ++i)
    {
        ASSERT_EQ(::write(fds[i], &x, sizeof(x)), 8);
    }

    // 等全部完成
    iom_->wait_all();

    EXPECT_TRUE(waitUntil([&]
                          {
        for (auto& d : dones) if (!d->load()) return false;
        return true; }, std::chrono::seconds(5)))
        << "有协程没被恢复";

    for (auto &d : dones)
    {
        EXPECT_TRUE(d->load());
    }
}

TEST_F(IOConcurrencyTest, MixedTimerAndFds)
{
    constexpr int N = 50;
    auto counter = std::make_shared<std::atomic<int>>(0);

    // 一半定时器
    for (int i = 0; i < N; i++)
    {
        iom_->schedule(sleep_then_increment(counter, 10 + i % 20));
    }

    // 一半fd
    std::vector<int> fds;
    for (int i = 0; i < N; i++)
    {
        int fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        ASSERT_GT(fd, 0);
        fds.push_back(fd);
        iom_->schedule(wait_fd_counter(iom_.get(), fd, counter));
    }

    struct FdCleaner
    {
        std::vector<int> &fds;
        ~FdCleaner()
        {
            for (int x : fds)
            {
                if (x >= 0)
                {
                    ::close(x);
                }
            }
        }
    } cleaner{fds};

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    uint64_t x = 1;
    for (int i = 0; i < N; i++)
    {
        ::write(fds[i], &x, sizeof(x));
    }

    iom_->wait_all();
    EXPECT_EQ(counter->load(), N * 2);
}

// 反复创建销毁fdmanager
TEST_F(IOConcurrencyTest, RepeatedCreateDestroy)
{
    constexpr int ROUNDS = 10;
    for (int r = 0; r < ROUNDS; ++r)
    {
        auto iom = std::make_unique<blue::IOManager>(2);
        auto counter = std::make_shared<std::atomic<int>>(0);

        for (int i = 0; i < 20; ++i)
        {
            iom->schedule(sleep_then_increment(counter,1));
        }
        iom->wait_all();

        EXPECT_EQ(counter->load(), 20) << "round " << r;
    }
}

TEST_F(IOConcurrencyTest, ManyWaitAllCycles)
{
    constexpr int CYCLES = 50;
    constexpr int PER_CYCLE = 20;

    auto counter = std::make_shared<std::atomic<int>>(0);

    for (int c = 0; c < CYCLES; ++c)
    {
        for (int i = 0; i < PER_CYCLE; ++i)
        {
            iom_->schedule(increment(counter));
        }
        iom_->wait_all();
    }
    
    EXPECT_EQ(counter->load(), CYCLES * PER_CYCLE);
}