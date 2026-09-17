#include <iostream>
#include "blue/io_manager.h"
#include "blue/task.h"
#include "blue/await.h"

blue::Task<void> child()
{
    co_await blue::sleepForMs(1);
    std::cout << "child\n";
    co_return;
}

blue::Task<void> simple()
{
    // 重大发现，co_await后对称转移把父协程挂起到空操作，实际上在这个函数上跑的线程被放出去了
    // 放在我的调度器上来看就是，task.cb()‘执行完’回来了，但是又没完全回来，等待被恢复后又重新执行
    // 所以需要在协程被析构时判断它是不是被调度器接管，就是destroySafe()中三层判断逻辑
    // 因为被挂起，所以可能导致进入destroySafe()，所以我们需要判断它是否被调度器接管，
    // 或者说是否是因为被挂起加上对称转移导致的‘假性’析构，或者他是一个孩子，并且这个孩子内部
    // 也co_await了一个子协程，那么这个孩子就不能被析构，因为他还要去恢复父协程
    

    // 总结就是假如有 a co_await b; 此时因为a是顶层协程，若是它被提交到schedule中，会被设置为detached
    // 保证在b结束前它没有被销毁，（从而引发出，在这个架构下，如果没有将顶层协程提交给调度器，就需要自己手动设置
    // detached=true,同时可能需要设置Scheduler::SetThis(&iom)）。此时我们再加上b co_await c;那么对于b来说，他就是有a这个父协程在，所以也需要保证他不会被析构
    // 这正好对应destroySafe()中的if逻辑
    co_await blue::sleepFor(1);
    co_await child();
    std::cout << "1\n";
    co_return;
}

blue::Task<void> tem()
{
    std::cout << "tem\n";
    co_return;
}

blue::Task<void> tem1()
{
    co_await blue::sleepForMs(1);
    std::cout << "tem1\n";
    co_return;
}

int main()
{
    blue::IOManager iom(2);
    iom.schedule(simple());
    iom.wait_all();

    // 不需要设置detached
    auto x = tem();
    x.resume();

    // 需要设置detached
    // 更重要，Scheduler::SetThis(&iom)
    blue::Scheduler::setThis(&iom);
    auto y = tem1();
    auto h = y.getHandle();
    h.promise().detached = true;    // 说明handle句柄已被外部接管，其实就是不希望tem1内部co_await时误将tem1协程句柄释放
    y.resume();

    // 等待所有协程结束
    sleep(5);
}