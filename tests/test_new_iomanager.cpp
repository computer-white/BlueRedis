#include "newblue/io_manager.h"
#include "newblue/task.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>


// 异步读 awaiter
struct AsyncRead {
    int fd;
    char* buf;
    size_t len;
    ssize_t ret = -1;

    bool await_ready() {
        ret = ::read(fd, buf, len);
        if (ret >= 0) return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return false;
        return true;
    }

    void await_suspend(std::coroutine_handle<> h) {
        newblue::IOManager::GetThis()->addEvent(fd, newblue::IOManager::READ, h, nullptr);
    }

    ssize_t await_resume() {
        if (ret >= 0) return ret;
        ret = ::read(fd, buf, len);
        return ret;
    }
};

// 测试协程：从 fd 读数据
newblue::Task reader(int fd) {
    char buf[32] = {};
    std::cout << "[协程] 开始读取 fd=" << fd << std::endl;
    
    ssize_t n = co_await AsyncRead{fd, buf, sizeof(buf) - 1};
    
    if (n > 0) {
        buf[n] = '\0';
        std::cout << "[协程] 读到 " << n << " 字节: " << buf << std::endl;
    } else {
        std::cout << "[协程] 读取失败或 EOF" << std::endl;
    }
    co_return;
}


int main() {
    std::cout << "=== IOManager 测试 ===" << std::endl;
    
    int pipefd[2];
    pipe(pipefd);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    
    {
        newblue::IOManager iom(2);
        std::cout << "IOManager 已启动" << std::endl;
        
        // 用 schedule 提交协程
        iom.schedule(reader(pipefd[0]));
        std::cout << "reader 协程已提交" << std::endl;
        
        sleep(1);
        std::cout << "写入数据到管道..." << std::endl;
        const char* msg = "Hello Coroutine!";
        write(pipefd[1], msg, strlen(msg));
        
        sleep(1);
        
        std::cout << "=== 测试完成 ===" << std::endl;
        close(pipefd[0]);
        close(pipefd[1]);
    }
    sleep(1);
    return 0;
}

// int main() {
//     std::cout << "=== IOManager 测试 ===" << std::endl;
    
//     int pipefd[2];
//     pipe(pipefd);
//     fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    
//     newblue::IOManager iom(2);
//     std::cout << "IOManager 已启动" << std::endl;
    
//     // 手动管理 Task 生命周期
//     auto task = reader(pipefd[0]);   // 创建协程
//     task.resume();                    // 启动（会自动挂起在 co_await）
//     std::cout << "reader 协程已启动" << std::endl;
    
//     sleep(1);
//     std::cout << "写入数据到管道..." << std::endl;
//     const char* msg = "Hello Coroutine!";
//     write(pipefd[1], msg, strlen(msg));
    
//     sleep(1);  // 等 epoll 处理
    
//     std::cout << "=== 测试完成 ===" << std::endl;
//     close(pipefd[0]);
//     close(pipefd[1]);
//     return 0;
// }