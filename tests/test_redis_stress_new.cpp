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
    std::string m_password;
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
    RedisStressTest(const std::string& host, int port, const std::string& password,
                    int client_id, int ops, 
                    std::atomic<int>* total_ops, std::vector<double>* latencies)
        : m_host(host), m_port(port), m_password(password), m_client_id(client_id), 
          m_ops(ops), m_total_ops(total_ops), m_latencies(latencies),
          m_rng(std::random_device{}()),
          m_key_dist(0, 1000),
          m_field_dist(0, 50),
          m_score_dist(0, 100),
          m_member_dist(0, 50) {}
    
    void run() {
        // 每个客户端只连接一次
        redisContext* c = redisConnect(m_host.c_str(), m_port);
        if (c == nullptr || c->err) {
            std::cerr << "Client " << m_client_id << " connection error: " 
                      << (c ? c->errstr : "null") << std::endl;
            return;
        }
        
        // 认证（只认证一次）
        if (!m_password.empty()) {
            redisReply* reply = (redisReply*)redisCommand(c, "AUTH %s", m_password.c_str());
            if (!reply || reply->type == REDIS_REPLY_ERROR) {
                std::cerr << "Client " << m_client_id << " auth failed: " 
                          << (reply ? reply->str : "unknown") << std::endl;
                freeReplyObject(reply);
                redisFree(c);
                return;
            }
            freeReplyObject(reply);
        }
        
        // 每个客户端用不同的数据库避免冲突
        int db = m_client_id % 16;
        redisReply* reply = (redisReply*)redisCommand(c, "SELECT %d", db);
        freeReplyObject(reply);
        
        // 为每个客户端生成唯一的key前缀，避免数据竞争
        std::string key_prefix = "stress_" + std::to_string(m_client_id) + "_";
        
        for (int i = 0; i < m_ops; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // 随机选择命令类型
            int cmd_type = m_rng() % 20;
            int cmd_count = 0;  // 实际执行的Redis命令数
            
            switch (cmd_type) {
                case 0: cmd_count = testString(c, key_prefix, i); break;
                case 1: cmd_count = testHash(c, key_prefix, i); break;
                case 2: cmd_count = testList(c, key_prefix, i); break;
                case 3: cmd_count = testSet(c, key_prefix, i); break;
                case 4: cmd_count = testZSet(c, key_prefix, i); break;
                case 5: cmd_count = testIncr(c, key_prefix, i); break;
                case 6: cmd_count = testExpire(c, key_prefix, i); break;
                default: cmd_count = testString(c, key_prefix, i); break;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            
            // 每条命令都记录延迟（平均分摊）
            if (cmd_count > 0) {
                double per_cmd_latency = static_cast<double>(latency) / cmd_count;
                for (int j = 0; j < cmd_count; ++j) {
                    m_latencies->push_back(per_cmd_latency);
                }
            }
            
            m_total_ops->fetch_add(1);
            
            // 每1000次操作打印进度
            if (i > 0 && i % 1000 == 0) {
                std::cout << "Client " << m_client_id << " completed " 
                          << i << " ops" << std::endl;
            }
        }
        
        redisFree(c);
    }
    
private:
    std::string generateKey(const std::string& prefix, int index) {
        return prefix + std::to_string(m_key_dist(m_rng)) + "_" + std::to_string(index);
    }
    
    // 返回执行的命令数
    int testString(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "str_", idx);
        std::string value = "value_" + std::to_string(m_rng());
        redisReply* reply;
        int count = 0;
        
        // SET
        reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply); count++;
        
        // GET
        reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // APPEND
        reply = (redisReply*)redisCommand(c, "APPEND %s %s", key.c_str(), "_append");
        freeReplyObject(reply); count++;
        
        // STRLEN
        reply = (redisReply*)redisCommand(c, "STRLEN %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // DEL (清理)
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
    
    int testHash(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "hash_", idx);
        std::string field = "field_" + std::to_string(m_field_dist(m_rng));
        std::string value = "value_" + std::to_string(m_rng());
        redisReply* reply;
        int count = 0;
        
        // HSET
        reply = (redisReply*)redisCommand(c, "HSET %s %s %s", 
                                          key.c_str(), field.c_str(), value.c_str());
        freeReplyObject(reply); count++;
        
        // HGET
        reply = (redisReply*)redisCommand(c, "HGET %s %s", key.c_str(), field.c_str());
        freeReplyObject(reply); count++;
        
        // HEXISTS
        reply = (redisReply*)redisCommand(c, "HEXISTS %s %s", key.c_str(), field.c_str());
        freeReplyObject(reply); count++;
        
        // HLEN
        reply = (redisReply*)redisCommand(c, "HLEN %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // HDEL
        reply = (redisReply*)redisCommand(c, "HDEL %s %s", key.c_str(), field.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
    
    int testList(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "list_", idx);
        std::string value = "value_" + std::to_string(m_rng());
        redisReply* reply;
        int count = 0;
        
        // LPUSH
        reply = (redisReply*)redisCommand(c, "LPUSH %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply); count++;
        
        // RPUSH
        reply = (redisReply*)redisCommand(c, "RPUSH %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply); count++;
        
        // LLEN
        reply = (redisReply*)redisCommand(c, "LLEN %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // LPOP
        reply = (redisReply*)redisCommand(c, "LPOP %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // RPOP
        reply = (redisReply*)redisCommand(c, "RPOP %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // LRANGE (只取前10个)
        reply = (redisReply*)redisCommand(c, "LRANGE %s 0 10", key.c_str());
        freeReplyObject(reply); count++;
        
        // DEL 清理
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
    
    int testSet(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "set_", idx);
        std::string member = "member_" + std::to_string(m_member_dist(m_rng));
        redisReply* reply;
        int count = 0;
        
        // SADD
        reply = (redisReply*)redisCommand(c, "SADD %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply); count++;
        
        // SISMEMBER
        reply = (redisReply*)redisCommand(c, "SISMEMBER %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply); count++;
        
        // SCARD
        reply = (redisReply*)redisCommand(c, "SCARD %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // SMEMBERS (如果集合太大会有性能问题，这里限制一下)
        reply = (redisReply*)redisCommand(c, "SMEMBERS %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // SREM
        reply = (redisReply*)redisCommand(c, "SREM %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply); count++;
        
        // DEL 清理
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
    
    int testZSet(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "zset_", idx);
        std::string member = "member_" + std::to_string(m_member_dist(m_rng));
        double score = m_score_dist(m_rng);
        redisReply* reply;
        int count = 0;
        
        // ZADD
        reply = (redisReply*)redisCommand(c, "ZADD %s %f %s", 
                                          key.c_str(), score, member.c_str());
        freeReplyObject(reply); count++;
        
        // ZSCORE
        reply = (redisReply*)redisCommand(c, "ZSCORE %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply); count++;
        
        // ZRANK
        reply = (redisReply*)redisCommand(c, "ZRANK %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply); count++;
        
        // ZCOUNT
        reply = (redisReply*)redisCommand(c, "ZCOUNT %s %f %f", 
                                          key.c_str(), 0.0, 100.0);
        freeReplyObject(reply); count++;
        
        // ZREM
        reply = (redisReply*)redisCommand(c, "ZREM %s %s", key.c_str(), member.c_str());
        freeReplyObject(reply); count++;
        
        // DEL 清理
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
    
    int testIncr(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "incr_", idx);
        int incr = m_rng() % 10 + 1;
        redisReply* reply;
        int count = 0;
        
        // INCR
        reply = (redisReply*)redisCommand(c, "INCR %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // INCRBY
        reply = (redisReply*)redisCommand(c, "INCRBY %s %d", key.c_str(), incr);
        freeReplyObject(reply); count++;
        
        // GET
        reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // DEL
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
    
    int testExpire(redisContext* c, const std::string& key_prefix, int idx) {
        std::string key = generateKey(key_prefix + "expire_", idx);
        std::string value = "value_" + std::to_string(m_rng());
        redisReply* reply;
        int count = 0;
        
        // SET
        reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
        freeReplyObject(reply); count++;
        
        // EXPIRE
        reply = (redisReply*)redisCommand(c, "EXPIRE %s 10", key.c_str());
        freeReplyObject(reply); count++;
        
        // TTL
        reply = (redisReply*)redisCommand(c, "TTL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // PERSIST
        reply = (redisReply*)redisCommand(c, "PERSIST %s", key.c_str());
        freeReplyObject(reply); count++;
        
        // DEL
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply); count++;
        
        return count;
    }
};

int main(int argc, char* argv[]) {
    // 参数解析
    int num_clients = 10;
    int ops_per_client = 10000;
    std::string host = "127.0.0.1";
    int port = 6666;
    std::string password = "client123";  // 修正密码
    
    if (argc > 1) num_clients = std::atoi(argv[1]);
    if (argc > 2) ops_per_client = std::atoi(argv[2]);
    if (argc > 3) host = argv[3];
    if (argc > 4) port = std::atoi(argv[4]);
    if (argc > 5) password = argv[5];
    
    std::cout << "=== Redis Stress Test ===" << std::endl;
    std::cout << "Host: " << host << ":" << port << std::endl;
    std::cout << "Password: " << (password.empty() ? "none" : "****") << std::endl;
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
            RedisStressTest tester(host, port, password, i, ops_per_client, 
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
        std::cout << std::endl;
        
        // 额外统计：每条命令的平均延迟
        std::cout << "=== Per-Command Estimate ===" << std::endl;
        std::cout << "Each operation executes ~4-6 Redis commands" << std::endl;
        std::cout << "Estimated command QPS: " 
                  << std::fixed << std::setprecision(2) 
                  << (total_ops.load() * 4.5 * 1000.0 / duration.count()) << std::endl;
    }
    
    return 0;
}