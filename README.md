# Blue - C++20 协程异步网络框架

基于 C++20 无栈协程的高性能异步网络框架，支持 epoll 事件驱动、SSL/TLS、HTTP 客户端/服务端、连接池、异步日志轮转。

主要在上一个版本blue_proxy(基于sylar的服务器框架完成后搭配ai写的简易代理服务器)上做了修改
有些测试代码没有给出，再次重写测试代码跟现在的版本不一样，测试函数内部必须使用co_await并返回Task<T>类型.
可以参考newtests/test_debug.cpp这个简单的版本.


## 特性

- **C++20 无栈协程** — `Task<T>`、`Scheduler`、`IOManager`，对称转移（Symmetric Transfer），零调度开销
- **epoll 事件驱动** — 异步 I/O（`co_await Read/Write/Accept`）、定时器（`co_await sleepFor`）、SSL/TLS
- **HTTP 客户端** — GET/POST、HTTPS、连接池（Keep-Alive 复用）、超时控制
- **HTTP 服务端** — 基于 llhttp 的请求解析、Servlet 路由、反向代理
- **异步日志** — 无锁 SPSC 队列 + 后台线程 + 按大小自动轮转
- **配置系统** — YAML / JSON 热加载，支持配置变更回调

## 性能

| 测试 | 并发 | QPS | 平均延迟 | 成功率 |
|:---|:---|:---|:---|:---|
| Echo Server (wrk, 本地) | 100 | 11,583 | 10ms | 100% |
| HTTP 客户端 (本地 Echo, 连接池) | 500 | 1,650 | 4.5ms | 100% |
| HTTP 客户端 (百度, 连接池) | 200 | 139 | 106ms | 100% |
| HTTPS 客户端 (百度, 连接池) | 100 | 85 | 184ms | 100% |

## 快速开始

### 依赖

bash
# 编译器
sudo apt install g++-12

# 库
sudo apt install libboost-iostreams-dev libboost-coroutine-dev libboost-context-dev
sudo apt install libssl-dev libyaml-cpp-dev nlohmann-json3-dev
sudo apt install libmysqlclient-dev libhiredis-dev



### 运行测试

# echo 测试
../bin/test_echo &
nc localhost 8080

# HTTP 压力测试
../bin/test_benchmark

# I/O Manager 测试
../bin/test_new_iomanager

# 异步日志测试
../bin/test_asynclog

### 结构
newblue/
| 
|-- blue/
|    ├-- task.h              # 协程任务 (Task<T>)，支持 co_await 嵌套
|    ├-- scheduler.h/cpp     # 线程池调度器，任务队列 + 生命周期管理
|    ├-- io_manager.h/cpp    # epoll 事件循环，FdContext，tickle 唤醒
|    ├-- timer.h/cpp         # 定时器管理，std::set 排序，processExpired
|    ├-- sslsocket.h/cpp     # SSL/TLS 封装，异步握手 + 读写
|    ├-- ssl_asyncio.h       # SSL 异步 I/O awaiter (SRead/SWrite/SConnect)
|    ├-- async_logger.h/cpp  # 异步日志器，SPSC 无锁队列 + 轮转
|    ├-- spsc_queue.h        # 单生产者单消费者无锁队列
|    ├-- logentry.h          # 日志条目，支持格式化输出
|    ├-- config.h/cpp        # 配置系统，YAML/JSON 解析，热加载 + 回调
|    ├-- await.h             # 常用 awaiter (sleepFor/Accept/Recv/Send)
|    ├-- asyncio.h           # 异步 I/O awaiter (Read/Write/ReadWithTimeout)
|    ├-- msocket.h/cpp       # 非阻塞 TCP Socket
|    ├-- url.h/cpp           # URL 解析
|    ├-- address.h/cpp       # DNS 解析 + 地址抽象
|    ├-- mthread.h/cpp       # 线程封装
|    ├-- hook.h/cpp          # 系统调用 Hook（兼容旧版）
|-- http/                        # HTTP 子系统
|    ├-- http.h/cpp               # HTTP 请求/响应/状态码定义
|    ├-- httpconnection.h/cpp     # HTTP 客户端 + 连接池
|    ├-- httpParser.h/cpp         # HTTP 解析器（基于 llhttp）
|    ├-- httpserver.h/cpp         # HTTP 服务器
|    ├-- httpservlet.h/cpp        # Servlet 路由
|    └-- httpsession.h/cpp        # HTTP 会话管理
|-- newblue/
    |-- asyncio.h                   # 最初版的协程io
    |-- await.h                     # 最初版的sleepFor
    |-- io_manager.h/cpp            # 最初版iomanager
    |-- scheduler.n/cpp             # 最初版scheduler
    |-- task.h                      # 未使用对称转移的task(后面改成模板一出错不行了ai才透露出这个写法)
    |-- timer.h/.cpp                # 将sylar中timer改写了一些也是最初版的timer

newblue 中是刚开始准备重构的最初版

### 协程用例

## 所有返回Task<T>的(或者说内部使用co_await等等,其实内部使用了co_await家族的函数返回值都不许为普通类型了)都是协程，注意协程函数参数不能有引用，形参必须全部使用值传递，当然在传递实参时使用std::move()更推荐

#include "blue/task.h"
## 1
// 无返回值协程
blue::Task<void> hello() {
    std::cout << "Hello, ";
    co_await blue::sleepFor(1);  // 挂起 1 秒
    std::cout << "World!" << std::endl;
    co_return;
}

// 有返回值协程
blue::Task<int> compute() {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i;
        co_await blue::sleepForMs(10);  // 每 10ms 让出 CPU
    }
    co_return sum;
}

// 协程嵌套：co_await 另一个协程
blue::Task<void> parent() {
    std::cout << "开始计算..." << std::endl;
    int result = co_await compute();
    std::cout << "结果: " << result << std::endl;
    co_return;
}

initial_suspend 返回 suspend_always：协程创建后挂起，由调度器 resume() 启动
final_suspend 返回 SubCorroutine：协程结束时对称转移回父协程，不经过调度器
支持 co_await嵌套，子协程异常会传播到父协程

## 2 IOManager
#include "blue/io_manager.h"
#include "blue/asyncio.h"

// 异步读取文件（或 socket）
blue::Task<void> read_file(int fd) {        // 注意参数不能带有& 即int &fd
    char buf[1024];
    ssize_t n = co_await blue::Read(fd, buf, sizeof(buf));
    if (n > 0) {
        std::cout << "读到 " << n << " 字节" << std::endl;
    }
    co_return;
}

// 带超时的异步读取
blue::Task<void> read_with_timeout(int fd) {        // 注意参数不能带有& 即int &fd
    char buf[1024];
    ssize_t n = co_await blue::ReadT(fd, buf, sizeof(buf), 5000);
    if (n <= 0) {
        std::cout << "超时或断开" << std::endl;
    }
    co_return;
}

int main() {
    blue::IOManager iom(4);  // 4 个 worker 线程
    iom.schedule(read_file(some_fd));
    iom.schedule(read_with_timeout(another_fd));
    iom.wait_all();     // 等待所有任务完成(包括定时任务)
}

## 3 定时器
#include "blue/await.h"

blue::Task<void> timer_demo() {
    std::cout << "开始等待..." << std::endl;
    
    co_await blue::sleepFor(2);      // 挂起 2 秒, co_await 不能出现在main函数
    std::cout << "2 秒到了" << std::endl;
    
    co_await blue::sleepForMs(500);  // 挂起 500 毫秒
    std::cout << "500 毫秒到了" << std::endl;
    
    co_return;
}

可以在main中使用:
IOManager iom(2);
iom.schedule(tiemr_demo());
iom.wait_all();

## 想要测试一些http相关

# test_httpconnection 可以测试一些客户端相关，给baidu或httpbin.org发送请求来接收响应
# test_httpserver 可以测试一些服务器相关的，运行后在浏览器输入路径为[localhost:8082/admin](http://localhost:8082/admin),可以看到
# 相关的网页,你可以添加路由即ds->addServlet，来对于不同的url返回不同的响应,也可以输入http://localhost:8082/blue/https://www.baidu.com，
# 查看是否可以输出百度的页面，我这边好像被百度限流了，页面显示Rate limit exceeded，这段时间一直给百度发请求...

代码有90%以上ai完成或提供思路,测试代码几乎你看到的有注释的就是ai写的, test_httpconnection,test_httpserver这两不是








