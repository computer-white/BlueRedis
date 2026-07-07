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

## 快速开始

### 依赖

bash
# 编译器
sudo apt install g++-12

# 库
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

## 说明

# 协程模型
每个客户端连接对应一个协程
使用 IOManager 调度协程
对称转移减少协程切换开销

# 存储引擎
128 分片：将数据分散到 128 个分片，减少锁竞争

16 个数据库：支持 Redis 多数据库模式

absl::flat_hash_map：高性能哈希表，比 std::unordered_map 快 30%

# 持久化
RDB：定期快照保存

AOF：追加日志，支持 always、everysec、no 三种策略

## 快速开始

# 编译
    mkdir build && cd build
    cmake ..    # 默认是命令表
    make -j$(nproc)

# 启动服务器
    ../bin/test_commandHandler 127.0.0.1 6666

# 性能测试

# 命令
    # 基础性能测试
    redis-benchmark -h 127.0.0.1 -p 6666 -a admin123 -t set,get -c 100 -n 100000

    # 管道模式测试
    redis-benchmark -h 127.0.0.1 -p 6666 -a admin123 -t set,get -P 32 -c 100 -n 1000000 -q
# 结果

# 加入对象池前
# set,get Pipeline
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123     -c 50 -n 50000 -t set,get -q -P 8
    WARNING: Could not fetch server CONFIG
    SET: 142045.45 requests per second, p50=2.407 msec                    
    GET: 226244.34 requests per second, p50=1.647 msec  
    
# set Pipeline
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t set -c 100 -n 100000 -P 10
    WARNING: Could not fetch server CONFIG
    ====== SET ======                                                     
    100000 requests completed in 0.62 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.111 milliseconds (cumulative count 10)
    50.000% <= 5.807 milliseconds (cumulative count 52000)
    75.000% <= 5.911 milliseconds (cumulative count 75500)
    87.500% <= 6.047 milliseconds (cumulative count 87740)
    93.750% <= 6.327 milliseconds (cumulative count 93770)
    96.875% <= 7.983 milliseconds (cumulative count 96890)
    98.438% <= 11.551 milliseconds (cumulative count 98440)
    99.219% <= 22.719 milliseconds (cumulative count 99220)
    99.609% <= 26.575 milliseconds (cumulative count 99610)
    99.805% <= 27.215 milliseconds (cumulative count 99810)
    99.902% <= 27.903 milliseconds (cumulative count 99910)
    99.951% <= 28.783 milliseconds (cumulative count 99960)
    99.976% <= 29.231 milliseconds (cumulative count 99980)
    99.988% <= 29.407 milliseconds (cumulative count 99990)
    99.994% <= 29.631 milliseconds (cumulative count 100000)
    100.000% <= 29.631 milliseconds (cumulative count 100000)

    Cumulative distribution of latencies:
    0.000% <= 0.103 milliseconds (cumulative count 0)
    0.030% <= 0.207 milliseconds (cumulative count 30)
    0.050% <= 0.303 milliseconds (cumulative count 50)
    0.060% <= 0.703 milliseconds (cumulative count 60)
    0.070% <= 0.807 milliseconds (cumulative count 70)
    0.080% <= 1.103 milliseconds (cumulative count 80)
    0.100% <= 3.103 milliseconds (cumulative count 100)
    0.120% <= 4.103 milliseconds (cumulative count 120)
    0.150% <= 5.103 milliseconds (cumulative count 150)
    90.070% <= 6.103 milliseconds (cumulative count 90070)
    95.240% <= 7.103 milliseconds (cumulative count 95240)
    96.970% <= 8.103 milliseconds (cumulative count 96970)
    98.170% <= 9.103 milliseconds (cumulative count 98170)
    98.270% <= 10.103 milliseconds (cumulative count 98270)
    98.380% <= 11.103 milliseconds (cumulative count 98380)
    98.480% <= 12.103 milliseconds (cumulative count 98480)
    98.600% <= 13.103 milliseconds (cumulative count 98600)
    98.720% <= 14.103 milliseconds (cumulative count 98720)
    98.770% <= 15.103 milliseconds (cumulative count 98770)
    98.820% <= 16.103 milliseconds (cumulative count 98820)
    98.890% <= 17.103 milliseconds (cumulative count 98890)
    98.950% <= 18.111 milliseconds (cumulative count 98950)
    98.990% <= 19.103 milliseconds (cumulative count 98990)
    99.060% <= 20.111 milliseconds (cumulative count 99060)
    99.100% <= 21.103 milliseconds (cumulative count 99100)
    99.160% <= 22.111 milliseconds (cumulative count 99160)
    99.240% <= 23.103 milliseconds (cumulative count 99240)
    99.300% <= 24.111 milliseconds (cumulative count 99300)
    99.430% <= 25.103 milliseconds (cumulative count 99430)
    99.530% <= 26.111 milliseconds (cumulative count 99530)
    99.800% <= 27.103 milliseconds (cumulative count 99800)
    99.930% <= 28.111 milliseconds (cumulative count 99930)
    99.970% <= 29.103 milliseconds (cumulative count 99970)
    100.000% <= 30.111 milliseconds (cumulative count 100000)

    Summary:
    throughput summary: 162074.56 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            6.126     0.104     5.807     6.783    19.247    29.631

# get,set 混合
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t set,get -c 100 -n 100000
    WARNING: Could not fetch server CONFIG
    ====== SET ======                                                   
    100000 requests completed in 1.48 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.031 milliseconds (cumulative count 6)
    50.000% <= 1.343 milliseconds (cumulative count 52679)
    75.000% <= 1.423 milliseconds (cumulative count 75233)
    87.500% <= 1.527 milliseconds (cumulative count 87782)
    93.750% <= 1.879 milliseconds (cumulative count 93766)
    96.875% <= 2.079 milliseconds (cumulative count 96978)
    98.438% <= 3.079 milliseconds (cumulative count 98442)
    99.219% <= 5.111 milliseconds (cumulative count 99223)
    99.609% <= 7.455 milliseconds (cumulative count 99610)
    99.805% <= 10.783 milliseconds (cumulative count 99807)
    99.902% <= 13.495 milliseconds (cumulative count 99903)
    99.951% <= 15.287 milliseconds (cumulative count 99952)
    99.976% <= 15.911 milliseconds (cumulative count 99976)
    99.988% <= 17.151 milliseconds (cumulative count 99988)
    99.994% <= 18.607 milliseconds (cumulative count 99994)
    99.997% <= 19.551 milliseconds (cumulative count 99997)
    99.998% <= 20.623 milliseconds (cumulative count 99999)
    99.999% <= 21.087 milliseconds (cumulative count 100000)
    100.000% <= 21.087 milliseconds (cumulative count 100000)

    Cumulative distribution of latencies:
    0.054% <= 0.103 milliseconds (cumulative count 54)
    0.061% <= 0.207 milliseconds (cumulative count 61)
    0.064% <= 0.303 milliseconds (cumulative count 64)
    0.066% <= 0.407 milliseconds (cumulative count 66)
    0.067% <= 0.503 milliseconds (cumulative count 67)
    0.085% <= 0.607 milliseconds (cumulative count 85)
    0.094% <= 0.703 milliseconds (cumulative count 94)
    0.153% <= 0.807 milliseconds (cumulative count 153)
    0.344% <= 0.903 milliseconds (cumulative count 344)
    1.040% <= 1.007 milliseconds (cumulative count 1040)
    2.821% <= 1.103 milliseconds (cumulative count 2821)
    8.266% <= 1.207 milliseconds (cumulative count 8266)
    33.402% <= 1.303 milliseconds (cumulative count 33402)
    71.997% <= 1.407 milliseconds (cumulative count 71997)
    86.450% <= 1.503 milliseconds (cumulative count 86450)
    90.396% <= 1.607 milliseconds (cumulative count 90396)
    92.170% <= 1.703 milliseconds (cumulative count 92170)
    93.224% <= 1.807 milliseconds (cumulative count 93224)
    93.887% <= 1.903 milliseconds (cumulative count 93887)
    94.935% <= 2.007 milliseconds (cumulative count 94935)
    97.286% <= 2.103 milliseconds (cumulative count 97286)
    98.459% <= 3.103 milliseconds (cumulative count 98459)
    98.818% <= 4.103 milliseconds (cumulative count 98818)
    99.218% <= 5.103 milliseconds (cumulative count 99218)
    99.494% <= 6.103 milliseconds (cumulative count 99494)
    99.563% <= 7.103 milliseconds (cumulative count 99563)
    99.686% <= 8.103 milliseconds (cumulative count 99686)
    99.740% <= 9.103 milliseconds (cumulative count 99740)
    99.754% <= 10.103 milliseconds (cumulative count 99754)
    99.869% <= 11.103 milliseconds (cumulative count 99869)
    99.897% <= 12.103 milliseconds (cumulative count 99897)
    99.898% <= 13.103 milliseconds (cumulative count 99898)
    99.906% <= 14.103 milliseconds (cumulative count 99906)
    99.943% <= 15.103 milliseconds (cumulative count 99943)
    99.979% <= 16.103 milliseconds (cumulative count 99979)
    99.987% <= 17.103 milliseconds (cumulative count 99987)
    99.992% <= 18.111 milliseconds (cumulative count 99992)
    99.996% <= 19.103 milliseconds (cumulative count 99996)
    99.998% <= 20.111 milliseconds (cumulative count 99998)
    100.000% <= 21.103 milliseconds (cumulative count 100000)

    Summary:
    throughput summary: 67796.61 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.455     0.024     1.343     2.015     4.615    21.087
    ====== GET ======                                                   
    100000 requests completed in 1.10 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 2)
    50.000% <= 1.039 milliseconds (cumulative count 51961)
    75.000% <= 1.103 milliseconds (cumulative count 76874)
    87.500% <= 1.183 milliseconds (cumulative count 88092)
    93.750% <= 1.359 milliseconds (cumulative count 93763)
    96.875% <= 1.575 milliseconds (cumulative count 97024)
    98.438% <= 1.631 milliseconds (cumulative count 98473)
    99.219% <= 1.767 milliseconds (cumulative count 99225)
    99.609% <= 2.087 milliseconds (cumulative count 99610)
    99.805% <= 8.863 milliseconds (cumulative count 99805)
    99.902% <= 10.519 milliseconds (cumulative count 99906)
    99.951% <= 10.775 milliseconds (cumulative count 99952)
    99.976% <= 11.687 milliseconds (cumulative count 99976)
    99.988% <= 12.039 milliseconds (cumulative count 99988)
    99.994% <= 12.327 milliseconds (cumulative count 99994)
    99.997% <= 12.415 milliseconds (cumulative count 99997)
    99.998% <= 12.631 milliseconds (cumulative count 99999)
    99.999% <= 12.727 milliseconds (cumulative count 100000)
    100.000% <= 12.727 milliseconds (cumulative count 100000)

    Cumulative distribution of latencies:
    0.031% <= 0.103 milliseconds (cumulative count 31)
    0.034% <= 0.207 milliseconds (cumulative count 34)
    0.035% <= 0.303 milliseconds (cumulative count 35)
    0.093% <= 0.407 milliseconds (cumulative count 93)
    0.584% <= 0.503 milliseconds (cumulative count 584)
    1.964% <= 0.607 milliseconds (cumulative count 1964)
    3.297% <= 0.703 milliseconds (cumulative count 3297)
    6.156% <= 0.807 milliseconds (cumulative count 6156)
    12.099% <= 0.903 milliseconds (cumulative count 12099)
    36.996% <= 1.007 milliseconds (cumulative count 36996)
    76.874% <= 1.103 milliseconds (cumulative count 76874)
    89.792% <= 1.207 milliseconds (cumulative count 89792)
    92.764% <= 1.303 milliseconds (cumulative count 92764)
    94.274% <= 1.407 milliseconds (cumulative count 94274)
    94.979% <= 1.503 milliseconds (cumulative count 94979)
    98.025% <= 1.607 milliseconds (cumulative count 98025)
    99.021% <= 1.703 milliseconds (cumulative count 99021)
    99.293% <= 1.807 milliseconds (cumulative count 99293)
    99.432% <= 1.903 milliseconds (cumulative count 99432)
    99.546% <= 2.007 milliseconds (cumulative count 99546)
    99.630% <= 2.103 milliseconds (cumulative count 99630)
    99.756% <= 3.103 milliseconds (cumulative count 99756)
    99.792% <= 4.103 milliseconds (cumulative count 99792)
    99.800% <= 5.103 milliseconds (cumulative count 99800)
    99.810% <= 9.103 milliseconds (cumulative count 99810)
    99.834% <= 10.103 milliseconds (cumulative count 99834)
    99.961% <= 11.103 milliseconds (cumulative count 99961)
    99.990% <= 12.103 milliseconds (cumulative count 99990)
    100.000% <= 13.103 milliseconds (cumulative count 100000)

    Summary:
    throughput summary: 91074.68 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.070     0.016     1.039     1.511     1.703    12.727
# 纯get
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -t get -c 100 -n 100000
    WARNING: Could not fetch server CONFIG
    ====== GET ======                                                   
    100000 requests completed in 1.11 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.031 milliseconds (cumulative count 3)
    50.000% <= 1.031 milliseconds (cumulative count 50722)
    75.000% <= 1.087 milliseconds (cumulative count 75014)
    87.500% <= 1.159 milliseconds (cumulative count 88273)
    93.750% <= 1.255 milliseconds (cumulative count 93937)
    96.875% <= 1.487 milliseconds (cumulative count 96929)
    98.438% <= 1.839 milliseconds (cumulative count 98449)
    99.219% <= 2.423 milliseconds (cumulative count 99227)
    99.609% <= 5.015 milliseconds (cumulative count 99610)
    99.805% <= 9.391 milliseconds (cumulative count 99805)
    99.902% <= 10.575 milliseconds (cumulative count 99903)
    99.951% <= 10.735 milliseconds (cumulative count 99955)
    99.976% <= 10.823 milliseconds (cumulative count 99980)
    99.988% <= 11.455 milliseconds (cumulative count 99988)
    99.994% <= 13.319 milliseconds (cumulative count 99994)
    99.997% <= 14.791 milliseconds (cumulative count 99997)
    99.998% <= 15.671 milliseconds (cumulative count 99999)
    99.999% <= 15.887 milliseconds (cumulative count 100000)
    100.000% <= 15.887 milliseconds (cumulative count 100000)

    Cumulative distribution of latencies:
    0.036% <= 0.103 milliseconds (cumulative count 36)
    0.042% <= 0.207 milliseconds (cumulative count 42)
    0.048% <= 0.303 milliseconds (cumulative count 48)
    0.081% <= 0.407 milliseconds (cumulative count 81)
    0.275% <= 0.503 milliseconds (cumulative count 275)
    0.839% <= 0.607 milliseconds (cumulative count 839)
    1.948% <= 0.703 milliseconds (cumulative count 1948)
    4.935% <= 0.807 milliseconds (cumulative count 4935)
    11.619% <= 0.903 milliseconds (cumulative count 11619)
    38.591% <= 1.007 milliseconds (cumulative count 38591)
    79.652% <= 1.103 milliseconds (cumulative count 79652)
    92.138% <= 1.207 milliseconds (cumulative count 92138)
    94.905% <= 1.303 milliseconds (cumulative count 94905)
    96.062% <= 1.407 milliseconds (cumulative count 96062)
    97.148% <= 1.503 milliseconds (cumulative count 97148)
    98.111% <= 1.607 milliseconds (cumulative count 98111)
    98.302% <= 1.703 milliseconds (cumulative count 98302)
    98.411% <= 1.807 milliseconds (cumulative count 98411)
    98.551% <= 1.903 milliseconds (cumulative count 98551)
    98.696% <= 2.007 milliseconds (cumulative count 98696)
    98.830% <= 2.103 milliseconds (cumulative count 98830)
    99.300% <= 3.103 milliseconds (cumulative count 99300)
    99.480% <= 4.103 milliseconds (cumulative count 99480)
    99.615% <= 5.103 milliseconds (cumulative count 99615)
    99.688% <= 6.103 milliseconds (cumulative count 99688)
    99.732% <= 7.103 milliseconds (cumulative count 99732)
    99.778% <= 8.103 milliseconds (cumulative count 99778)
    99.800% <= 9.103 milliseconds (cumulative count 99800)
    99.861% <= 10.103 milliseconds (cumulative count 99861)
    99.986% <= 11.103 milliseconds (cumulative count 99986)
    99.990% <= 12.103 milliseconds (cumulative count 99990)
    99.993% <= 13.103 milliseconds (cumulative count 99993)
    99.996% <= 14.103 milliseconds (cumulative count 99996)
    99.997% <= 15.103 milliseconds (cumulative count 99997)
    100.000% <= 16.103 milliseconds (cumulative count 100000)

    Summary:
    throughput summary: 90252.70 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.079     0.024     1.031     1.311     2.207    15.887

# 加入对象池后的性能
# Pipe
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop -P 4 -q
    WARNING: Could not fetch server CONFIG
    SET: 132766.86 requests per second, p50=2.927 msec                    
    GET: 181851.25 requests per second, p50=2.127 msec                    
    LPUSH: 69974.11 requests per second, p50=3.351 msec                     
    LPOP: 26123.98 requests per second, p50=16.095 msec                    

    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 4 -q
    WARNING: Could not fetch server CONFIG
    SET: 133529.17 requests per second, p50=2.935 msec                    
    GET: 179147.27 requests per second, p50=2.135 msec                    
    LPUSH: 124409.05 requests per second, p50=3.007 msec                    
    RPUSH: 131978.36 requests per second, p50=2.983 msec                    
    LPOP: 118245.24 requests per second, p50=3.223 msec                    
    RPOP: 112752.28 requests per second, p50=3.239 msec                    
    SADD: 109206.08 requests per second, p50=3.215 msec                    

    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 8 -q
    WARNING: Could not fetch server CONFIG
    SET: 169262.02 requests per second, p50=4.663 msec                    
    GET: 234082.41 requests per second, p50=3.159 msec                    
    LPUSH: 155666.25 requests per second, p50=4.775 msec                    
    RPUSH: 158982.52 requests per second, p50=4.847 msec                    
    LPOP: 143225.44 requests per second, p50=5.479 msec                    
    RPOP: 136462.88 requests per second, p50=5.367 msec                    
    SADD: 138888.89 requests per second, p50=4.999 msec                    

    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 16 -q
    WARNING: Could not fetch server CONFIG
    SET: 193423.59 requests per second, p50=8.087 msec                      
    GET: 227894.25 requests per second, p50=5.255 msec                     
    LPUSH: 188964.48 requests per second, p50=8.239 msec                    
    RPUSH: 167897.92 requests per second, p50=8.175 msec                     
    LPOP: 147754.14 requests per second, p50=9.367 msec                      
    RPOP: 172146.67 requests per second, p50=9.087 msec                      
    SADD: 171939.48 requests per second, p50=8.671 msec                     

    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,lpop,rpush,rpop,sadd,srem -P 32 -q
    WARNING: Could not fetch server CONFIG
    SET: 210748.16 requests per second, p50=14.839 msec                     
    GET: 343997.25 requests per second, p50=9.127 msec                      
    LPUSH: 208246.56 requests per second, p50=14.759 msec                     
    RPUSH: 211819.52 requests per second, p50=14.735 msec                     
    LPOP: 203583.05 requests per second, p50=15.559 msec                     
    RPOP: 204750.22 requests per second, p50=15.519 msec                     
    SADD: 209292.59 requests per second, p50=15.215 msec
# 非Pipe
    blue@Plus:~/c_projects/newblue$ redis-benchmark -h 127.0.0.1 -p 6666 -a client123 -c 100 -n 1000000 -t set,get,lpush,rpush,lpop,rpop,sadd,srem
    WARNING: Could not fetch server CONFIG
    ====== SET ======                                                   
    1000000 requests completed in 13.32 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 14)
    50.000% <= 1.263 milliseconds (cumulative count 503101)
    75.000% <= 1.327 milliseconds (cumulative count 750598)
    87.500% <= 1.415 milliseconds (cumulative count 877009)
    93.750% <= 1.551 milliseconds (cumulative count 938385)
    96.875% <= 1.767 milliseconds (cumulative count 969024)
    98.438% <= 1.999 milliseconds (cumulative count 984802)
    99.219% <= 2.255 milliseconds (cumulative count 992258)
    99.609% <= 2.975 milliseconds (cumulative count 996105)
    99.805% <= 6.775 milliseconds (cumulative count 998047)
    99.902% <= 10.247 milliseconds (cumulative count 999027)
    99.951% <= 11.479 milliseconds (cumulative count 999512)
    99.976% <= 11.591 milliseconds (cumulative count 999761)
    99.988% <= 11.671 milliseconds (cumulative count 999881)
    99.994% <= 11.951 milliseconds (cumulative count 999940)
    99.997% <= 12.183 milliseconds (cumulative count 999972)
    99.998% <= 12.479 milliseconds (cumulative count 999985)
    99.999% <= 12.543 milliseconds (cumulative count 999993)
    100.000% <= 12.551 milliseconds (cumulative count 999997)
    100.000% <= 12.567 milliseconds (cumulative count 999999)
    100.000% <= 12.647 milliseconds (cumulative count 1000000)
    100.000% <= 12.647 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.049% <= 0.103 milliseconds (cumulative count 486)
    0.060% <= 0.207 milliseconds (cumulative count 603)
    0.063% <= 0.303 milliseconds (cumulative count 627)
    0.085% <= 0.407 milliseconds (cumulative count 853)
    0.147% <= 0.503 milliseconds (cumulative count 1475)
    0.253% <= 0.607 milliseconds (cumulative count 2527)
    0.405% <= 0.703 milliseconds (cumulative count 4052)
    0.679% <= 0.807 milliseconds (cumulative count 6787)
    1.154% <= 0.903 milliseconds (cumulative count 11541)
    2.465% <= 1.007 milliseconds (cumulative count 24648)
    6.101% <= 1.103 milliseconds (cumulative count 61010)
    22.581% <= 1.207 milliseconds (cumulative count 225807)
    68.136% <= 1.303 milliseconds (cumulative count 681358)
    86.966% <= 1.407 milliseconds (cumulative count 869662)
    92.478% <= 1.503 milliseconds (cumulative count 924780)
    94.972% <= 1.607 milliseconds (cumulative count 949721)
    96.280% <= 1.703 milliseconds (cumulative count 962804)
    97.218% <= 1.807 milliseconds (cumulative count 972182)
    97.904% <= 1.903 milliseconds (cumulative count 979036)
    98.517% <= 2.007 milliseconds (cumulative count 985171)
    98.876% <= 2.103 milliseconds (cumulative count 988761)
    99.632% <= 3.103 milliseconds (cumulative count 996321)
    99.737% <= 4.103 milliseconds (cumulative count 997371)
    99.782% <= 5.103 milliseconds (cumulative count 997819)
    99.796% <= 6.103 milliseconds (cumulative count 997956)
    99.809% <= 7.103 milliseconds (cumulative count 998094)
    99.815% <= 8.103 milliseconds (cumulative count 998149)
    99.821% <= 9.103 milliseconds (cumulative count 998212)
    99.891% <= 10.103 milliseconds (cumulative count 998913)
    99.924% <= 11.103 milliseconds (cumulative count 999236)
    99.997% <= 12.103 milliseconds (cumulative count 999967)
    100.000% <= 13.103 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 75075.08 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.312     0.016     1.263     1.615     2.143    12.647
    ====== GET ======                                                   
    1000000 requests completed in 10.79 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 28)
    50.000% <= 1.039 milliseconds (cumulative count 514187)
    75.000% <= 1.087 milliseconds (cumulative count 776856)
    87.500% <= 1.127 milliseconds (cumulative count 887225)
    93.750% <= 1.183 milliseconds (cumulative count 941276)
    96.875% <= 1.271 milliseconds (cumulative count 969978)
    98.438% <= 1.399 milliseconds (cumulative count 984385)
    99.219% <= 1.583 milliseconds (cumulative count 992339)
    99.609% <= 1.823 milliseconds (cumulative count 996124)
    99.805% <= 2.207 milliseconds (cumulative count 998056)
    99.902% <= 9.087 milliseconds (cumulative count 999024)
    99.951% <= 10.271 milliseconds (cumulative count 999512)
    99.976% <= 11.071 milliseconds (cumulative count 999759)
    99.988% <= 11.303 milliseconds (cumulative count 999878)
    99.994% <= 169.599 milliseconds (cumulative count 999940)
    99.997% <= 171.647 milliseconds (cumulative count 999972)
    99.998% <= 172.671 milliseconds (cumulative count 999988)
    99.999% <= 172.799 milliseconds (cumulative count 999993)
    100.000% <= 173.055 milliseconds (cumulative count 999998)
    100.000% <= 173.183 milliseconds (cumulative count 999999)
    100.000% <= 173.311 milliseconds (cumulative count 1000000)
    100.000% <= 173.311 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.026% <= 0.103 milliseconds (cumulative count 265)
    0.034% <= 0.207 milliseconds (cumulative count 336)
    0.037% <= 0.303 milliseconds (cumulative count 374)
    0.057% <= 0.407 milliseconds (cumulative count 572)
    0.155% <= 0.503 milliseconds (cumulative count 1551)
    0.624% <= 0.607 milliseconds (cumulative count 6235)
    1.635% <= 0.703 milliseconds (cumulative count 16352)
    4.597% <= 0.807 milliseconds (cumulative count 45973)
    10.749% <= 0.903 milliseconds (cumulative count 107495)
    34.331% <= 1.007 milliseconds (cumulative count 343314)
    83.375% <= 1.103 milliseconds (cumulative count 833751)
    95.246% <= 1.207 milliseconds (cumulative count 952460)
    97.514% <= 1.303 milliseconds (cumulative count 975144)
    98.494% <= 1.407 milliseconds (cumulative count 984940)
    98.982% <= 1.503 milliseconds (cumulative count 989818)
    99.284% <= 1.607 milliseconds (cumulative count 992844)
    99.460% <= 1.703 milliseconds (cumulative count 994596)
    99.596% <= 1.807 milliseconds (cumulative count 995963)
    99.678% <= 1.903 milliseconds (cumulative count 996778)
    99.741% <= 2.007 milliseconds (cumulative count 997414)
    99.775% <= 2.103 milliseconds (cumulative count 997746)
    99.882% <= 3.103 milliseconds (cumulative count 998821)
    99.890% <= 4.103 milliseconds (cumulative count 998900)
    99.903% <= 9.103 milliseconds (cumulative count 999028)
    99.943% <= 10.103 milliseconds (cumulative count 999431)
    99.977% <= 11.103 milliseconds (cumulative count 999773)
    99.990% <= 12.103 milliseconds (cumulative count 999900)
    99.990% <= 154.111 milliseconds (cumulative count 999902)
    99.990% <= 156.159 milliseconds (cumulative count 999904)
    99.990% <= 157.183 milliseconds (cumulative count 999905)
    99.991% <= 167.167 milliseconds (cumulative count 999908)
    99.992% <= 168.191 milliseconds (cumulative count 999918)
    99.993% <= 169.215 milliseconds (cumulative count 999933)
    99.995% <= 170.111 milliseconds (cumulative count 999947)
    99.997% <= 171.135 milliseconds (cumulative count 999966)
    99.998% <= 172.159 milliseconds (cumulative count 999980)
    100.000% <= 173.183 milliseconds (cumulative count 999999)
    100.000% <= 174.207 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 92678.41 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.058     0.016     1.039     1.207     1.511   173.311
    ====== LPUSH ======                                                   
    1000000 requests completed in 13.44 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 11)
    50.000% <= 1.303 milliseconds (cumulative count 523754)
    75.000% <= 1.375 milliseconds (cumulative count 766773)
    87.500% <= 1.439 milliseconds (cumulative count 881564)
    93.750% <= 1.511 milliseconds (cumulative count 937584)
    96.875% <= 1.655 milliseconds (cumulative count 969210)
    98.438% <= 1.863 milliseconds (cumulative count 984383)
    99.219% <= 2.087 milliseconds (cumulative count 992231)
    99.609% <= 2.431 milliseconds (cumulative count 996125)
    99.805% <= 2.871 milliseconds (cumulative count 998047)
    99.902% <= 5.671 milliseconds (cumulative count 999024)
    99.951% <= 10.207 milliseconds (cumulative count 999513)
    99.976% <= 10.847 milliseconds (cumulative count 999758)
    99.988% <= 11.607 milliseconds (cumulative count 999878)
    99.994% <= 11.719 milliseconds (cumulative count 999942)
    99.997% <= 11.743 milliseconds (cumulative count 999970)
    99.998% <= 11.759 milliseconds (cumulative count 999988)
    99.999% <= 11.783 milliseconds (cumulative count 999995)
    100.000% <= 11.799 milliseconds (cumulative count 999998)
    100.000% <= 11.807 milliseconds (cumulative count 999999)
    100.000% <= 11.815 milliseconds (cumulative count 1000000)
    100.000% <= 11.815 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.049% <= 0.103 milliseconds (cumulative count 486)
    0.057% <= 0.207 milliseconds (cumulative count 574)
    0.059% <= 0.303 milliseconds (cumulative count 593)
    0.064% <= 0.407 milliseconds (cumulative count 639)
    0.071% <= 0.503 milliseconds (cumulative count 705)
    0.097% <= 0.607 milliseconds (cumulative count 968)
    0.126% <= 0.703 milliseconds (cumulative count 1257)
    0.217% <= 0.807 milliseconds (cumulative count 2169)
    0.379% <= 0.903 milliseconds (cumulative count 3787)
    0.916% <= 1.007 milliseconds (cumulative count 9164)
    2.936% <= 1.103 milliseconds (cumulative count 29364)
    13.543% <= 1.207 milliseconds (cumulative count 135428)
    52.375% <= 1.303 milliseconds (cumulative count 523754)
    83.308% <= 1.407 milliseconds (cumulative count 833083)
    93.401% <= 1.503 milliseconds (cumulative count 934012)
    96.308% <= 1.607 milliseconds (cumulative count 963077)
    97.404% <= 1.703 milliseconds (cumulative count 974042)
    98.144% <= 1.807 milliseconds (cumulative count 981442)
    98.621% <= 1.903 milliseconds (cumulative count 986212)
    99.021% <= 2.007 milliseconds (cumulative count 990209)
    99.246% <= 2.103 milliseconds (cumulative count 992457)
    99.841% <= 3.103 milliseconds (cumulative count 998413)
    99.892% <= 4.103 milliseconds (cumulative count 998917)
    99.901% <= 5.103 milliseconds (cumulative count 999011)
    99.909% <= 6.103 milliseconds (cumulative count 999087)
    99.919% <= 7.103 milliseconds (cumulative count 999187)
    99.920% <= 8.103 milliseconds (cumulative count 999200)
    99.947% <= 10.103 milliseconds (cumulative count 999470)
    99.979% <= 11.103 milliseconds (cumulative count 999790)
    100.000% <= 12.103 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 74382.62 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.329     0.016     1.303     1.551     2.007    11.815
    ====== RPUSH ======                                                   
    1000000 requests completed in 13.60 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 12)
    50.000% <= 1.303 milliseconds (cumulative count 530321)
    75.000% <= 1.359 milliseconds (cumulative count 755200)
    87.500% <= 1.431 milliseconds (cumulative count 878619)
    93.750% <= 1.535 milliseconds (cumulative count 939312)
    96.875% <= 1.743 milliseconds (cumulative count 969111)
    98.438% <= 1.999 milliseconds (cumulative count 984534)
    99.219% <= 2.335 milliseconds (cumulative count 992274)
    99.609% <= 2.831 milliseconds (cumulative count 996125)
    99.805% <= 3.735 milliseconds (cumulative count 998048)
    99.902% <= 5.847 milliseconds (cumulative count 999025)
    99.951% <= 10.247 milliseconds (cumulative count 999512)
    99.976% <= 11.663 milliseconds (cumulative count 999757)
    99.988% <= 13.823 milliseconds (cumulative count 999878)
    99.994% <= 62.975 milliseconds (cumulative count 999939)
    99.997% <= 73.663 milliseconds (cumulative count 999970)
    99.998% <= 83.903 milliseconds (cumulative count 999985)
    99.999% <= 87.679 milliseconds (cumulative count 999993)
    100.000% <= 88.895 milliseconds (cumulative count 999997)
    100.000% <= 89.471 milliseconds (cumulative count 1000000)
    100.000% <= 89.471 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.048% <= 0.103 milliseconds (cumulative count 480)
    0.056% <= 0.207 milliseconds (cumulative count 559)
    0.057% <= 0.303 milliseconds (cumulative count 573)
    0.067% <= 0.407 milliseconds (cumulative count 671)
    0.106% <= 0.503 milliseconds (cumulative count 1064)
    0.172% <= 0.607 milliseconds (cumulative count 1723)
    0.261% <= 0.703 milliseconds (cumulative count 2610)
    0.431% <= 0.807 milliseconds (cumulative count 4306)
    0.693% <= 0.903 milliseconds (cumulative count 6933)
    1.411% <= 1.007 milliseconds (cumulative count 14112)
    3.659% <= 1.103 milliseconds (cumulative count 36590)
    11.922% <= 1.207 milliseconds (cumulative count 119216)
    53.032% <= 1.303 milliseconds (cumulative count 530321)
    85.037% <= 1.407 milliseconds (cumulative count 850371)
    92.811% <= 1.503 milliseconds (cumulative count 928115)
    95.486% <= 1.607 milliseconds (cumulative count 954864)
    96.591% <= 1.703 milliseconds (cumulative count 965910)
    97.338% <= 1.807 milliseconds (cumulative count 973379)
    97.895% <= 1.903 milliseconds (cumulative count 978955)
    98.490% <= 2.007 milliseconds (cumulative count 984901)
    98.835% <= 2.103 milliseconds (cumulative count 988354)
    99.686% <= 3.103 milliseconds (cumulative count 996863)
    99.844% <= 4.103 milliseconds (cumulative count 998436)
    99.882% <= 5.103 milliseconds (cumulative count 998824)
    99.909% <= 6.103 milliseconds (cumulative count 999090)
    99.912% <= 7.103 milliseconds (cumulative count 999124)
    99.913% <= 8.103 milliseconds (cumulative count 999135)
    99.924% <= 9.103 milliseconds (cumulative count 999243)
    99.944% <= 10.103 milliseconds (cumulative count 999443)
    99.967% <= 11.103 milliseconds (cumulative count 999672)
    99.981% <= 12.103 milliseconds (cumulative count 999813)
    99.986% <= 13.103 milliseconds (cumulative count 999864)
    99.989% <= 14.103 milliseconds (cumulative count 999887)
    99.990% <= 15.103 milliseconds (cumulative count 999900)
    99.990% <= 37.119 milliseconds (cumulative count 999902)
    99.990% <= 39.103 milliseconds (cumulative count 999903)
    99.990% <= 40.127 milliseconds (cumulative count 999905)
    99.991% <= 41.119 milliseconds (cumulative count 999906)
    99.991% <= 49.119 milliseconds (cumulative count 999914)
    99.992% <= 50.111 milliseconds (cumulative count 999916)
    99.992% <= 51.103 milliseconds (cumulative count 999919)
    99.992% <= 52.127 milliseconds (cumulative count 999921)
    99.992% <= 53.119 milliseconds (cumulative count 999923)
    99.992% <= 54.111 milliseconds (cumulative count 999924)
    99.993% <= 55.103 milliseconds (cumulative count 999926)
    99.993% <= 56.127 milliseconds (cumulative count 999928)
    99.993% <= 57.119 milliseconds (cumulative count 999930)
    99.993% <= 58.111 milliseconds (cumulative count 999932)
    99.993% <= 59.103 milliseconds (cumulative count 999933)
    99.993% <= 60.127 milliseconds (cumulative count 999934)
    99.994% <= 61.119 milliseconds (cumulative count 999936)
    99.994% <= 62.111 milliseconds (cumulative count 999937)
    99.994% <= 63.103 milliseconds (cumulative count 999940)
    99.994% <= 64.127 milliseconds (cumulative count 999944)
    99.995% <= 65.119 milliseconds (cumulative count 999946)
    99.995% <= 66.111 milliseconds (cumulative count 999950)
    99.995% <= 67.135 milliseconds (cumulative count 999951)
    99.996% <= 68.159 milliseconds (cumulative count 999956)
    99.996% <= 69.119 milliseconds (cumulative count 999958)
    99.996% <= 70.143 milliseconds (cumulative count 999960)
    99.997% <= 71.103 milliseconds (cumulative count 999966)
    99.997% <= 72.127 milliseconds (cumulative count 999968)
    99.997% <= 73.151 milliseconds (cumulative count 999969)
    99.997% <= 74.111 milliseconds (cumulative count 999970)
    99.997% <= 75.135 milliseconds (cumulative count 999972)
    99.997% <= 78.143 milliseconds (cumulative count 999973)
    99.997% <= 79.103 milliseconds (cumulative count 999974)
    99.997% <= 81.151 milliseconds (cumulative count 999975)
    99.998% <= 82.111 milliseconds (cumulative count 999977)
    99.998% <= 83.135 milliseconds (cumulative count 999982)
    99.999% <= 84.159 milliseconds (cumulative count 999986)
    99.999% <= 85.119 milliseconds (cumulative count 999988)
    99.999% <= 86.143 milliseconds (cumulative count 999990)
    99.999% <= 87.103 milliseconds (cumulative count 999992)
    99.999% <= 88.127 milliseconds (cumulative count 999994)
    100.000% <= 89.151 milliseconds (cumulative count 999998)
    100.000% <= 90.111 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 73551.05 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.341     0.016     1.303     1.583     2.191    89.471
    ====== LPOP ======                                                   
    1000000 requests completed in 12.93 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 12)
    50.000% <= 1.223 milliseconds (cumulative count 501224)
    75.000% <= 1.303 milliseconds (cumulative count 753967)
    87.500% <= 1.399 milliseconds (cumulative count 875026)
    93.750% <= 1.551 milliseconds (cumulative count 939227)
    96.875% <= 1.751 milliseconds (cumulative count 969572)
    98.438% <= 1.911 milliseconds (cumulative count 984948)
    99.219% <= 2.047 milliseconds (cumulative count 992269)
    99.609% <= 2.303 milliseconds (cumulative count 996169)
    99.805% <= 3.711 milliseconds (cumulative count 998049)
    99.902% <= 10.783 milliseconds (cumulative count 999025)
    99.951% <= 11.639 milliseconds (cumulative count 999521)
    99.976% <= 11.799 milliseconds (cumulative count 999760)
    99.988% <= 15.687 milliseconds (cumulative count 999878)
    99.994% <= 17.759 milliseconds (cumulative count 999939)
    99.997% <= 18.639 milliseconds (cumulative count 999971)
    99.998% <= 19.711 milliseconds (cumulative count 999986)
    99.999% <= 19.743 milliseconds (cumulative count 999993)
    100.000% <= 19.775 milliseconds (cumulative count 999997)
    100.000% <= 19.791 milliseconds (cumulative count 999999)
    100.000% <= 19.999 milliseconds (cumulative count 1000000)
    100.000% <= 19.999 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.033% <= 0.103 milliseconds (cumulative count 326)
    0.038% <= 0.207 milliseconds (cumulative count 377)
    0.038% <= 0.303 milliseconds (cumulative count 382)
    0.042% <= 0.407 milliseconds (cumulative count 415)
    0.062% <= 0.503 milliseconds (cumulative count 622)
    0.113% <= 0.607 milliseconds (cumulative count 1128)
    0.198% <= 0.703 milliseconds (cumulative count 1984)
    0.416% <= 0.807 milliseconds (cumulative count 4157)
    1.005% <= 0.903 milliseconds (cumulative count 10054)
    2.979% <= 1.007 milliseconds (cumulative count 29795)
    9.779% <= 1.103 milliseconds (cumulative count 97787)
    42.339% <= 1.207 milliseconds (cumulative count 423389)
    75.397% <= 1.303 milliseconds (cumulative count 753967)
    88.120% <= 1.407 milliseconds (cumulative count 881202)
    92.603% <= 1.503 milliseconds (cumulative count 926032)
    95.098% <= 1.607 milliseconds (cumulative count 950981)
    96.443% <= 1.703 milliseconds (cumulative count 964434)
    97.481% <= 1.807 milliseconds (cumulative count 974811)
    98.430% <= 1.903 milliseconds (cumulative count 984303)
    99.058% <= 2.007 milliseconds (cumulative count 990578)
    99.365% <= 2.103 milliseconds (cumulative count 993649)
    99.769% <= 3.103 milliseconds (cumulative count 997687)
    99.812% <= 4.103 milliseconds (cumulative count 998120)
    99.843% <= 5.103 milliseconds (cumulative count 998435)
    99.853% <= 6.103 milliseconds (cumulative count 998533)
    99.855% <= 7.103 milliseconds (cumulative count 998550)
    99.866% <= 8.103 milliseconds (cumulative count 998661)
    99.870% <= 9.103 milliseconds (cumulative count 998696)
    99.879% <= 10.103 milliseconds (cumulative count 998794)
    99.909% <= 11.103 milliseconds (cumulative count 999090)
    99.982% <= 12.103 milliseconds (cumulative count 999816)
    99.984% <= 13.103 milliseconds (cumulative count 999838)
    99.986% <= 14.103 milliseconds (cumulative count 999864)
    99.987% <= 15.103 milliseconds (cumulative count 999874)
    99.988% <= 16.103 milliseconds (cumulative count 999882)
    99.989% <= 17.103 milliseconds (cumulative count 999892)
    99.995% <= 18.111 milliseconds (cumulative count 999950)
    99.997% <= 19.103 milliseconds (cumulative count 999975)
    100.000% <= 20.111 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 77339.52 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.273     0.016     1.223     1.607     1.999    19.999
    ====== RPOP ======                                                    
    1000000 requests completed in 13.68 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 8)
    50.000% <= 1.231 milliseconds (cumulative count 530317)
    75.000% <= 1.327 milliseconds (cumulative count 754961)
    87.500% <= 1.447 milliseconds (cumulative count 877465)
    93.750% <= 1.639 milliseconds (cumulative count 937908)
    96.875% <= 1.863 milliseconds (cumulative count 969412)
    98.438% <= 2.007 milliseconds (cumulative count 984668)
    99.219% <= 2.191 milliseconds (cumulative count 992277)
    99.609% <= 2.615 milliseconds (cumulative count 996106)
    99.805% <= 3.951 milliseconds (cumulative count 998047)
    99.902% <= 6.087 milliseconds (cumulative count 999025)
    99.951% <= 9.711 milliseconds (cumulative count 999513)
    99.976% <= 11.535 milliseconds (cumulative count 999759)
    99.988% <= 12.895 milliseconds (cumulative count 999878)
    99.994% <= 582.655 milliseconds (cumulative count 999939)
    99.997% <= 596.479 milliseconds (cumulative count 999970)
    99.998% <= 605.695 milliseconds (cumulative count 999986)
    99.999% <= 608.255 milliseconds (cumulative count 999994)
    100.000% <= 609.791 milliseconds (cumulative count 1000000)
    100.000% <= 609.791 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.033% <= 0.103 milliseconds (cumulative count 331)
    0.037% <= 0.207 milliseconds (cumulative count 373)
    0.038% <= 0.303 milliseconds (cumulative count 385)
    0.050% <= 0.407 milliseconds (cumulative count 497)
    0.105% <= 0.503 milliseconds (cumulative count 1047)
    0.193% <= 0.607 milliseconds (cumulative count 1930)
    0.343% <= 0.703 milliseconds (cumulative count 3433)
    0.623% <= 0.807 milliseconds (cumulative count 6232)
    1.232% <= 0.903 milliseconds (cumulative count 12317)
    3.240% <= 1.007 milliseconds (cumulative count 32403)
    8.899% <= 1.103 milliseconds (cumulative count 88985)
    42.406% <= 1.207 milliseconds (cumulative count 424055)
    71.670% <= 1.303 milliseconds (cumulative count 716698)
    85.172% <= 1.407 milliseconds (cumulative count 851716)
    90.163% <= 1.503 milliseconds (cumulative count 901633)
    93.137% <= 1.607 milliseconds (cumulative count 931367)
    94.801% <= 1.703 milliseconds (cumulative count 948011)
    96.075% <= 1.807 milliseconds (cumulative count 960749)
    97.471% <= 1.903 milliseconds (cumulative count 974707)
    98.467% <= 2.007 milliseconds (cumulative count 984668)
    99.005% <= 2.103 milliseconds (cumulative count 990047)
    99.722% <= 3.103 milliseconds (cumulative count 997220)
    99.816% <= 4.103 milliseconds (cumulative count 998155)
    99.875% <= 5.103 milliseconds (cumulative count 998755)
    99.903% <= 6.103 milliseconds (cumulative count 999032)
    99.918% <= 7.103 milliseconds (cumulative count 999175)
    99.920% <= 8.103 milliseconds (cumulative count 999198)
    99.931% <= 9.103 milliseconds (cumulative count 999308)
    99.959% <= 10.103 milliseconds (cumulative count 999590)
    99.962% <= 11.103 milliseconds (cumulative count 999621)
    99.986% <= 12.103 milliseconds (cumulative count 999858)
    99.988% <= 13.103 milliseconds (cumulative count 999882)
    99.990% <= 14.103 milliseconds (cumulative count 999896)
    99.990% <= 15.103 milliseconds (cumulative count 999900)
    99.990% <= 553.471 milliseconds (cumulative count 999901)
    99.990% <= 554.495 milliseconds (cumulative count 999904)
    99.991% <= 556.543 milliseconds (cumulative count 999906)
    99.991% <= 557.567 milliseconds (cumulative count 999908)
    99.991% <= 558.591 milliseconds (cumulative count 999909)
    99.991% <= 559.103 milliseconds (cumulative count 999910)
    99.991% <= 560.127 milliseconds (cumulative count 999911)
    99.991% <= 561.151 milliseconds (cumulative count 999912)
    99.991% <= 562.175 milliseconds (cumulative count 999913)
    99.992% <= 563.199 milliseconds (cumulative count 999915)
    99.992% <= 564.223 milliseconds (cumulative count 999916)
    99.992% <= 565.247 milliseconds (cumulative count 999918)
    99.992% <= 566.271 milliseconds (cumulative count 999919)
    99.992% <= 567.295 milliseconds (cumulative count 999920)
    99.992% <= 568.319 milliseconds (cumulative count 999921)
    99.992% <= 569.343 milliseconds (cumulative count 999923)
    99.993% <= 570.367 milliseconds (cumulative count 999926)
    99.993% <= 571.391 milliseconds (cumulative count 999927)
    99.993% <= 572.415 milliseconds (cumulative count 999928)
    99.993% <= 573.439 milliseconds (cumulative count 999929)
    99.993% <= 574.463 milliseconds (cumulative count 999930)
    99.993% <= 575.487 milliseconds (cumulative count 999932)
    99.993% <= 576.511 milliseconds (cumulative count 999934)
    99.993% <= 577.535 milliseconds (cumulative count 999935)
    99.994% <= 579.583 milliseconds (cumulative count 999936)
    99.994% <= 580.607 milliseconds (cumulative count 999937)
    99.994% <= 581.119 milliseconds (cumulative count 999938)
    99.994% <= 583.167 milliseconds (cumulative count 999939)
    99.994% <= 584.191 milliseconds (cumulative count 999941)
    99.994% <= 585.215 milliseconds (cumulative count 999943)
    99.995% <= 586.239 milliseconds (cumulative count 999947)
    99.995% <= 587.263 milliseconds (cumulative count 999948)
    99.995% <= 588.287 milliseconds (cumulative count 999951)
    99.996% <= 589.311 milliseconds (cumulative count 999955)
    99.996% <= 590.335 milliseconds (cumulative count 999956)
    99.996% <= 591.359 milliseconds (cumulative count 999958)
    99.996% <= 592.383 milliseconds (cumulative count 999959)
    99.996% <= 593.407 milliseconds (cumulative count 999961)
    99.996% <= 594.431 milliseconds (cumulative count 999964)
    99.997% <= 595.455 milliseconds (cumulative count 999967)
    99.997% <= 596.479 milliseconds (cumulative count 999970)
    99.997% <= 597.503 milliseconds (cumulative count 999973)
    99.997% <= 598.527 milliseconds (cumulative count 999974)
    99.998% <= 599.551 milliseconds (cumulative count 999976)
    99.998% <= 600.575 milliseconds (cumulative count 999978)
    99.998% <= 601.599 milliseconds (cumulative count 999979)
    99.998% <= 603.135 milliseconds (cumulative count 999980)
    99.998% <= 604.159 milliseconds (cumulative count 999982)
    99.998% <= 605.183 milliseconds (cumulative count 999983)
    99.999% <= 606.207 milliseconds (cumulative count 999988)
    99.999% <= 607.231 milliseconds (cumulative count 999992)
    99.999% <= 608.255 milliseconds (cumulative count 999994)
    100.000% <= 609.279 milliseconds (cumulative count 999996)
    100.000% <= 610.303 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 73099.41 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.344     0.016     1.231     1.719     2.103   609.791
    ====== SADD ======                                                   
    1000000 requests completed in 14.14 seconds
    100 parallel clients
    3 bytes payload
    keep alive: 1
    multi-thread: no

    Latency by percentile distribution:
    0.000% <= 0.023 milliseconds (cumulative count 14)
    50.000% <= 1.335 milliseconds (cumulative count 519768)
    75.000% <= 1.399 milliseconds (cumulative count 762076)
    87.500% <= 1.463 milliseconds (cumulative count 877916)
    93.750% <= 1.559 milliseconds (cumulative count 939989)
    96.875% <= 1.767 milliseconds (cumulative count 969257)
    98.438% <= 2.023 milliseconds (cumulative count 984602)
    99.219% <= 2.247 milliseconds (cumulative count 992434)
    99.609% <= 2.415 milliseconds (cumulative count 996118)
    99.805% <= 3.063 milliseconds (cumulative count 998057)
    99.902% <= 3.807 milliseconds (cumulative count 999029)
    99.951% <= 9.847 milliseconds (cumulative count 999512)
    99.976% <= 11.719 milliseconds (cumulative count 999761)
    99.988% <= 11.967 milliseconds (cumulative count 999878)
    99.994% <= 357.887 milliseconds (cumulative count 999940)
    99.997% <= 364.799 milliseconds (cumulative count 999970)
    99.998% <= 366.847 milliseconds (cumulative count 999988)
    99.999% <= 367.359 milliseconds (cumulative count 999993)
    100.000% <= 367.871 milliseconds (cumulative count 999998)
    100.000% <= 368.127 milliseconds (cumulative count 1000000)
    100.000% <= 368.127 milliseconds (cumulative count 1000000)

    Cumulative distribution of latencies:
    0.043% <= 0.103 milliseconds (cumulative count 428)
    0.050% <= 0.207 milliseconds (cumulative count 504)
    0.052% <= 0.303 milliseconds (cumulative count 521)
    0.057% <= 0.407 milliseconds (cumulative count 569)
    0.067% <= 0.503 milliseconds (cumulative count 666)
    0.124% <= 0.607 milliseconds (cumulative count 1240)
    0.175% <= 0.703 milliseconds (cumulative count 1746)
    0.270% <= 0.807 milliseconds (cumulative count 2698)
    0.406% <= 0.903 milliseconds (cumulative count 4064)
    0.793% <= 1.007 milliseconds (cumulative count 7930)
    2.088% <= 1.103 milliseconds (cumulative count 20879)
    7.083% <= 1.207 milliseconds (cumulative count 70834)
    35.741% <= 1.303 milliseconds (cumulative count 357411)
    78.150% <= 1.407 milliseconds (cumulative count 781495)
    91.482% <= 1.503 milliseconds (cumulative count 914820)
    95.135% <= 1.607 milliseconds (cumulative count 951355)
    96.438% <= 1.703 milliseconds (cumulative count 964382)
    97.174% <= 1.807 milliseconds (cumulative count 971739)
    97.729% <= 1.903 milliseconds (cumulative count 977291)
    98.378% <= 2.007 milliseconds (cumulative count 983781)
    98.772% <= 2.103 milliseconds (cumulative count 987719)
    99.815% <= 3.103 milliseconds (cumulative count 998146)
    99.928% <= 4.103 milliseconds (cumulative count 999283)
    99.942% <= 5.103 milliseconds (cumulative count 999421)
    99.944% <= 6.103 milliseconds (cumulative count 999442)
    99.945% <= 7.103 milliseconds (cumulative count 999453)
    99.950% <= 8.103 milliseconds (cumulative count 999497)
    99.950% <= 9.103 milliseconds (cumulative count 999500)
    99.955% <= 10.103 milliseconds (cumulative count 999554)
    99.960% <= 11.103 milliseconds (cumulative count 999600)
    99.990% <= 12.103 milliseconds (cumulative count 999900)
    99.990% <= 349.183 milliseconds (cumulative count 999902)
    99.991% <= 350.207 milliseconds (cumulative count 999911)
    99.992% <= 351.231 milliseconds (cumulative count 999915)
    99.992% <= 352.255 milliseconds (cumulative count 999923)
    99.992% <= 353.279 milliseconds (cumulative count 999924)
    99.993% <= 354.303 milliseconds (cumulative count 999928)
    99.993% <= 355.327 milliseconds (cumulative count 999931)
    99.993% <= 356.351 milliseconds (cumulative count 999935)
    99.994% <= 357.119 milliseconds (cumulative count 999936)
    99.994% <= 358.143 milliseconds (cumulative count 999940)
    99.994% <= 359.167 milliseconds (cumulative count 999944)
    99.995% <= 360.191 milliseconds (cumulative count 999947)
    99.995% <= 361.215 milliseconds (cumulative count 999951)
    99.996% <= 362.239 milliseconds (cumulative count 999960)
    99.996% <= 363.263 milliseconds (cumulative count 999964)
    99.997% <= 364.287 milliseconds (cumulative count 999967)
    99.997% <= 365.311 milliseconds (cumulative count 999973)
    99.998% <= 366.335 milliseconds (cumulative count 999981)
    99.999% <= 367.103 milliseconds (cumulative count 999992)
    100.000% <= 368.127 milliseconds (cumulative count 1000000)

    Summary:
    throughput summary: 70716.36 requests per second
    latency summary (msec):
            avg       min       p50       p95       p99       max
            1.399     0.016     1.335     1.607     2.183   368.127
# 关闭服务器(比较繁琐)
    blue@Plus:~/c_projects/newblue$ redis-cli -p 6666
    127.0.0.1:6666> auth admin123
    OK
    127.0.0.1:6666> shutdown
    "OK - waiting for clients to disconnect"
    127.0.0.1:6666> exit
## 欢迎提交 Issue 和 Pull Request！
    








