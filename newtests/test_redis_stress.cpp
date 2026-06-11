#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <hiredis/hiredis.h>

void stress_test(int client_id, int operations) {
    redisContext* c = redisConnect("127.0.0.1", 6666);
    if (c == nullptr || c->err) {
        std::cerr << "Connection error\n";
        return;
    }
    
    for (int i = 0; i < operations; ++i) {
        std::string key = "stress_" + std::to_string(client_id) + "_" + std::to_string(i);
        
        // SET
        redisReply* reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), "value");
        freeReplyObject(reply);
        
        // GET
        reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
        freeReplyObject(reply);
        
        // DEL
        reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
        freeReplyObject(reply);
        
        if (i % 1000 == 0) {
            std::cout << "Client " << client_id << " completed " << i << " ops\n";
        }
    }
    
    redisFree(c);
}

int main() {
    const int NUM_CLIENTS = 10;
    const int OPS_PER_CLIENT = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        threads.emplace_back(stress_test, i, OPS_PER_CLIENT);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int total_ops = NUM_CLIENTS * OPS_PER_CLIENT * 3;  // SET, GET, DEL
    std::cout << "\n=== Stress Test Results ===\n";
    std::cout << "Total operations: " << total_ops << "\n";
    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "QPS: " << (total_ops * 1000.0 / duration.count()) << "\n";
    
    return 0;
}