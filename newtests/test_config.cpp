#include <yaml-cpp/yaml.h>
#include <iostream>
#include "blue/config.h"
#include "blue/log.h"
#include "blue/configinit.h"
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
    // 输出初始配置（只输出一次）
    std::cout << "##Before :\n"
              << blue::LoggerMgr::GetInstance()->toyamlString() << std::endl;

    // 加载配置文件（只执行一次）
    YAML::Node root = YAML::LoadFile(
        "/etc/blueRedis/logs_cof/log.yml");
    blue::Config::LoadFromYAML(root);
    
    // 获取 system logger（只获取一次）
    system_logger = BLUE_LOG_NAME("system");
    
    // 输出加载后的配置（只输出一次）
    std::cout << "##After :\n"
              << blue::LoggerMgr::GetInstance()->toyamlString() << std::endl;
    
    // 修改 formatter（只执行一次）
    system_logger->setFormatter("%d -- %m%n");
}

int main(int argc, char *argv[])
{
    blue::blueIniteConfig();
    test_YAML();
    test_config();
    test_class();
    return 0;
}