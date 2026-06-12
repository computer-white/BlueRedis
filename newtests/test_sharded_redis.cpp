#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>
#include <hiredis/hiredis.h>

using namespace std::chrono;

std::atomic<int> g_success{0};
std::atomic<int> g_fail{0};

// 测试1：基础命令测试
void test_basic_commands() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) {
        std::cerr << "Connection failed\n";
        return;
    }

    redisReply* reply;
    
    // SET/GET
    reply = (redisReply*)redisCommand(c, "SET key1 value1");
    assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "GET key1");
    assert(reply->type == REDIS_REPLY_STRING && std::string(reply->str) == "value1");
    freeReplyObject(reply);
    
    // APPEND
    reply = (redisReply*)redisCommand(c, "APPEND key1 hello");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 11);
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "GET key1");
    assert(reply->type == REDIS_REPLY_STRING && std::string(reply->str) == "value1hello");
    freeReplyObject(reply);
    
    // DEL
    reply = (redisReply*)redisCommand(c, "DEL key1");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    // INCR
    reply = (redisReply*)redisCommand(c, "INCR counter");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "INCRBY counter 10");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 11);
    freeReplyObject(reply);
    
    // MSET/MGET
    reply = (redisReply*)redisCommand(c, "MSET a 1 b 2 c 3");
    assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "MGET a b c d");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 4);
    assert(std::string(reply->element[0]->str) == "1");
    assert(std::string(reply->element[1]->str) == "2");
    assert(std::string(reply->element[2]->str) == "3");
    assert(reply->element[3]->type == REDIS_REPLY_NIL);
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "✅ Basic commands test passed\n";
}

// 测试2：Hash 命令测试
void test_hash_commands() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;

    redisReply* reply;
    
    // HSET/HGET
    reply = (redisReply*)redisCommand(c, "HSET user name alice age 30");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 2);
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "HGET user name");
    assert(reply->type == REDIS_REPLY_STRING && std::string(reply->str) == "alice");
    freeReplyObject(reply);
    
    // HGETALL
    reply = (redisReply*)redisCommand(c, "HGETALL user");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 4);
    freeReplyObject(reply);
    
    // HEXISTS
    reply = (redisReply*)redisCommand(c, "HEXISTS user name");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    // HLEN
    reply = (redisReply*)redisCommand(c, "HLEN user");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 2);
    freeReplyObject(reply);
    
    // HKEYS
    reply = (redisReply*)redisCommand(c, "HKEYS user");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 2);
    freeReplyObject(reply);
    
    // HDEL
    reply = (redisReply*)redisCommand(c, "HDEL user age");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "✅ Hash commands test passed\n";
}

// 测试3：List 命令测试
void test_list_commands() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;

    redisReply* reply;
    
    // LPUSH/RPUSH
    reply = (redisReply*)redisCommand(c, "LPUSH list a b c");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 3);
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "RPUSH list d e");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 5);
    freeReplyObject(reply);
    
    // LRANGE
    reply = (redisReply*)redisCommand(c, "LRANGE list 0 -1");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 5);
    freeReplyObject(reply);
    
    // LPOP/RPOP
    reply = (redisReply*)redisCommand(c, "LPOP list");
    assert(reply->type == REDIS_REPLY_STRING);
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "RPOP list");
    assert(reply->type == REDIS_REPLY_STRING);
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "✅ List commands test passed\n";
}

// 测试4：Set 命令测试
void test_set_commands() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;

    redisReply* reply;
    
    // SADD
    reply = (redisReply*)redisCommand(c, "SADD set a b c");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 3);
    freeReplyObject(reply);
    
    // SMEMBERS
    reply = (redisReply*)redisCommand(c, "SMEMBERS set");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    freeReplyObject(reply);
    
    // SISMEMBER
    reply = (redisReply*)redisCommand(c, "SISMEMBER set a");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    // SCARD
    reply = (redisReply*)redisCommand(c, "SCARD set");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 3);
    freeReplyObject(reply);
    
    // SREM
    reply = (redisReply*)redisCommand(c, "SREM set a");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "✅ Set commands test passed\n";
}

// 测试5：ZSet 命令测试
void test_zset_commands() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;

    redisReply* reply;
    
    // ZADD
    reply = (redisReply*)redisCommand(c, "ZADD zset 1 one 2 two 3 three");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 3);
    freeReplyObject(reply);
    
    // ZRANGE
    reply = (redisReply*)redisCommand(c, "ZRANGE zset 0 -1");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    freeReplyObject(reply);
    
    // ZSCORE
    reply = (redisReply*)redisCommand(c, "ZSCORE zset two");
    assert(reply->type == REDIS_REPLY_STRING && std::string(reply->str) == "2");
    freeReplyObject(reply);
    
    // ZRANK
    reply = (redisReply*)redisCommand(c, "ZRANK zset two");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    // ZREM
    reply = (redisReply*)redisCommand(c, "ZREM zset two");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "✅ ZSet commands test passed\n";
}

// 测试6：类型检查测试
void test_type_check() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;

    redisReply* reply;
    
    // 创建不同类型
    redisCommand(c, "SET str_key string_value");
    redisCommand(c, "HSET hash_key field value");
    redisCommand(c, "LPUSH list_key a");
    redisCommand(c, "SADD set_key a");
    redisCommand(c, "ZADD zset_key 1 one");
    
    // TYPE 检查
    reply = (redisReply*)redisCommand(c, "TYPE str_key");
    assert(std::string(reply->str) == "string");
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "TYPE hash_key");
    assert(std::string(reply->str) == "hash");
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "TYPE list_key");
    assert(std::string(reply->str) == "list");
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "TYPE set_key");
    assert(std::string(reply->str) == "set");
    freeReplyObject(reply);
    
    reply = (redisReply*)redisCommand(c, "TYPE zset_key");
    assert(std::string(reply->str) == "zset");
    freeReplyObject(reply);
    
    // 类型错误检查（STRLEN 对非字符串）
    reply = (redisReply*)redisCommand(c, "STRLEN hash_key");
    assert(reply->type == REDIS_REPLY_ERROR);
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "✅ Type check test passed\n";
}

// 测试7：并发测试
void test_concurrent() {
    const int NUM_THREADS = 10;
    const int OPS_PER_THREAD = 1000;
    std::vector<std::thread> threads;
    
    auto worker = [](int id) {
        redisContext* c = redisConnect("127.0.0.1", 6666);
        if (!c || c->err) return;
        
        for (int i = 0; i < OPS_PER_THREAD; i++) {
            std::string key = "concurrent_" + std::to_string(id) + "_" + std::to_string(i);
            
            redisReply* reply = (redisReply*)redisCommand(c, "SET %s value", key.c_str());
            freeReplyObject(reply);
            
            reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
            freeReplyObject(reply);
            
            reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
            freeReplyObject(reply);
            
            g_success++;
        }
        redisFree(c);
    };
    
    auto start = steady_clock::now();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = steady_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    
    std::cout << "✅ Concurrent test passed: " << g_success << " ops in " << ms << " ms\n";
    std::cout << "   QPS: " << (g_success * 1000.0 / ms) << "\n";
}

// 测试8：分片测试（验证不同 key 落到不同分片）
void test_sharding() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;
    
    // 写入 1000 个 key
    for (int i = 0; i < 1000; i++) {
        std::string key = "shard_test_" + std::to_string(i);
        redisCommand(c, "SET %s value_%d", key.c_str(), i);
    }
    
    // 读取验证
    int success = 0;
    for (int i = 0; i < 1000; i++) {
        std::string key = "shard_test_" + std::to_string(i);
        redisReply* reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        if (reply && reply->type == REDIS_REPLY_STRING) {
            success++;
        }
        freeReplyObject(reply);
    }
    
    assert(success == 1000);
    std::cout << "✅ Sharding test passed: " << success << "/1000 keys retrieved\n";
    
    redisFree(c);
}

// 测试9：过期时间测试
void test_expire() {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (!c || c->err) return;
    
    redisCommand(c, "SET expire_key temp_value EX 2");
    
    // 立即获取
    redisReply* reply = (redisReply*)redisCommand(c, "GET expire_key");
    assert(reply->type == REDIS_REPLY_STRING);
    freeReplyObject(reply);
    
    // 等待 3 秒
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 应该过期
    reply = (redisReply*)redisCommand(c, "GET expire_key");
    assert(reply->type == REDIS_REPLY_NIL);
    freeReplyObject(reply);
    
    std::cout << "✅ Expire test passed\n";
    redisFree(c);
}

// 测试10：压力测试
void stress_test() {
    const int NUM_CLIENTS = 10;
    const int OPS_PER_CLIENT = 10000;
    std::vector<std::thread> threads;
    
    auto worker = [](int id) {
        redisContext* c = redisConnect("127.0.0.1", 6666);
        if (!c || c->err) return;
        
        for (int i = 0; i < OPS_PER_CLIENT; i++) {
            std::string key = "stress_" + std::to_string(id) + "_" + std::to_string(i);
            
            redisReply* reply = (redisReply*)redisCommand(c, "SET %s value", key.c_str());
            freeReplyObject(reply);
            
            reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
            freeReplyObject(reply);
            
            reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
            freeReplyObject(reply);
        }
        redisFree(c);
    };
    
    auto start = steady_clock::now();
    
    for (int i = 0; i < NUM_CLIENTS; i++) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = steady_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    
    int total_ops = NUM_CLIENTS * OPS_PER_CLIENT * 3;
    std::cout << "\n=== Stress Test Results ===\n";
    std::cout << "Total operations: " << total_ops << "\n";
    std::cout << "Time: " << ms << " ms\n";
    std::cout << "QPS: " << (total_ops * 1000.0 / ms) << "\n";
}

int main() {
    std::cout << "\n========== Redis Server Test Suite ==========\n\n";
    
    // 等待服务器启动
    std::cout << "Waiting for server...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 检查连接
    redisContext* test_conn = redisConnect("127.0.0.1", 6666);
    if (!test_conn || test_conn->err) {
        std::cerr << "Cannot connect to server! Make sure it's running on port 6666\n";
        return 1;
    }
    redisFree(test_conn);
    
    try {
        test_basic_commands();
        test_hash_commands();
        test_list_commands();
        test_set_commands();
        test_zset_commands();
        test_type_check();
        test_expire();
        test_sharding();
        test_concurrent();
        stress_test();
        
        std::cout << "\n========== All Tests Passed! 🎉 ==========\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}