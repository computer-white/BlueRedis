#include <iostream>
#include <stdio.h>
#include "src/core/io_manager.h"
#include "src/core/task.h"
#include "src/core/await.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("root");

blue::Task<int> child1()
{
    g_logger->Info("child1 begin");
    co_await blue::sleepFor(1);  // 会泄漏（崩溃了ccc）     最终解决（感谢deepSeek）
    g_logger->Info("child1 end");
    co_return 1;
}

blue::Task<void> child2()
{
    g_logger->Info("child2 begin");
    int x = co_await child1();
    g_logger->Info("child1", ", x: ", x);
    co_return;
}

blue::Task<void> child3()
{
    g_logger->Info("child3 begin");
    co_await child2();
    g_logger->Info("child3 end");
    co_return;
}

blue::Task<void> simple()
{
    g_logger->Info("simple begin");
    co_await blue::sleepForMs(1);
    int x = co_await child1();
    g_logger->Info("x: ", x);
    co_await child2();
    co_await child3();
    g_logger->Info("simple end");
    co_return;
}

blue::Task<void> test_MulTask()
{
    g_logger->Info("MulTask begin");
    co_await child1();
    co_await child2();
    co_await child3();
    g_logger->Info("MulTask end");
}

blue::Task<void> test_onlyoneTask()
{
    g_logger->Info("onlyoneTask begin");
    co_return;
}

blue::Task<void> test_onlySleepFor()
{
    g_logger->Info("onlySleepFor begin");
    co_await blue::sleepForMs(1);
    g_logger->Info("onlySleepFor end");
    co_return;
}

blue::Task<void> child(int id, int seconds)
{
    g_logger->Info("[子", id, "] 开始，等待 ", 
        seconds, " 秒", "线程：", std::this_thread::get_id());
    co_await blue::sleepFor(seconds);
    std::cout << "[子" << id << "] 完成"
              << "（线程: " << std::this_thread::get_id() << "）" << std::endl;
    g_logger->Info("[子", id, "] 完成，线程：", std::this_thread::get_id());
    co_return;
}

blue::Task<void> father()
{
    g_logger->Info("父协程 begin");
    g_logger->Info("父协程开始等待子协程");
    // co_await child(1,1);
    // co_await child(2,2);
    // co_await child(3,3);
    blue::IOManager::GetThis()->schedule(child(1, 1));
    blue::IOManager::GetThis()->schedule(child(1, 2));
    blue::IOManager::GetThis()->schedule(child(1, 3));
    g_logger->Info("子协程全部结束");
    // 暴露一个问题，没有 go 的 waitGroup 导致father协程不能真正去等待child
    // 输出：

    // 父协程
    // 父亲协程开始等待子协程
    // 子协程全部结束
    // [子1] 开始，等待 1 秒（线程: 126157932459584）
    // [子1] 开始，等待 2 秒（线程: 126157932459584）
    // [子1] 开始，等待 3 秒（线程: 126157932459584）
    // [子1] 完成（线程: 126157924066880）
    // [子1] 完成（线程: 126157932459584）
    // [子1] 完成（线程: 126157932459584）
    co_return;
}

int main()
{
    blue::IOManager iom(2);
    iom.scheduleMul(test_onlyoneTask(), test_MulTask(), test_onlySleepFor(), simple());
    iom.schedule(test_onlySleepFor());
    iom.schedule(child3());
    iom.schedule(father());

    iom.wait_all();
}// touch Thu Sep 17 18:50:15 CST 2026
