#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <hiredis/hiredis.h>

class RedisStressTest {
private:
    std::string m_host;
    int m_port;
    int m_client_id;
    int m_ops;
    std::atomic<int>* m_total_ops;
    std::vector<double>* m_latencies;
    
    // 随机数生成器
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_key_dist;
    std::uniform_int_distribution<int> m_field_dist;
    std::uniform_int_distribution<int> m_score_dist;
    std::uniform_int_distribution<int> m_member_dist;
    
public:
    RedisStressTest(const std::string& host, int port, int client_id, int ops, 
                    std::atomic<int>* total_ops, std::vector<double>* latencies)
        : m_host(host), m_port(port), m_client_id(client_id), m_ops(ops),
          m_total_ops(total_ops), m_latencies(latencies),
          m_rng(std::random_device{}()),
          m_key_dist(0, 1000),
          m_field_dist(0, 50),
          m_score_dist(0, 100),
          m_member_dist(0, 50) {}
    
    void run() {
        redisContext* c = redisConnect(m_host.c_str(), m_port);
        if (c == nullptr || c->err) {
            std::cerr << "Client " << m_client_id << " connection error: " 
                      << (c ? c->errstr : "null") << std::endl;
            return;
        }
        
        // 认证
        redisReply* reply = (redisReply*)redisCommand(c, "AUTH admin123");
        freeReplyObject(reply);
        
        // 选择数据库（每个客户端用不同的数据库避免冲突）
        int db = m_client_id % 16;
        reply = (redisReply*)redisCommand(c, "SELECT %d", db);
        freeReplyObject(reply);
        
        for (int i = 0; i < m_ops; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // 随机选择命令类型
            int cmd_type = m_rng() % 20;  // 20种命令
            
            switch (cmd_type) {
                case 0:  // SET
                case 1:  // GET
                case 2:  // DEL
                    testString(c);
                    break;
                case 3:  // HSET
                case 4:  // HGET
                case 5:  // HDEL
                    testHash(c);
                    break;
                case 6:  // LPUSH
                case 7:  // RPUSH
                case 8:  // LPOP
                case 9:  // RPOP
                    testList(c);
                    break;
                case 10: // SADD
                case 11: // SREM
                case 12: // SISMEMBER
                    testSet(c);
                    break;
                case 13: // ZADD
                case 14: // ZREM
                case 15: // ZSCORE
                    testZSet(c);
                    break;
                case 16: // INCR
                case 17: // INCRBY
                    testIncr(c);
                    break;
                case 18: // EXPIRE
                case 19: // TTL
                    testExpire(c);
                    break;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            m_latencies->push_back(latency);
            
            m_total_ops->fetch_add(1);
            
            // 每 1000 次操作打印进度
            if (i % 1000 == 0 && i > 0) {
                std::cout << "Client " << m_client_id << " completed " 
                          << i << " ops" << std::endl;
            }
        }
        
        redisFree(c);
    }
    
private:
    std::string generateKey(const std::string& prefix) {
        return prefix + "_c" + std::to_string(m_client_id) + 
               "_" + std::to_string(m_key_dist(m_rng));
    }
    
    void testString(redisContext* c) {
        std::string key = generateKey("str");
        std::string value = "value_" + std::to_string(m_rng());
        
        redisReply* reply;
        
        // SET
        reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply);
        
        // GET
        reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply);
        
        // APPEND
        reply = (redisReply*)redisCommand(c, "APPEND %s %s", key.c_str(), "_append");
        freeReplyObject(reply);
        
        // STRLEN
        reply = (redisReply*)redisCommand(c, "STRLEN %s", key.c_str());
        freeReplyObject(reply);
        
        // DEL (清理)
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply);
    }
    
    void testHash(redisContext* c) {
        std::string key = generateKey("hash");
        std::string field = "field_" + std::to_string(m_field_dist(m_rng));
        std::string value = "value_" + std::to_string(m_rng());
        
        redisReply* reply;
        
        // HSET
        reply = (redisReply*)redisCommand(c, "HSET %s %s %s", 
                                          key.c_str(), field.c_str(), value.c_str());
        freeReplyObject(reply);
        
        // HGET
        reply = (redisReply*)redisCommand(c, "HGET %s %s", key.c_str(), field.c_str());
        freeReplyObject(reply);
        
        // HEXISTS
        reply = (redisReply*)redisCommand(c, "HEXISTS %s %s", key.c_str(), field.c_str());
        freeReplyObject(reply);
        
        // HLEN
        reply = (redisReply*)redisCommand(c, "HLEN %s", key.c_str());
        freeReplyObject(reply);
        
        // HDEL
        reply = (redisReply*)redisCommand(c, "HDEL %s %s", key.c_str(), field.c_str());
        freeReplyObject(reply);
    }
    
    void testList(redisContext* c) {
        std::string key = generateKey("list");
        std::string value = "value_" + std::to_string(m_rng());
        
        redisReply* reply;
        
        // LPUSH
        reply = (redisReply*)redisCommand(c, "LPUSH %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply);
        
        // RPUSH
        reply = (redisReply*)redisCommand(c, "RPUSH %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply);
        
        // LLEN
        reply = (redisReply*)redisCommand(c, "LLEN %s", key.c_str());
        freeReplyObject(reply);
        
        // LPOP
        reply = (redisReply*)redisCommand(c, "LPOP %s", key.c_str());
        freeReplyObject(reply);
        
        // RPOP
        reply = (redisReply*)redisCommand(c, "RPOP %s", key.c_str());
        freeReplyObject(reply);
        
        // LRANGE
        reply = (redisReply*)redisCommand(c, "LRANGE %s 0 -1", key.c_str());
        freeReplyObject(reply);
    }
    
    void testSet(redisContext* c) {
        std::string key = generateKey("set");
        std::string member = "member_" + std::to_string(m_member_dist(m_rng));
        
        redisReply* reply;
        
        // SADD
        reply = (redisReply*)redisCommand(c, "SADD %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply);
        
        // SISMEMBER
        reply = (redisReply*)redisCommand(c, "SISMEMBER %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply);
        
        // SCARD
        reply = (redisReply*)redisCommand(c, "SCARD %s", key.c_str());
        freeReplyObject(reply);
        
        // SMEMBERS
        reply = (redisReply*)redisCommand(c, "SMEMBERS %s", key.c_str());
        freeReplyObject(reply);
        
        // SREM
        reply = (redisReply*)redisCommand(c, "SREM %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply);
    }
    
    void testZSet(redisContext* c) {
        std::string key = generateKey("zset");
        std::string member = "member_" + std::to_string(m_member_dist(m_rng));
        double score = m_score_dist(m_rng);
        
        redisReply* reply;
        
        // ZADD
        reply = (redisReply*)redisCommand(c, "ZADD %s %f %s", 
                                          key.c_str(), score, member.c_str());
        freeReplyObject(reply);
        
        // ZSCORE
        reply = (redisReply*)redisCommand(c, "ZSCORE %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply);
        
        // ZRANK
        reply = (redisReply*)redisCommand(c, "ZRANK %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply);
        
        // ZCOUNT
        reply = (redisReply*)redisCommand(c, "ZCOUNT %s %f %f", 
                                          key.c_str(), 0.0, 100.0);
        freeReplyObject(reply);
        
        // ZREM
        reply = (redisReply*)redisCommand(c, "ZREM %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply);
    }
    
    void testIncr(redisContext* c) {
        std::string key = generateKey("incr");
        int incr = m_rng() % 10 + 1;
        
        redisReply* reply;
        
        // INCR
        reply = (redisReply*)redisCommand(c, "INCR %s", key.c_str());
        freeReplyObject(reply);
        
        // INCRBY
        reply = (redisReply*)redisCommand(c, "INCRBY %s %d", key.c_str(), incr);
        freeReplyObject(reply);
        
        // GET
        reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply);
        
        // DEL
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply);
    }
    
    void testExpire(redisContext* c) {
        std::string key = generateKey("expire");
        std::string value = "value_" + std::to_string(m_rng());
        
        redisReply* reply;
        
        // SET
        reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply);
        
        // EXPIRE
        reply = (redisReply*)redisCommand(c, "EXPIRE %s 10", key.c_str());
        freeReplyObject(reply);
        
        // TTL
        reply = (redisReply*)redisCommand(c, "TTL %s", key.c_str());
        freeReplyObject(reply);
        
        // PERSIST
        reply = (redisReply*)redisCommand(c, "PERSIST %s", key.c_str());
        freeReplyObject(reply);
        
        // DEL
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply);
    }
};

int main(int argc, char* argv[]) {
    // 参数解析
    int num_clients = 10;
    int ops_per_client = 10000;
    std::string host = "127.0.0.1";
    int port = 6666;
    
    if (argc > 1) num_clients = std::atoi(argv[1]);
    if (argc > 2) ops_per_client = std::atoi(argv[2]);
    if (argc > 3) host = argv[3];
    if (argc > 4) port = std::atoi(argv[4]);
    
    std::cout << "=== Redis Stress Test ===" << std::endl;
    std::cout << "Host: " << host << ":" << port << std::endl;
    std::cout << "Clients: " << num_clients << std::endl;
    std::cout << "Ops per client: " << ops_per_client << std::endl;
    std::cout << "Total ops: " << num_clients * ops_per_client << std::endl;
    std::cout << "===========================" << std::endl << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::atomic<int> total_ops{0};
    std::vector<std::thread> threads;
    std::vector<std::vector<double>> all_latencies(num_clients);
    
    for (int i = 0; i < num_clients; ++i) {
        threads.emplace_back([&, i]() {
            RedisStressTest tester(host, port, i, ops_per_client, 
                                   &total_ops, &all_latencies[i]);
            tester.run();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 统计延迟
    std::vector<double> all_lat;
    for (const auto& lat : all_latencies) {
        all_lat.insert(all_lat.end(), lat.begin(), lat.end());
    }
    
    if (!all_lat.empty()) {
        std::sort(all_lat.begin(), all_lat.end());
        
        double p50 = all_lat[all_lat.size() * 50 / 100];
        double p95 = all_lat[all_lat.size() * 95 / 100];
        double p99 = all_lat[all_lat.size() * 99 / 100];
        double avg = std::accumulate(all_lat.begin(), all_lat.end(), 0.0) / all_lat.size();
        
        std::cout << "\n=== Results ===" << std::endl;
        std::cout << "Total operations: " << total_ops.load() << std::endl;
        std::cout << "Time: " << duration.count() << " ms" << std::endl;
        std::cout << "QPS: " << std::fixed << std::setprecision(2) 
                  << (total_ops.load() * 1000.0 / duration.count()) << std::endl;
        std::cout << std::endl;
        std::cout << "=== Latency (microseconds) ===" << std::endl;
        std::cout << "Average: " << std::setprecision(2) << avg << " μs" << std::endl;
        std::cout << "P50: " << p50 << " μs" << std::endl;
        std::cout << "P95: " << p95 << " μs" << std::endl;
        std::cout << "P99: " << p99 << " μs" << std::endl;
    }
    
    return 0;
}