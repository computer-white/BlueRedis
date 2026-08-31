#include "blue/io_manager.h"
#include "blue/asyncio.h"
#include "blue/await.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
using namespace blue;
Task<void> test_compute()
{
    // 纯 CPU 计算
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) 
    {
        sum += i;
    }
    co_return;
}

int main()
{
    blue::IOManager iom(4);
    
    auto start = std::chrono::steady_clock::now();
    int total_tasks = 1000000;
    
    for (int i = 0; i < total_tasks; i++)
    {
        iom.schedule(test_compute());
    }
    
    iom.wait_all();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Processed " << total_tasks << " tasks in " 
              << duration.count() << "ms" << std::endl;
    std::cout << "QPS: " << (total_tasks * 1000 / duration.count()) << std::endl;
    
    return 0;
}