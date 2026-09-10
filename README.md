# Blue - C++20 协程服务器框架

基于 C++20 无栈协程的异步网络框架，支持 epoll 事件驱动、SSL/TLS、HTTP 客户端/服务端、连接池、异步日志轮转。
目前扩展了redis

## 特性

- **C++20 无栈协程** — `Task<T>`、`Scheduler`、`IOManager`，对称转移（Symmetric Transfer），零调度开销
- **epoll 事件驱动** — 异步 I/O（`co_await Read/Write/Accept`）、定时器（`co_await sleepFor`）、SSL/TLS
- **HTTP 客户端** — GET/POST、HTTPS、连接池（Keep-Alive 复用）、超时控制
- **HTTP 服务端** — 基于 llhttp 的请求解析、Servlet 路由、反向代理
- **异步日志** — 无锁 SPSC 队列 + 后台线程 + 按大小自动轮转
- **配置系统** — YAML / JSON 热加载，支持配置变更回调
- **redis** — 使用c++20协程的redis服务器

## 关于一些模块
### 关于Redis服务器的入口文件(newtests/test_commandHandler.cpp)
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
## 依赖
bash
## 编译器
sudo apt install g++-12

## 库
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
    mkdir build && cd build
    cmake ..    # 默认是命令表
    make -j$(nproc)
```

## 启动服务器

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
    # 基础性能测试
    redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t set,get -c 100 -n 100000

    # 管道模式测试
    redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t set,get -P 32 -c 100 -n 1000000 -q
# 结果
### Pipe                   
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
### 非Pipe
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,rpush,lpop,rpop,sadd,srem -q
    WARNING: Could not fetch server CONFIG
    SET: 69871.44 requests per second, p50=1.343 msec                   
    GET: 80808.08 requests per second, p50=1.079 msec                   
    LPUSH: 66379.02 requests per second, p50=1.367 msec                   
    RPUSH: 69584.58 requests per second, p50=1.327 msec                   
    LPOP: 67015.15 requests per second, p50=1.271 msec                   
    RPOP: 70866.70 requests per second, p50=1.247 msec                    
    SADD: 65828.45 requests per second, p50=1.391 msec 
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
## 1.使用管理员登录进去，输入shutdown
    blue@Plus:~/c_projects/newblue$ redis-cli -p 6666
    127.0.0.1:6666> auth admin123
    OK
    127.0.0.1:6666> shutdown
    "OK - waiting for clients to disconnect"
    127.0.0.1:6666> exit
## 2.直接按ctrl + c
    ctrl + c

# 欢迎提交 Issue 和 Pull Request！

# License
This project is licensed under the GNU General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details.
