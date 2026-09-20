/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>

#include "blue/io_manager.h"
#include "blue/await.h"
#include "blue/asyncio.h"
#include "blue/task.h"
#include "blue/defer.h"

namespace
{
    void setNonBlocking(int fd)
    {
        int flags = ::fcntl(fd, F_GETFL, 0);
        ASSERT_GE(flags, 0);
        ASSERT_EQ(::fcntl(fd, F_SETFL, flags | O_NONBLOCK), 0);
    }

    struct SocketPair
    {
        int fds[2] = {-1, -1};
        SocketPair()
        {
            EXPECT_EQ(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds), 0);
        }

        ~SocketPair()
        {
            if (fds[0] >= 0)
            {
                ::close(fds[0]);
            }
            if (fds[1] >= 0)
            {
                ::close(fds[1]);
            }
        }

        SocketPair(const SocketPair &) = delete;
        SocketPair &operator=(const SocketPair &) = delete;
    };
}

class AsyncIo : public ::testing::Test
{
protected:
    void SetUp() override
    {
        iom = std::make_unique<blue::IOManager>(2);
    }
    void TearDown() override
    {
        iom.reset();
    }
    std::unique_ptr<blue::IOManager> iom;
};

// 立即完成的Read
TEST_F(AsyncIo, ReadImmediate)
{
    SocketPair sp;
    const char *msg = "hello";
    ASSERT_EQ(::write(sp.fds[1], msg, 5), 5);

    auto task = [](int fd) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::Read(fd, buf, sizeof(buf));
        EXPECT_EQ(n, 5);
        EXPECT_STREQ(buf, "hello");
        co_return;
    };

    iom->schedule(task(sp.fds[0]));
    iom->wait_all();
}

// 异步: read挂起后写入触发恢复
TEST_F(AsyncIo, ReadSuspendThenResume)
{
    SocketPair sp;
    auto done = std::make_shared<std::atomic<bool>>(false);
    auto read_ok = std::make_shared<std::atomic<bool>>(false);

    auto task = [](int fd, std::shared_ptr<std::atomic<bool>> done,
                   std::shared_ptr<std::atomic<bool>> read_ok) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::Read(fd, buf, sizeof(buf));
        if (n == 5 && std::strcmp(buf, "world") == 0)
        {
            read_ok->store(true);
        }
        done->store(true);
        co_return;
    };

    iom->schedule(task(sp.fds[0], done, read_ok));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(done->load());

    ASSERT_EQ(::write(sp.fds[1], "world", 5), 5);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!done->load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(done->load());
    EXPECT_TRUE(read_ok->load());

    iom->wait_all();
}

TEST_F(AsyncIo, ReadTimeout)
{
    SocketPair sp;

    auto done = std::make_shared<std::atomic<bool>>(false);
    auto timeout = std::make_shared<std::atomic<bool>>(false);

    auto task = [](int fd, std::shared_ptr<std::atomic<bool>> done,
                   std::shared_ptr<std::atomic<bool>> timeout) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::ReadT(fd, buf, sizeof(buf), 50);
        if (n == -1 && errno == ETIMEDOUT)
        {
            timeout->store(true);
        }
        done->store(true);
        co_return;
    };

    iom->schedule(task(sp.fds[0], done, timeout));

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!done->load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    iom->wait_all();
    EXPECT_TRUE(done->load());
    EXPECT_TRUE(timeout->load()) << "ReadT 应该超时";

}

// read在超时前完场
TEST_F(AsyncIo, ReadCompleteBeforeTimeout)
{
    SocketPair sp;

    auto done = std::make_shared<std::atomic<bool>>(false);
    auto ok = std::make_shared<std::atomic<bool>>(false);

    auto task = [](int fd, std::shared_ptr<std::atomic<bool>> done,
                   std::shared_ptr<std::atomic<bool>> ok) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::ReadT(fd, buf, sizeof(buf), 500);
        if (n == 3 && std::strcmp(buf, "abc") == 0)
        {
            ok->store(true);
        }
        done->store(true);
        co_return;
    };

    iom->schedule(task(sp.fds[0], done, ok));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(::write(sp.fds[1], "abc", 3), 3);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!done->load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    iom->wait_all();
    EXPECT_TRUE(done->load());
    EXPECT_TRUE(ok->load());

}

// 取消：挂起后cancelEvent恢复
TEST_F(AsyncIo, CancelResumeCoroutine)
{
    SocketPair sp;
    auto done = std::make_shared<std::atomic<bool>>(false);

    auto task = [](int fd, std::shared_ptr<std::atomic<bool>> done) -> blue::Task<void>
    {
        char buf[16] = {0};
        (void)co_await blue::Read(fd, buf, sizeof(buf));
        done->store(true);
        co_return;
    };

    iom->schedule(task(sp.fds[0], done));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(done->load());

    // 取消成功后会触发task协程被恢复
    EXPECT_TRUE(iom->cancelEvent(sp.fds[0], blue::IOManager::Event::READ));

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!done->load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    iom->wait_all();
    EXPECT_TRUE(done->load());

}

// 写 write 挂起后读数据触发恢复
TEST_F(AsyncIo, WriteSuspendThenResume)
{
    SocketPair sp;

    // 把sp的写缓冲区填满
    std::vector<char> big(1 << 20, 'x');
    ssize_t total = 0;
    while (total < static_cast<ssize_t>(big.size()))
    {
        ssize_t n = ::write(sp.fds[1], big.data() + total, big.size() - total);
        if (n < 0)
            break;
        total += n;
    }

    auto done = std::make_shared<std::atomic<bool>>(false);
    auto written_ok = std::make_shared<std::atomic<bool>>(false);

    auto task = [](int fd, std::shared_ptr<std::atomic<bool>> done,
                   std::shared_ptr<std::atomic<bool>> writeen_ok) -> blue::Task<void>
    {
        const char *msg = "hi";
        ssize_t n = co_await blue::Write(fd, msg, 2);
        if (n == 2)
        {
            writeen_ok->store(true);
        }
        done->store(true);
        co_return;
    };

    iom->schedule(task(sp.fds[1], done, written_ok));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(done->load()) << "写应该挂起";

    // 读走数据让写缓冲区有数据
    std::vector<char> drain(1 << 20);
    while (::read(sp.fds[0], drain.data(), drain.size()) > 0)
    {
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!done->load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    iom->wait_all();
    EXPECT_TRUE(done->load());
    EXPECT_TRUE(written_ok->load());

}

// 子协程嵌套co_await Read
namespace
{
    blue::Task<ssize_t> read_helper(int fd, void *buf, size_t len)
    {
        ssize_t n = co_await blue::Read(fd, buf, len);
        co_return n;
    }

    blue::Task<void> outer_read(int fd, std::shared_ptr<std::atomic<bool>> ok)
    {
        char buf[16] = {0};
        ssize_t n = co_await read_helper(fd, buf, sizeof(buf));
        if (n == 5 && std::strcmp(buf, "nest!") == 0)
        {
            ok->store(true);
        }
        co_return;
    }
}

TEST_F(AsyncIo, NestedCoroutineRead)
{
    SocketPair sp;

    auto ok = std::make_shared<std::atomic<bool>>(false);
    iom->schedule(outer_read(sp.fds[0], ok));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(::write(sp.fds[1], "nest!", 5), 5);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!ok->load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    iom->wait_all();
    EXPECT_TRUE(ok->load());

}

TEST_F(AsyncIo, ReadInvalidFd)
{
    auto done = std::make_shared<std::atomic<bool>>(false);
    auto err_ok = std::make_shared<std::atomic<bool>>(false);

    auto task = [](std::shared_ptr<std::atomic<bool>> done,
                   std::shared_ptr<std::atomic<bool>> err_ok) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::Read(-1, buf, sizeof(buf));
        if (n == -1)
        {
            EXPECT_EQ(errno, EBADF);
            err_ok->store(true);
        }
        done->store(true);
        co_return;
    };

    iom->schedule(task(done, err_ok));
    iom->wait_all();
    EXPECT_TRUE(done->load());
    EXPECT_TRUE(err_ok->load());
}

TEST_F(AsyncIo, ManyConcurrentReads)
{
    constexpr int N = 100;

    std::vector<std::unique_ptr<SocketPair>> pairs;
    pairs.reserve(N);
    for (int i = 0; i < N; i++)
    {
        pairs.push_back(std::make_unique<SocketPair>());
    }

    auto counter = std::make_shared<std::atomic<int>>(0);

    auto task = [](int fd, std::shared_ptr<std::atomic<int>> counter) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::Read(fd, buf, sizeof(buf));
        if (n == 1 && buf[0] == 'x')
        {
            counter->fetch_add(1);
        }
        co_return;
    };

    for (int i = 0; i < N; i++)
    {
        iom->schedule(task(pairs[i]->fds[0], counter));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    for (int i = 0; i < N; i++)
    {
        ASSERT_EQ(::write(pairs[i]->fds[1], "x", 1), 1);
    }
    iom->wait_all();
    EXPECT_EQ(counter->load(), N);
}

TEST_F(AsyncIo, ManyConcurrentTimeouts)
{
    constexpr int N = 50;

    std::vector<std::unique_ptr<SocketPair>> pairs;
    pairs.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        pairs.push_back(std::make_unique<SocketPair>());
    }

    auto counter = std::make_shared<std::atomic<int>>(0);

    auto task = [](int fd, std::shared_ptr<std::atomic<int>> counter) -> blue::Task<void>
    {
        char buf[16] = {0};
        ssize_t n = co_await blue::ReadT(fd, buf, sizeof(buf), 10);
        if (n == -1 && errno == ETIMEDOUT)
        {
            counter->fetch_add(1);
        }
        co_return;
    };

    for (int i = 0; i < N; ++i)
    {
        iom->schedule(task(pairs[i]->fds[0], counter));
    }

    // 不写入，等所有超时
    iom->wait_all();
    EXPECT_EQ(counter->load(), N);
}
