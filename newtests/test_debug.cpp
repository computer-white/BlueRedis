#include "blue/blue.h"
#include "blue/io_manager.h"
#include <iostream>
#include "blue/await.h"

blue::Task<void> simple_test() {
    std::cout << "simple_test 开始" << std::endl;
    co_await blue::sleepFor(2);
    std::cout << "simple_test 结束" << std::endl;
    co_return;
}

int main() {
    blue::IOManager iom(4);
    iom.schedule(simple_test());
    iom.wait_all();
    std::cout << "main 退出" << std::endl;
}