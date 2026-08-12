#include "../blue/config.h"
#include "../blue/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
// int -> string string -> int
blue::ConfigVar<int>::ConfigVarPtr
    g_int_config_ptr = blue::Config::Lookup<int>("system.port",
                                                 (int)8080,
                                                 "system port");
// float -> string string -> float
blue::ConfigVar<float>::ConfigVarPtr
    g_float_config_ptr = blue::Config::Lookup<float>("system.value",
                                                     (float)10.2f,
                                                     "system value");
// vector -> string string ->vector
blue::ConfigVar<std::vector<int>>::ConfigVarPtr
    g_int_vec_config_ptr = blue::Config::Lookup<std::vector<int>>("system.int_vec",
                                                                  std::vector<int>{1, 2},
                                                                  "system int vec");
// list -> string string ->list
blue::ConfigVar<std::list<int>>::ConfigVarPtr
    g_int_list_config_ptr = blue::Config::Lookup<std::list<int>>("system.int_lis",
                                                                 std::list<int>{3, 4},
                                                                 "system int list");
// set -> string string ->set
blue::ConfigVar<std::set<int>>::ConfigVarPtr
    g_int_set_config_ptr = blue::Config::Lookup<std::set<int>>("system.int_set",
                                                               std::set<int>{5, 6},
                                                               "system int set");
// unordered_set -> string string -> unordered_set
blue::ConfigVar<std::unordered_set<int>>::ConfigVarPtr
    g_int_unordered_set_config_ptr = blue::Config::Lookup<std::unordered_set<int>>("system.int_unordered_set",
                                                                                   std::unordered_set<int>{7, 8},
                                                                                   "system int unordered_set");
// map<std::string,T> -> string string -> map<std::string,T>
blue::ConfigVar<std::map<std::string, int>>::ConfigVarPtr
    g_int_map_config_ptr = blue::Config::Lookup<std::map<std::string, int>>("system.str_int_map",
                                                                            std::map<std::string, int>{{"first", 1}, {"second", 2}, {"third", 3}},
                                                                            "system str int map");
// unordered_map<std::string,T> -> string string -> unordered_map<std::string,T>
blue::ConfigVar<std::unordered_map<std::string, int>>::ConfigVarPtr
    g_int_unordered_map_config_ptr = blue::Config::Lookup<std::unordered_map<std::string, int>>("system.str_int_unordered_map",
                                                                                                std::unordered_map<std::string, int>{{"k1", 4}, {"k2", 5}, {"k3", 6}},
                                                                                                "system str int unordered_map");

void print_YAML(const YAML::Node &node, int level)
{
    if (node.IsScalar()) // 检测是否是一个标量
    {
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull()) // 检测是否为空
    {
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << std::string(level * 4, ' ')
                                               << " NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap()) // 检测是否是一个映射(map)
    {
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << std::string(level * 4, ' ')
                                                   << it->first << " - " << it->second.Type() << " - " << level;
            print_YAML(it->second, level + 1);
        }
    }
    else if (node.IsSequence()) // 检测是否是一个序列(数组/列表)
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << std::string(level * 4, ' ')
                                                   << i << " - " << node[i].Type() << " - " << level;
            print_YAML(node[i], level + 1);
        }
    }
}

void test_YAML()
{
    YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/test.yml");
    // BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << root;
    print_YAML(root, 0);
}

void test_config()
{
    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "Before : " << g_int_config_ptr->getValue();
    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "before : " << g_float_config_ptr->toString();
#define YY(g_val_map, name, prefix)                                                                              \
    {                                                                                                            \
        auto &map_v = g_val_map->getValue();                                                                     \
        for (auto &[key, value] : map_v)                                                                         \
        {                                                                                                        \
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " " #name "--key: " << key << ",value: " << value; \
        }                                                                                                        \
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " " #name "--YAML-string: \n"                        \
                                               << g_val_map->toString();                                         \
    }
#define XX(g_val, name, prefix)                                                           \
    {                                                                                     \
        auto &v = g_val->getValue();                                                      \
        for (auto &x : v)                                                                 \
        {                                                                                 \
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " " #name "--val: " << x;   \
        }                                                                                 \
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " " #name "--YAML-string: \n" \
                                               << g_val->toString();                      \
    } // Before
    XX(g_int_vec_config_ptr, int_vec, Before);
    XX(g_int_list_config_ptr, int_lis, Before);
    XX(g_int_set_config_ptr, int_set, Before);
    XX(g_int_unordered_set_config_ptr, int_unordered_set, Before);
    YY(g_int_map_config_ptr, str_int_map, Before);
    YY(g_int_unordered_map_config_ptr, str_int_unordered_map, Before);
    YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/test.yml");
    blue::Config::LoadFromYAML(root);
    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "After : " << g_int_config_ptr->getValue();
    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "After : " << g_float_config_ptr->toString();

    // After
    XX(g_int_vec_config_ptr, int_vec, After);
    XX(g_int_list_config_ptr, int_lis, After);
    XX(g_int_set_config_ptr, int_set, After);
    XX(g_int_unordered_set_config_ptr, int_unordered_set, After);
    YY(g_int_map_config_ptr, int_map, After);
    YY(g_int_unordered_map_config_ptr, str_int_unordered_map, After);
}
class Person
{
public:
    std::string m_name = "blue";
    int m_age = 21;
    bool m_sex = true;

    std::string tostring() const
    {
        std::stringstream ss;
        ss << "[Person name = " << m_name
           << " age = " << m_age
           << " sex = " << m_sex << "]";
        return ss.str();
    }
    bool operator==(const Person &rhs) const
    {
        return m_name == rhs.m_name && m_age == rhs.m_age && m_sex == rhs.m_sex;
    }
};

namespace blue
{
    // 特化string -> Person
    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    // 特化Person -> string
    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &val)
        {
            YAML::Node node;
            node["name"] = val.m_name;
            node["age"] = val.m_age;
            node["sex"] = val.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

}
// Person
blue::ConfigVar<Person>::ConfigVarPtr
    g_person_config_ptr = blue::Config::Lookup<Person>("class.person",
                                                       Person(),
                                                       "class person");

// map + Person
blue::ConfigVar<std::map<std::string, Person>>::ConfigVarPtr
    g_person_map_config_ptr = blue::Config::Lookup<std::map<std::string, Person>>("class.map_p1",
                                                                                  std::map<std::string, Person>(),
                                                                                  "class person map person");

// map + vector<Person>
blue::ConfigVar<std::map<std::string, std::vector<Person>>>::ConfigVarPtr
    g_vec_person_map_config_ptr = blue::Config::Lookup<std::map<std::string, std::vector<Person>>>("class.map_vec_p",
                                                                                                   std::map<std::string, std::vector<Person>>(),
                                                                                                   "class person map vec person");
void test_class()
{
#define XX_P(g_val, prefix)                                                             \
    {                                                                                   \
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix                               \
                                               << g_val->getValue().tostring() << "-\n" \
                                               << g_val->toString();                    \
    }
#define XX_P_M(g_val, prefix)                                                                                            \
    {                                                                                                                    \
        auto &m = g_val->getValue();                                                                                     \
        for (auto &[key, value] : m)                                                                                     \
        {                                                                                                                \
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " " << "key : " << key << ",value : " << value.tostring(); \
        }                                                                                                                \
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " YAML-string :\n"                                             \
                                               << g_val->toString();                                                     \
    }
#define XX_V_P_M(g_val, prefix)                                                                                          \
    {                                                                                                                    \
        auto &m = g_val->getValue();                                                                                     \
        for (auto &[key, vec_val] : m)                                                                                   \
        {                                                                                                                \
            for (auto &p : vec_val)                                                                                      \
            {                                                                                                            \
                BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " " << "key : " << key << ",value : " << p.tostring(); \
            }                                                                                                            \
        }                                                                                                                \
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << #prefix " YAML-string :\n"                                             \
                                               << g_val->toString();                                                     \
    }
    g_person_config_ptr->addListener([](const Person &old_val, const Person &new_val)
                                     { BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "old_val : " << old_val.tostring() << " new val : " << new_val.tostring(); });
    XX_P(g_person_config_ptr, Before);
    XX_P_M(g_person_map_config_ptr, Before);
    XX_V_P_M(g_vec_person_map_config_ptr, Before);
    YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/test.yml");
    blue::Config::LoadFromYAML(root);
    XX_P(g_person_config_ptr, After);
    XX_P_M(g_person_map_config_ptr, After);
    XX_V_P_M(g_vec_person_map_config_ptr, After);
}

// 利用pthread_once + pthread_once_t 实现保证配置只被加载一次
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
static blue::Logger::LoggerPtr system_logger = nullptr;

static void LoadYaml()
{
    // 加载配置文件（只执行一次）
    YAML::Node root = YAML::LoadFile(
        "/etc/blueRedis/logs_cof/log.yml");
    blue::Config::LoadFromYAML(root);
    
    // 获取 system logger（只获取一次）
    system_logger = BLUE_LOG_NAME("system");
    
    // 输出初始配置（只输出一次）
    std::cout << "##Before :\n"
              << blue::LoggerMgr::GetInstance()->toyamlString() << std::endl;
    
    // 输出加载后的配置（只输出一次）
    std::cout << "##After :\n"
              << blue::LoggerMgr::GetInstance()->toyamlString() << std::endl;
    
    // 修改 formatter（只执行一次）
    system_logger->setFormatter("%d -- %m%n");
}

void test_log()
{
    // 使用 call_once 确保配置只加载一次
    // static std::once_flag init_flag;
    // static blue::Logger::LoggerPtr system_logger = nullptr;
    
    // // 就是对pthread_once的包装
    // std::call_once(init_flag, []() {
    //     // 加载配置文件（只执行一次）
    //     YAML::Node root = YAML::LoadFile(
    //         "/etc/blueRedis/logs_cof/log.yml");
    //     blue::Config::LoadFromYAML(root);
        
    //     // 获取 system logger（只获取一次）
    //     system_logger = BLUE_LOG_NAME("system");
        
    //     // 输出初始配置（只输出一次）
    //     std::cout << "##Before :\n"
    //               << blue::LoggerMgr::GetInstance()->toyamlString() << std::endl;
        
    //     // 输出加载后的配置（只输出一次）
    //     std::cout << "##After :\n"
    //               << blue::LoggerMgr::GetInstance()->toyamlString() << std::endl;
        
    //     // 修改 formatter（只执行一次）
    //     system_logger->setFormatter("%d -- %m%n");
    // });
    
    // 使用pthread_once
    pthread_once(&init_done,LoadYaml);

    // 多线程并发写日志（每个线程都会执行）
    BLUE_LOG_INFO(system_logger) << "hello system ";
}   
int main(int argc, char *argv[])
{
    // test_YAML();
    // test_config();
    // test_class();
    // test_log();
    std::vector<blue::Mthread::MthreadPtr> threads;
    for (size_t i = 0;i<2;i++)
    {
        blue::Mthread::MthreadPtr new_thread1 = 
        std::make_shared<blue::Mthread>(&test_log,"name_" + std::to_string(i * 2));

        blue::Mthread::MthreadPtr new_thread2 = 
        std::make_shared<blue::Mthread>(&test_log,"name_" + std::to_string(i * 2 + 1));
        threads.push_back(new_thread1);
        threads.push_back(new_thread2);
        
    }
    for (size_t i = 0;i<threads.size();i++)
    {
        threads[i]->join();
    }
    
    blue::Config::Visit([](blue::ConfigVarBase::ConfigVarBasePtr cb){
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "name : " << cb->getName()
                                               << " description : " << cb->getDescription()
                                               << " typename : " << cb->getTypeName()
                                               << " value : " << cb->toString();
});
    return 0;
}






// //  测试线程
// #include "blue/blue.h"
// #include <chrono>
// #include <atomic>
// #include <iomanip>
// #include <cmath>
// #include <thread>
// #include <iostream>
// #include <yaml-cpp/yaml.h>
// // 统计数据
// struct BenchmarkStats {
//     std::atomic<long long> total_logs{0};
//     std::atomic<long long> total_time_us{0};
//     std::atomic<long long> min_latency_us{999999999};
//     std::atomic<long long> max_latency_us{0};
//     std::atomic<long long> error_count{0};
    
//     void record_latency(long long us) {
//         total_logs++;
//         total_time_us += us;
//         // 更新最小/最大
//         long long old_min = min_latency_us.load();
//         while (us < old_min && !min_latency_us.compare_exchange_weak(old_min, us));
//         long long old_max = max_latency_us.load();
//         while (us > old_max && !max_latency_us.compare_exchange_weak(old_max, us));
//     }
    
//     double avg_latency() const {
//         if (total_logs == 0) return 0;
//         return (double)total_time_us.load() / total_logs.load();
//     }
    
//     void print() const {
//         std::cout << "  Total logs: " << total_logs << std::endl;
//         std::cout << "  Min latency: " << min_latency_us.load() << " us" << std::endl;
//         std::cout << "  Max latency: " << max_latency_us.load() << " us" << std::endl;
//         std::cout << "  Avg latency: " << std::fixed << std::setprecision(2) 
//                   << avg_latency() << " us" << std::endl;
//     }
// };

// // 工作线程函数
// void worker_func(int thread_id, int iterations, int log_level, BenchmarkStats& stats) {
//     auto logger = BLUE_LOG_NAME("system");
    
//     for (int i = 0; i < iterations; i++) {
//         auto start = std::chrono::high_resolution_clock::now();
        
//         switch (log_level) {
//             case 0:
//                 BLUE_LOG_DEBUGE(logger) << "[Thread-" << thread_id << "] Iter-" << i 
//                                        << " [" << std::this_thread::get_id() << "]";
//                 break;
//             case 1:
//                 BLUE_LOG_INFO(logger) << "[Thread-" << thread_id << "] Iter-" << i 
//                                       << " [" << std::this_thread::get_id() << "]";
//                 break;
//             case 2:
//                 BLUE_LOG_WARN(logger) << "[Thread-" << thread_id << "] Iter-" << i 
//                                       << " [" << std::this_thread::get_id() << "]";
//                 break;
//             case 3:
//                 BLUE_LOG_ERROR(logger) << "[Thread-" << thread_id << "] Iter-" << i 
//                                        << " [" << std::this_thread::get_id() << "]";
//                 break;
//             default:
//                 BLUE_LOG_INFO(logger) << "Test message from thread " << thread_id;
//         }
        
//         auto end = std::chrono::high_resolution_clock::now();
//         auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//         stats.record_latency(latency);
//     }
// }

// // 测试1: 不同线程数性能对比
// void test_thread_scalability() {
//     std::cout << "\n========== Test 1: Thread Scalability ==========" << std::endl;
//     std::cout << std::setw(10) << "Threads" 
//               << std::setw(15) << "Total Logs" 
//               << std::setw(15) << "Time(ms)" 
//               << std::setw(15) << "QPS" 
//               << std::setw(15) << "Avg Latency(us)" << std::endl;
//     std::cout << std::string(70, '-') << std::endl;
    
//     int iterations = 1000;
//     std::vector<int> thread_counts = {1, 2, 4, 8, 16, 32, 64, 100, 200};
    
//     for (int thread_count : thread_counts) {
//         BenchmarkStats stats;
//         auto start = std::chrono::high_resolution_clock::now();
        
//         std::vector<blue::Mthread::MthreadPtr> threads;
//         for (int i = 0; i < thread_count; i++) {
//             threads.push_back(std::make_shared<blue::Mthread>(
//                 [i, iterations, &stats]() {
//                     worker_func(i, iterations, 1, stats);
//                 },
//                 "worker_" + std::to_string(i)
//             ));
//         }
        
//         for (auto& t : threads) {
//             t->join();
//         }
        
//         auto end = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         long long total_logs = stats.total_logs;
        
//         std::cout << std::setw(10) << thread_count
//                   << std::setw(15) << total_logs
//                   << std::setw(15) << duration
//                   << std::setw(15) << std::fixed << std::setprecision(0) 
//                   << (total_logs * 1000.0 / duration)
//                   << std::setw(15) << std::fixed << std::setprecision(2) 
//                   << stats.avg_latency() << std::endl;
//     }
// }

// // 测试2: 不同日志大小性能
// void test_log_size() {
//     std::cout << "\n========== Test 2: Log Size Impact ==========" << std::endl;
//     std::cout << std::setw(15) << "Log Size(bytes)" 
//               << std::setw(15) << "Total Logs" 
//               << std::setw(15) << "Time(ms)" 
//               << std::setw(15) << "QPS" 
//               << std::setw(15) << "MB/s" << std::endl;
//     std::cout << std::string(75, '-') << std::endl;
    
//     int thread_count = 100;
//     int iterations = 500;
    
//     std::vector<int> log_sizes = {128, 256, 512, 1024};
    
//     for (int size : log_sizes) {
//         std::string padding(size, 'X');
//         BenchmarkStats stats;
//         auto logger = BLUE_LOG_NAME("system");
//         auto start = std::chrono::high_resolution_clock::now();
        
//         std::vector<blue::Mthread::MthreadPtr> threads;
//         for (int i = 0; i < thread_count; i++) {
//             threads.push_back(std::make_shared<blue::Mthread>(
//                 [i, iterations, &stats, &padding, logger]() {
//                     for (int j = 0; j < iterations; j++) {
//                         auto t_start = std::chrono::high_resolution_clock::now();
//                         BLUE_LOG_INFO(logger) << "[" << padding << "]";
//                         auto t_end = std::chrono::high_resolution_clock::now();
//                         stats.record_latency(
//                             std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count()
//                         );
//                     }
//                 },
//                 "size_worker_" + std::to_string(i)
//             ));
//         }
        
//         for (auto& t : threads) t->join();
        
//         auto end = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         long long total_logs = stats.total_logs;
//         long long total_bytes = total_logs * (size + 50); // 加上格式开销
        
//         std::cout << std::setw(15) << size
//                   << std::setw(15) << total_logs
//                   << std::setw(15) << duration
//                   << std::setw(15) << std::fixed << std::setprecision(0)
//                   << (total_logs * 1000.0 / duration)
//                   << std::setw(15) << std::fixed << std::setprecision(2)
//                   << (total_bytes * 1000.0 / duration / 1024 / 1024) << std::endl;
//     }
// }

// // 测试3: 长时间稳定性测试
// void test_stability() {
//     std::cout << "\n========== Test 3: Stability Test ==========" << std::endl;
    
//     int thread_count = 50;
//     int duration_seconds = 10;
//     int iterations_per_second = 1000;
    
//     BenchmarkStats stats;
//     std::atomic<bool> running{true};
    
//     std::cout << "Running " << thread_count << " threads for " << duration_seconds 
//               << " seconds..." << std::endl;
    
//     auto start_time = std::chrono::steady_clock::now();
    
//     std::vector<blue::Mthread::MthreadPtr> threads;
//     for (int i = 0; i < thread_count; i++) {
//         threads.push_back(std::make_shared<blue::Mthread>(
//             [i, iterations_per_second, &running, &stats]() {
//                 auto logger = BLUE_LOG_NAME("system");
//                 int iter = 0;
//                 while (running) {
//                     auto start = std::chrono::high_resolution_clock::now();
//                     BLUE_LOG_INFO(logger) << "[Stability] Thread-" << i << " iter-" << iter++;
//                     auto end = std::chrono::high_resolution_clock::now();
//                     stats.record_latency(
//                         std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
//                     );
//                     std::this_thread::sleep_for(std::chrono::microseconds(1000000 / iterations_per_second));
//                 }
//             },
//             "stable_" + std::to_string(i)
//         ));
//     }
    
//     std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
//     running = false;
    
//     for (auto& t : threads) t->join();
    
//     auto end_time = std::chrono::steady_clock::now();
//     auto actual_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
//     std::cout << "\nStability Test Results (" << actual_duration << " seconds):" << std::endl;
//     stats.print();
//     std::cout << "  Throughput: " << std::fixed << std::setprecision(0)
//               << (stats.total_logs / actual_duration) << " logs/sec" << std::endl;
// }

// // 测试4: 混合日志级别性能
// void test_mixed_levels_performance() {
//     std::cout << "\n========== Test 4: Mixed Log Levels ==========" << std::endl;
    
//     int thread_count = 100;
//     int iterations = 1000;
    
//     std::cout << "Threads: " << thread_count << ", Iterations: " << iterations << std::endl;
    
//     for (int level_threshold = 0; level_threshold <= 3; level_threshold++) {
//         // 设置日志级别
//         auto logger = BLUE_LOG_NAME("system");
//         logger->setlevel(static_cast<blue::Level>(level_threshold));
        
//         BenchmarkStats stats;
//         auto start = std::chrono::high_resolution_clock::now();
        
//         std::vector<blue::Mthread::MthreadPtr> threads;
//         for (int i = 0; i < thread_count; i++) {
//             threads.push_back(std::make_shared<blue::Mthread>(
//                 [i, iterations, &stats]() {
//                     auto logger = BLUE_LOG_NAME("system");
//                     for (int j = 0; j < iterations; j++) {
//                         auto t_start = std::chrono::high_resolution_clock::now();
//                         BLUE_LOG_DEBUGE(logger) << "DEBUG msg";
//                         BLUE_LOG_INFO(logger) << "INFO msg";
//                         BLUE_LOG_WARN(logger) << "WARN msg";
//                         BLUE_LOG_ERROR(logger) << "ERROR msg";
//                         auto t_end = std::chrono::high_resolution_clock::now();
//                         stats.record_latency(
//                             std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count()
//                         );
//                     }
//                 },
//                 "mixed_" + std::to_string(i)
//             ));
//         }
        
//         for (auto& t : threads) t->join();
        
//         auto end = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
//         std::cout << "\nLevel threshold: " << level_threshold << " (";
//         switch(level_threshold) {
//             case 0: std::cout << "DEBUG"; break;
//             case 1: std::cout << "INFO"; break;
//             case 2: std::cout << "WARN"; break;
//             case 3: std::cout << "ERROR"; break;
//         }
//         std::cout << ")" << std::endl;
//         std::cout << "  Filtered logs: " << stats.total_logs << "/" 
//                   << (thread_count * iterations * 4) << std::endl;
//         std::cout << "  Time: " << duration << " ms" << std::endl;
//         std::cout << "  QPS: " << std::fixed << std::setprecision(0)
//                   << (stats.total_logs * 1000.0 / duration) << " logs/sec" << std::endl;
//     }
// }

// // 测试5: CPU 核心利用率测试
// void test_cpu_utilization() {
//     std::cout << "\n========== Test 5: CPU Utilization ==========" << std::endl;
    
//     int thread_count = std::thread::hardware_concurrency();
//     int iterations = 10000;
    
//     std::cout << "Hardware concurrency: " << thread_count << " cores" << std::endl;
//     std::cout << "Running with " << thread_count << " threads, " << iterations << " iterations each" << std::endl;
    
//     BenchmarkStats stats;
//     auto start = std::chrono::high_resolution_clock::now();
    
//     std::vector<blue::Mthread::MthreadPtr> threads;
//     for (int i = 0; i < thread_count; i++) {
//         threads.push_back(std::make_shared<blue::Mthread>(
//             [i, iterations, &stats]() {
//                 worker_func(i, iterations, 1, stats);
//             },
//             "cpu_" + std::to_string(i)
//         ));
//     }
    
//     for (auto& t : threads) t->join();
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
//     std::cout << "\nResults:" << std::endl;
//     stats.print();
//     std::cout << "  Total time: " << duration << " ms" << std::endl;
//     std::cout << "  Throughput: " << std::fixed << std::setprecision(0)
//               << (stats.total_logs * 1000.0 / duration) << " logs/sec" << std::endl;
//     std::cout << "  Logs per core: " << (stats.total_logs / thread_count) << std::endl;
// }

// int main(int argc, char* argv[]) {
//     std::cout << "========================================" << std::endl;
//     std::cout << "   Log System Performance Benchmark" << std::endl;
//     std::cout << "========================================" << std::endl;
    
//     // 确保配置已加载
//     YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/log.yml");
//     blue::Config::LoadFromYAML(root);
    
//     try {
//         // 运行所有测试
//         test_thread_scalability();
//         test_log_size();
//         test_stability();
//         test_mixed_levels_performance();
//         test_cpu_utilization();
        
//         std::cout << "\n========================================" << std::endl;
//         std::cout << "   Benchmark Completed!" << std::endl;
//         std::cout << "========================================" << std::endl;
        
//     } catch (const std::exception& e) {
//         std::cerr << "Benchmark failed: " << e.what() << std::endl;
//         return 1;
//     }
    
//     return 0;
// }