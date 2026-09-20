# Blue - C++20 协程服务器框架
[![CI](https://github.com/computer-white/BlueRedis/actions/workflows/ci.yml/badge.svg)](https://github.com/computer-white/BlueRedis/actions/workflows/ci.yml)

基于 C++20 无栈协程的异步网络框架，支持 epoll 事件驱动、HTTP 客户端/服务端、连接池。

## 特性

- **C++20 无栈协程** — `Task<T>`、`Scheduler`、`IOManager`，对称转移（Symmetric Transfer）
- **epoll 事件驱动** — 异步 I/O（`co_await Read/Write/Accept`）、定时器（`co_await sleepFor`）
- **HTTP 客户端** — GET/POST、HTTPS、连接池（Keep-Alive 复用）
- **HTTP 服务端** — 基于 llhttp 的请求解析、Servlet 路由
- **配置系统** — YAML / JSON 热加载，支持配置变更回调
- **redis** — 使用c++20协程的redis服务器

## 关于一些模块
### 关于Redis服务器的入口文件(tests/blueRedis.cpp)
    这个是Redis服务器的测试函数，IO调度器提交协程任务，然后协程被执行，comm->start()后Redis服务器就成功启动了
### 关于协程以及相关IO、Sleep实现(blue/task.h，blue/asyncio.h，blue/await.h，redis_command/generator.h)
    这些是关于协程和利用协程实现的IO和Sleep操作,在asyncio.h和await.h内部搭配epoll和定时器实现。关于C++20协程的用法这里不过多解释，
    这里只是它的一种使用方式，大家可以在github或其他地方或通过AI学习到其他的实现和使用方法。关于这部分协程的使用测试文件，我这里有简单的
    newtests/test_debug.cpp。最后一个redis_command/generator.h这个是一个异步生成器，测试文件有newtests/test_generator_game.cpp和
    newtests/test_generator.cpp。
### 关于调度器和定时器模块(blue/scheduler.h，blue/io_manager.h，blue/timer.h)
    关于调度器，这里为了适配c++20协程，我搭配AI改了之前跟着Sylar写的这块的逻辑，但是仔细看的话还是没有太大改变(^_^)，调度器支持提交
    协程句柄，协程（返回Task<T>类型的函数），以及CallBack函数。最后都已CallBack函数提交到任务队列，这块AI帮我做了优化，性能对比优化
    前提升了好几倍，距离我写这段过去好几个月了，当时也没仔细算。IO_manager这块还是基于epoll，对比Sylar的改动不大，定时器这块实际改变
    也不大...
### 存储文件路径(需要提前创建，并且需要让当前项目所属用户具有相应权限)
    日志存放在/var/log/blueRedis/logs_dir/
    AOF持久文件和RDB持久文件放在/var/lib/blueRedis/
    配置文件在/etc/blueRedis/redis_cof/blueredis.yml

### 配置文件中的一些参数说明
#### /etc/blueRedis/redis_cof/blueredis.yml"
```bash
    redis.max_command_size: 1M # 表示程序中的RESP命令解析器缓冲区最大大小
    redis.max_batch_size: 256K # 服务器批量响应大小阈值
    redis.max_exec_batch_size: 256 # 服务器批量执行的命令条数(即客户端单次输入的命令最大个数)
    redis.timeout: 60s # 服务器与客户端之间最长接收的待机时长
    redis.maxClients: 10000 # 服务器最大接收的客户端连接
    redis.admin.password: admin # 服务器管理员密码(可以单独放到一个文件通过设置权限来保证安全，也可以选择不设置直接使用ctrl + c来关闭服务器)
    redis.aof.aof_enabled: false # 服务器是否开启AOF写命令追加保存
    redis.aof.aof_filename: "appendonly.aof" # 服务器AOF文件名
    redis.aof.aof_max_buffer_size: 10M # 服务器AOF写入文件的最大缓冲区大小
    redis.aof.aof_max_file_size: 10M # AOF持久化文件的最大大小
    redis.aof.aof_max_file_number: 10 # AOF持久化文件轮转文件数量(文件最多保存多少)
    redis.slowlog.slow_log_slower_than: 10ms # 规定执行时长大于10ms的记录为慢日志
    redis.slowlog.slow_log_max_len: 256 # 慢日志缓存数组最大长度
```

## 编译器
```bash
sudo apt install g++-13 gcc-13
```

## 库
```bash
sudo apt install libboost-iostreams-dev libboost-coroutine-dev libboost-context-dev
sudo apt install libssl-dev libyaml-cpp-dev nlohmann-json3-dev
sudo apt install libmysqlclient-dev libhiredis-dev
sudo apt install -y \
    g++-12 \
    cmake \
    libevent-dev \
    libabsl-dev \
    libfmt-dev \
    libgflags-dev \
    libglog-dev \
    libboost-all-dev \
    libssl-dev \
    libhiredis-dev \
    libyaml-cpp-dev \
    ragel
```
```bash
**第三方源码**:`thirdparty/llhttp-release-v9.2.0/` 已 vendor 进仓库,
克隆后无需额外下载。
```

## 支持命令
<details> <summary><b>点击展开完整命令列表</b></summary>
连接命令
PING, AUTH, SELECT, CLIENT, CONFIG

String 命令
SET, GET, MSET, MGET, APPEND, SETNX, INCR, INCRBY, DEL, EXISTS, STRLEN, TYPE, GETSET

Hash 命令
HSET, HGET, HGETALL, HDEL, HLEN, HEXISTS, HKEYS, HVALS

List 命令
LPUSH, RPUSH, LPOP, RPOP, LRANGE, LLEN, LINSERT, LINDEX, LSET, RPOPLPUSH, LPOPRPUSH

Set 命令
SADD, SMEMBERS, SREM, SISMEMBER, SCARD, SRANDMEMBER, SPOP, SDIFF, SINTER, SUNION, SMOVE

Sorted Set 命令
ZADD, ZRANGE, ZREM, ZSCORE, ZRANK, ZINCRBY, ZCOUNT, ZRANGEBYSCORE, ZREMRANGEBYSCORE, ZINCRBYFLOAT

Server 命令
FLUSHDB, DBSIZE, INFO, SAVE, BGSAVE, LASTSAVE, COMMAND, ECHO, TIME, LOCALTIME, SHUTDOWN, KEYS, EXPIRE, TTL, PEXPIRE, PTTL, PERSIST, RENAME, RENAMENX, RANDOMKEY

事务命令
MULTI, EXEC, DISCARD, WATCH, UNWATCH

高级功能
SUBSCRIBE, PUBLISH, UNSUBSCRIBE, SLOWLOG, MONITOR, AOFROTATE, REPLICAOF, SLAVEOF
</details>

# 说明

## 协程模型
每个客户端连接对应一个协程
使用 IOManager 调度协程
对称转移减少协程切换开销

## 存储引擎
128 分片：将数据分散到 128 个分片，减少锁竞争

16 个数据库：支持 Redis 多数据库模式

absl::flat_hash_map：高性能哈希表，比 std::unordered_map 快 30%

## 持久化
RDB：定期快照保存

AOF：追加日志，支持 always、everysec、no 三种策略

# 快速开始

## 编译
```bash
git clone https://github.com/computer-white/BlueRedis.git blue
cd blue

mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

**产物位置**:

- 可执行文件:`bin/`
- 库文件:`lib/libblue.so` / `lib/libblue.a`

### 只构建库,不构建测试

```bash
cmake ..
```

### 开启 ASAN(推荐开发时使用)

```bash
cmake .. -DENABLE_ASAN=ON
make -j$(nproc)
```

---

## 测试

```bash
cd build
make -j$(nproc)
ctest --output-on-failure
```

### 只跑特定测试

```bash
ctest -R test_url --output-on-failure
./bin/test_task --gtest_filter='TaskBasic.*'
./bin/test_iomanager --gtest_filter='IOFdTest.*'
```

### 用 ASAN 跑全部测试

```bash
cmake .. -DENABLE_ASAN=ON
make -j$(nproc)
ctest --output-on-failure
```

---

## ⚠️ 开发注意事项

> 以下是踩过坑之后总结的经验。**写协程代码前请读一遍**,能省几小时调试。

---

### 1. 协程 lambda 捕获栈变量 → 悬空引用

C++20 协程帧**只保存 lambda 对象的 `this` 指针**,不保存 lambda 对象本身。如果 lambda 是临时对象且协程挂起,捕获的变量会在挂起点之后悬空。

**禁止**:

```cpp
// ❌ 危险:协程帧只保存 lambda 的 this,挂起后 lambda 对象可能已析构
iom.schedule([x, y]() -> blue::Task<void> {
    co_await sleepForMs(10);
    use(x, y);   // 💥 可能访问已析构的 lambda 对象
}());
```

**正确做法**:把数据通过**函数参数**传入(参数会被拷贝到协程帧,生命周期与协程一致):

```cpp
// ✅ 安全
// 同时注意协程参数不能使用&引用，一般我会使用值传递加上std::move()来避免不必要的拷贝
blue::Task<void> do_thing(X x, Y y) {
    co_await sleepForMs(10);
    use(x, y);
    co_return;
}
iom.schedule(do_thing(x, y));
```

**判断标准**:只要是"会挂起"的协程,就**不能用 lambda 捕获**。无捕获的 lambda 或普通函数才安全。

---

### 2. 协程句柄所有权规则

```bash
我重新设计了架构，或者说改正了架构，让顶层Task的生命周期跟随Handle的生命周期
我最开始用的是把Task设计为shared_ptr然后同promise_type绑定(在promise_type中保存为self),
在通过schedule提交给调度器时，通过std::coroutine_handle<typename blue::Task<T>::promise_type>::from_address(h.address())
来拿到带有promise类型的handle并将handle.promise().self = shared_ptr(Task<T>)
以此来拉长Task的生命周期，并当final_suspend时（即协程结束了）调用self.reset()。
期间顶层Task需要co_await SleepFor()时 需要通过拿到带有promise类型的handle并拿到 auto task = handle.promise().self
之后通过提交给定时器一个lambda捕获task来提升生命周期。

这是个屎山设计，在一部分测试中（可以看tests/test_taskbt.cpp中当child()没有co_await SleepFor(1)时）
他会正常运行，但是当出现 a co_await b; b co_await SleepFor()时
就会出现内存泄漏，原因我想是因为b是一个临时的Task，它并没有设置self，导致临时Task在定时器中没有被延长存储就被析构了
之后导致顶层协程不会被恢复从而导致内存泄漏

仔细想，目的就是要去延迟顶层Task的生命周期，我不希望Task被提交给调度器后在handle变为done之前就析构了
至于临时的那些Task他们结束后handle被destroy就行，无需担心。（另外测试中有一些测试是当协程被创建但是没有被执行
当Task析构时，handle也应该destroy）

所以最终设计为Task只管理对称转移，Task的生命周期由调度器统一管理，由于最初的Task是一个模板，而调度器不是一个模板
导致我一开始就pass了这个想法，但是被AI敲打后，发现这应该才是合理的解耦的设计。感叹模板编程的强大，赞美AI

在我目前代码的协程架构上，协程之间会出现以下情况:
1. 协程内部只进行业务逻辑，处理完就退出
2. 协程内部的业务逻辑需要`等待单个子协程完成`
3. 协程内部需要co_await sleepFor(1)来等待异步sleep
4. 协程内部需要`等待多个子协程完成`
5. 协程内部既需要`等待多个子协程又需要等待异步sleep`
6. 协程相互等待 a co_await b; b co_await c; c co_await d;
7. 协程相互等待，子协程又在等待异步sleep
目前我知道的协程之间用法就这么多，那么对应的测试文件为 tests/test_taskbt.cpp
详细细节请看测试文件。此外其他带有协程的测试文件有tests/test_task.cpp、test_iomanager.cpp、test_defer.cpp
test_defer.cpp是模拟go语言的defer的一个简单的defer
```
---

### 3. `Task` 是懒启动的

因为 `initial_suspend` 返回 `suspend_always`,创建后协程体**不会自动执行**:

```cpp
auto t = add_async(1, 2);   // ← 此时协程体一行都没跑
t.resume();                 // ← 需要显式驱动
t.get();                    // ← 拿结果 / 重抛异常
```

**这带来的好处**:可以精确控制何时启动,避免意外的同步执行。

**这带来的注意点**:忘了 `resume` 会导致协程永远不执行;

## 快速开始

```cpp
#include "blue/io_manager.h"
#include "blue/await.h"

blue::Task<void> hello()
{
    co_await blue::sleepForMs(100);
    std::cout << "Hello from coroutine" << std::endl;
    co_return;
}

int main()
{
    blue::IOManager iom(4);
    iom.schedule(hello());
    iom.wait_all();
}
```

### 4.协程的完成只能通知外部去清理，但是不能自己清理，所以在final_suspend里面不能调用任何可能删除当前协程帧的操作或回调
```cpp
// 函数内部不能调用或使用可能导致当前即将进行对称转移的协程帧被销毁的操作
// 这个外部回调会包含清理即将使用的协程帧的操作，跟上面的self.reset()本质
// 一样
SubCorroutine final_suspend() noexcept
{
    if (on_complete)
    {
        auto cb = std::move(on_complete);
        on_complete = nullptr;
        cb();
    }
    return SubCorroutine{fa};
}
```

## 启动Redis服务器(保证没有开UBSAN,会与absl冲突)

### 1. Default (Localhost)
```bash
./bin/blueredis-server
# Server started at 127.0.0.1:6666
```

### 2. Custom Host and Port
```bash
./bin/blueredis-server --bind 0.0.0.0 --port 6379
# Or using short options
./bin/blueredis-server -b 0.0.0.0 -p 6379
```

### 3. Show Help
```bash
./bin/blueredis-server --help
```

### 4. Connect with redis-cli
```bash
redis-cli -h 127.0.0.1 -p 6666
127.0.0.1:6666> PING
PONG
```


# 性能测试

## 命令
```bash
    # 基础性能测试
    redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t set,get -c 100 -n 100000

    # 管道模式测试
    redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t set,get -P 32 -c 100 -n 1000000 -q
```
# 结果
### Pipe
```bash              
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 8 -q
    WARNING: Could not fetch server CONFIG
    SET: 162548.77 requests per second, p50=4.775 msec                    
    GET: 245941.95 requests per second, p50=3.135 msec                    
    LPUSH: 148257.97 requests per second, p50=4.847 msec                     
    RPUSH: 158052.78 requests per second, p50=4.943 msec                    
    LPOP: 144300.14 requests per second, p50=5.375 msec                    
    RPOP: 132450.33 requests per second, p50=5.415 msec                    
    SADD: 147710.48 requests per second, p50=4.871 msec                    

    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 16 -q
    WARNING: Could not fetch server CONFIG
    SET: 195924.77 requests per second, p50=7.951 msec                      
    GET: 235904.70 requests per second, p50=5.127 msec                     
    LPUSH: 192122.97 requests per second, p50=8.127 msec                      
    RPUSH: 166861.34 requests per second, p50=8.031 msec                     
    LPOP: 157257.42 requests per second, p50=8.783 msec                     
    RPOP: 163398.70 requests per second, p50=8.847 msec                      
    SADD: 170183.80 requests per second, p50=8.479 msec                    

    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 32 -q
    WARNING: Could not fetch server CONFIG
    SET: 215563.70 requests per second, p50=14.511 msec                     
    GET: 337495.78 requests per second, p50=9.271 msec                     
    LPUSH: 214132.77 requests per second, p50=14.679 msec                     
    RPUSH: 208376.75 requests per second, p50=14.775 msec                     
    LPOP: 197199.77 requests per second, p50=15.855 msec                     
    RPOP: 198570.30 requests per second, p50=15.983 msec                   
    SADD: 211237.84 requests per second, p50=14.951 msec
```
### 非Pipe
```bash
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,rpush,lpop,rpop,sadd,srem -q
    WARNING: Could not fetch server CONFIG
    SET: 69871.44 requests per second, p50=1.343 msec                   
    GET: 80808.08 requests per second, p50=1.079 msec                   
    LPUSH: 66379.02 requests per second, p50=1.367 msec                   
    RPUSH: 69584.58 requests per second, p50=1.327 msec                   
    LPOP: 67015.15 requests per second, p50=1.271 msec                   
    RPOP: 70866.70 requests per second, p50=1.247 msec                    
    SADD: 65828.45 requests per second, p50=1.391 msec
```
# 机器参数
```bash
===== CPU =====
Architecture:                         x86_64
CPU(s):                               16
Model name:                           12th Gen Intel(R) Core(TM) i5-12500H
===== 内存 =====
               total        used        free      shared  buff/cache   available
Mem:           7.6Gi       2.2Gi       2.1Gi       1.0Mi       3.3Gi       5.2Gi
Swap:          2.0Gi       371Mi       1.6Gi
===== 磁盘 =====
Filesystem      Size  Used Avail Use% Mounted on
none            3.9G     0  3.9G   0% /usr/lib/modules/6.6.87.2-microsoft-standard-WSL2
none            3.9G  4.0K  3.9G   1% /mnt/wsl
drivers         909G  474G  435G  53% /usr/lib/wsl/drivers
/dev/sdd       1007G   61G  896G   7% /
none            3.9G  124K  3.9G   1% /mnt/wslg
none            3.9G     0  3.9G   0% /usr/lib/wsl/lib
rootfs          3.8G  2.7M  3.8G   1% /init
none            3.9G  936K  3.8G   1% /run
none            3.9G     0  3.9G   0% /run/lock
none            3.9G  4.0K  3.9G   1% /run/shm
none            3.9G   76K  3.9G   1% /mnt/wslg/versions.txt
none            3.9G   76K  3.9G   1% /mnt/wslg/doc
C:\             909G  474G  435G  53% /mnt/c
snapfuse         51M   51M     0 100% /snap/snapd/27710
snapfuse         51M   51M     0 100% /snap/snapd/27738
snapfuse         67M   67M     0 100% /snap/core24/1643
===== 系统 =====
 Static hostname: Plus
       Icon name: computer-container
         Chassis: container
      Machine ID: 626723b9f60d4c5696cd170f0175d379
         Boot ID: c91285b3aba24b148269ed9103855ec3
  Virtualization: wsl
Operating System: Ubuntu 22.04.5 LTS                    
          Kernel: Linux 6.6.87.2-microsoft-standard-WSL2
    Architecture: x86-64
```
# 关闭服务器
### 1.使用管理员登录进去，输入shutdown
```bash
    blue@Plus:~/c_projects/newblue$ redis-cli -p 6666
    127.0.0.1:6666> auth admin123
    OK
    127.0.0.1:6666> shutdown
    "OK - waiting for clients to disconnect"
    127.0.0.1:6666> exit
```
### 2.直接按ctrl + c
    ctrl + c

# 欢迎提交 Issue 和 Pull Request！

# License
### This project is licensed under the GNU General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details.
