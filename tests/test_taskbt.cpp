#include <iostream>
#include "blue/io_manager.h"
#include "blue/task.h"
#include "blue/await.h"

blue::Task<int> child1()
{
    std::cout << "child1 begin\n";
    co_await blue::sleepFor(1);  // 会泄漏（崩溃了ccc）     最终解决（感谢deepSeek）
    std::cout << "child1 end\n";
    co_return 1;
}

blue::Task<void> child2()
{
    std::cout << "child2 begin\n";
    int x = co_await child1();
    std::cout << "child2, x: " << x << "\n";
    co_return;
}

blue::Task<void> child3()
{
    std::cout << "child3 begin\n";
    co_await child2();
    std::cout << "child3 end\n";
    co_return;
}

blue::Task<void> simple()
{
    std::cout << "simple begin\n";
    co_await blue::sleepForMs(1);
    int x = co_await child1();
    std::cout << "x: " << x << "\n";
    co_await child2();
    co_await child3();
    std::cout << "simple end\n";
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

blue::Task<void> test_onlyoneTask()
{
    std::cout << "onlyoneTask begin\n";
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
    iom.scheduleMul(test_onlyoneTask(), test_MulTask(), test_onlySleepFor(), simple());
    // iom.schedule(test_onlySleepFor());
    // iom.schedule(child3());

    iom.wait_all();
}// touch Thu Sep 17 18:50:15 CST 2026
