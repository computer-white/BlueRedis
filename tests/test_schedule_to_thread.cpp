#include <gtest/gtest.h>
#include "src/core/io_manager.h"
#include "src/core/await.h"
#include "src/core/wait_group.h"

class ScheduleToThreadTest : public ::testing::Test
{
public:
    static int add(int id, int x, int y)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_EQ(id, blue::IOManager::GetThreadIndex());
        EXPECT_EQ(thread_count, blue::IOManager::GetThreadCount());
        return x + y;
    }

    blue::Task<void> worker(int id)
    {
        EXPECT_EQ(id, blue::IOManager::GetThreadIndex());
        co_await blue::sleepFor(1);
        EXPECT_EQ(id, blue::IOManager::GetThreadIndex());
        EXPECT_EQ(thread_count, blue::IOManager::GetThreadCount());
        co_return;
    }

    blue::Task<void> tem(int id)
    {
        EXPECT_EQ(id, blue::IOManager::GetThreadIndex());
        EXPECT_EQ(thread_count, blue::IOManager::GetThreadCount());
        co_return;
    }

    blue::Task<void> worker_with_waitGroup(int id, std::shared_ptr<blue::WaitGroup> wg)
    {
        co_await blue::sleepFor(1);
        EXPECT_EQ(id, blue::IOManager::GetThreadIndex());
        EXPECT_EQ(thread_count, blue::IOManager::GetThreadCount());
        wg->Done();
        co_return;
    }
protected:
    void TearDown() override
    {
        iom.reset();
        thread_count = 0;
    }
public:
    std::unique_ptr<blue::IOManager> iom;
    static int thread_count;
};

int ScheduleToThreadTest::thread_count = 0;

TEST_F(ScheduleToThreadTest, TestCallBack)
{
    thread_count = 2;
    iom = std::make_unique<blue::IOManager>(thread_count);
    for (int i = 0; i < thread_count; i++)
    {
        iom->scheduleCallBack(std::bind(&add, i, i + 1, i + 2), i);
    }
    iom->wait_all();
}

TEST_F(ScheduleToThreadTest, TestTask)
{
    thread_count = 4;
    iom = std::make_unique<blue::IOManager>(thread_count);
    for (int i = 0; i < thread_count; i++)
    {
        iom->schedule(worker(i), i);
    }
    iom->wait_all();
}

TEST_F(ScheduleToThreadTest, TestTaskAndWaitGroup)
{
    thread_count = 4;
    iom = std::make_unique<blue::IOManager>(thread_count);
    auto wg = std::make_shared<blue::WaitGroup>();
    wg->Add(thread_count);
    for (int i = 0; i < thread_count; i++)
    {
        iom->schedule(worker_with_waitGroup(i, wg), i);
    }
    wg->Wait();
    iom->wait_all();
}

TEST_F(ScheduleToThreadTest, TestTasksToThread)
{
    thread_count = 4;
    iom = std::make_unique<blue::IOManager>(thread_count);
    for (int i = 0; i < thread_count; i++)
    {
        iom->scheduleMulToThr(i, worker(i), tem(i));
    }
    iom->wait_all();
}

TEST_F(ScheduleToThreadTest, TestPackageTastToThread)
{
    thread_count = 4;
    iom = std::make_unique<blue::IOManager>(thread_count);
    std::vector<std::future<int>> fs;
    for (int i = 0; i < thread_count; i++)
    {
        auto f = iom->schedulePackageTaskToThread(add, i, i, i + 1, i + 2);
        fs.push_back(std::move(f));
    }
    for (size_t i = 0; i < fs.size(); i++)
    {
        EXPECT_EQ(fs[i].get(), i + 1 + i + 2);
    }
    iom->wait_all();
}