#include <iostream>
#include "blue/io_manager.h"
#include "blue/task.h"
#include "blue/await.h"

blue::Task<int> child1()
{
    co_await blue::sleepFor(1);  // 会泄漏（崩溃了ccc）     最终解决（感谢deepSeek）
    std::cout << "child1\n";
    co_return 1;
}

blue::Task<void> child2()
{
    int x = co_await child1();
    std::cout << "child2, x: " << x << "\n";
    co_return;
}

blue::Task<void> child3()
{
    co_await child2();
    std::cout << "child3\n";
    co_return;
}

blue::Task<void> simple()
{
    co_await blue::sleepForMs(1);
    int x = co_await child1();
    std::cout << "x: " << x << "\n";
    co_await child2();
    co_await child3();
    std::cout << "1\n";
    co_return;
}

blue::Task<void> test_MulTask()
{
    std::cout << "begin\n";
    co_await child1();
    co_await child2();
    co_await child3();
    std::cout << "end\n";
}

blue::Task<void> test_onlyonlyTask()
{
    std::cout << "tem\n";
    co_return;
}

blue::Task<void> test_onlySleepFor()
{
    std::cout << "begin\n";
    co_await blue::sleepForMs(1);
    std::cout << "end\n";
    co_return;
}

int main()
{
    blue::IOManager iom(2);
    iom.scheduleMul(-1, test_onlyonlyTask(), test_MulTask(), test_onlySleepFor(), simple());

    iom.wait_all();
}// touch Thu Sep 17 18:50:15 CST 2026
