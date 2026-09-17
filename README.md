[![CI](https://github.com/computer-white/BlueRedis/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/ci.yml)
[![ASAN](https://github.com/computer-white/BlueRedis/actions/workflows/asan.yml/badge.svg)](https://github.com/YOUR_USERNAME/YOUR_REPO/actions/workflows/asan.yml)
# Blue - C++20 协程服务器框架

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

协程句柄在生命周期中会被**多方持有**。每一方都必须清楚"什么时候我持有,什么时候我交出去"。

| 阶段 | 持有者 | 谁负责 destroy |
|---|---|---|
| `Task` 创建到 `schedule` | `Task` 对象 | `Task::destroySafe` |
| 已 `schedule`,在队列里 | `Scheduler` 队列 | worker 取出执行后 |
| 挂起在 timer 里 | `TimerManager` | 到期后转交 Scheduler |
| 挂起在 fd 上 | `FdContext` | 触发后转交 Scheduler |
| `co_await` 嵌套 | 父协程 | 父协程 `await_resume` 后 |
| **顶层协程完成** | **Scheduler** | **worker 里 `h.destroy()`** |

**三条铁律**:

1. **同一个句柄不能同时被 timer 和 fd 持有**(会 double destroy)
2. **recurring timer 只支持回调,不支持协程句柄**(句柄被消费后清空)
3. **`SubCorroutine` 不自毁**,由 `Scheduler::schedule(handle)` 的 worker 负责最终 destroy

---

### 3. `detached` 标记的真正含义

**`detached = true` 是"句柄所有权声明"**,不是"让协程能在任意线程跑"的魔法。

它表示:

> 这个协程句柄的**所有权已经交给外部**(调度器 / timer / fd / 父协程),
> **`Task` 对象析构时不应该 destroy 它**。

**什么时候需要 `detached = true`?**

| 场景 | 需要 detached? |
|---|---|
| 协程会挂起在 timer 上(`co_await sleepForMs`) | ✅ 需要 |
| 协程会挂起在 fd 上(`co_await FdAwaiter`) | ✅ 需要 |
| 协程只手动 `resume` 到底,不挂起 | ❌ 不需要 |
| 通过 `iom.schedule(Task)` 提交 | ✅ 自动设置,不用手动 |

**什么时候 `detached` 不够?**

**`detached` 只管"句柄销毁",不管"运行上下文"。** 如果协程要挂起在 timer / fd 上,还需要**当前线程有 `t_Scheduler` 上下文**(见第 4 条)。

---

### 4. `co_await` 挂起操作依赖线程局部上下文(这块可以看tests/test_taskbt.cpp)

`co_await sleepForMs(ms)` / `co_await FdAwaiter(...)` 内部会调 `IOManager::GetThis()`,
它依赖**当前线程**的 `t_Scheduler`(线程局部变量)。

**在 worker 线程里**:`setThis` 由 `Scheduler::start` 自动完成,直接 `iom.schedule(coro)` 即可。

**在调度器外的线程里**(比如 `main` 手动 `resume`),必须**手动设置 `Scheduler::setThis(&iom)`**:

```cpp
blue::IOManager iom(2);

// ⚠️ 必须:让 main 线程的 t_Scheduler 指向 iom（没有这个就会导致下面紧接着的症状）
blue::Scheduler::setThis(&iom);

auto y = tem1();
auto h = y.getHandle();
h.promise().detached = true;   // 句柄会被 timer 接管,Task 析构时不要 destroy
y.resume();

// ⚠️ iom 必须活到所有协程完成
sleep(5);
```

**症状**:崩溃在 `TimerManager::addTimer` 或 `IOManager::addEvent`,
gdb 里 `this` 是个**很小的值**(如 `0x168`),本质是 `nullptr->成员` 访问。

**原因**:`IOManager::GetThis()` 返回 `nullptr`,`nullptr->addTimer(...)` 里
`this->m_mutex` 的地址变成 `nullptr + 偏移量`,直接段错误。

**推荐做法**:**能用 `iom.schedule(coro)` 就别手动 `resume`。** `schedule` 会自动设 `detached` 并运行在正确的线程上。

---

### 5. `Task` 是懒启动的

因为 `initial_suspend` 返回 `suspend_always`,创建后协程体**不会自动执行**:

```cpp
auto t = add_async(1, 2);   // ← 此时协程体一行都没跑
t.resume();                 // ← 需要显式驱动
t.get();                    // ← 拿结果 / 重抛异常
```

**这带来的好处**:可以精确控制何时启动,避免意外的同步执行。

**这带来的注意点**:忘了 `resume` 会导致协程永远不执行;`Task` 析构时如果协程未完成,
`destroySafe` 会判断是否 `detached` 来决定是否销毁帧(见第 2、3 条)。

---

### 6. 不要在协程里用 GTest 的 `ASSERT_*`

`ASSERT_*` 会 `return`,协程里可能不走清理路径,导致句柄泄漏或状态不一致。

**用 `EXPECT_*`**(不提前返回,记录失败后继续执行)。

如果确实需要"提前中止",用 `if (!cond) co_return;` 或抛异常。

---

### 7. ASAN + UBSAN 是强制的

开发新功能时**至少**跑一次:

```bash
cmake .. -DENABLE_ASAN=ON
make -j$(nproc)
ctest --output-on-failure
```

**以下 bug 都是 ASAN / UBSAN 抓到的(靠功能测试根本发现不了)**:

| Bug | 检测器 | 说明 |
|---|---|---|
| 协程帧泄漏(72 字节 × N) | LeakSanitizer | `Task` 未 resume 或挂起后没人 destroy |
| 协程 lambda 捕获栈变量 | ASAN | `stack-use-after-scope` |
| `bool` 成员未初始化 | UBSAN | `load of value 64, which is not a valid value for type 'bool'` |
| 调度器重复 resume 已销毁句柄 | ASAN | `heap-use-after-free` |
| 句柄被 timer + fd 同时持有 | ASAN | `heap-use-after-free` |

**关于 UBSAN + absl 的已知冲突**:

`-fsanitize=undefined` 和 absl 的某些 `consteval` 模板代码(GCC 12/13)不兼容,
表现为 `hash_policy_traits.h: '(... == 0)' is not a constant expression`。

**解决方案**:

```cmake
option(ENABLE_ASAN  "Address Sanitizer"              OFF)
option(ENABLE_UBSAN "Undefined Behavior Sanitizer"   OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-sanitize-recover=all -g)
    add_link_options(-fsanitize=undefined)
endif()
```

**用法**:

```bash
cmake .. -DENABLE_ASAN=ON                  # 日常:ASAN 抓内存
cmake .. -DENABLE_UBSAN=ON                 # 定期:UBSAN 抓 UB(可能和 absl 冲突)
```

**UBSAN 冲突时**,给触发冲突的 target 单独关:

```cmake
if(ENABLE_UBSAN)
    target_compile_options(blueRedis PRIVATE -fno-sanitize=undefined)
endif()
```

---

### 8. `wait_all` 的语义

- 阻塞到**所有任务 + fd 事件 + timer** 都结束
- **不能**在 worker 线程里调(会死锁)
- 多次调用是安全的
- 内部用 `wait_for(10ms)` 兜底,避免条件变量的"丢失唤醒"导致偶发卡死

**丢失唤醒**是条件变量的经典陷阱:

- worker 修改状态后 `notify`,但 `wait_all` 还没真正进入 `wait`
- `notify` 丢失 → `wait_all` 永久阻塞
- **修法**:`wait_for` 加超时,即使丢失唤醒也能定期重试

---

### 9. 不要在 `IOManager` 析构前关闭它管理的 fd

`FdContext` 里记录了 fd,`~IOManager` 会 `close` 自己内部创建的 `eventfd` 和 `epfd`,
但**不负责关你注册进来的业务 fd**。

**正确做法**:先 `iom.wait_all()`,再关业务 fd,最后析构 `iom`:

```cpp
{
    blue::IOManager iom(2);
    
    int sock = socket(...);
    iom.addEvent(sock, READ, handle);
    // ...
    
    iom.wait_all();    // ← 先等所有任务结束
    close(sock);       // ← 再关业务 fd
}                      // ← iom 析构
```

---

### 10. 快速排查清单

遇到协程相关的问题,按这个顺序查:

1. **崩溃在 `addTimer` / `addEvent`,gdb 里 `this` 是小数值** → 当前线程 `t_Scheduler` 为 `nullptr`,见第 4 条
2. **ASAN 报 `stack-use-after-scope`** → 协程 lambda 捕获栈变量,见第 1 条
3. **ASAN 报 `heap-use-after-free`** → 句柄被多处持有或提前销毁,见第 2、3 条
4. **LeakSanitizer 报泄漏** → `Task` 未 resume 或挂起后没人 destroy,见第 2、3 条
5. **`wait_all` 偶发卡死** → 条件变量丢失唤醒,见第 8 条
6. **UBSAN 报 absl 的 `consteval` 错误** → ASAN / UBSAN 拆开,见第 7 条

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
