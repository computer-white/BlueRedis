/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "blue/log.h"
#include "blue/config.h"
#include "blue/configinit.h"

blue::ConfigVar<int>::ConfigVarPtr
    g_int_config_ptr = blue::Config::Lookup<int>("system.port", 8080, "system port");

blue::ConfigVar<float>::ConfigVarPtr
    g_float_config_ptr = blue::Config::Lookup<float>("system.value", 10.2f, "system value");

blue::ConfigVar<std::vector<int>>::ConfigVarPtr
    g_int_vec_config_ptr = blue::Config::Lookup<std::vector<int>>(
        "system.int_vec", std::vector<int>{1, 2}, "system int vec");

blue::ConfigVar<std::list<int>>::ConfigVarPtr
    g_int_list_config_ptr = blue::Config::Lookup<std::list<int>>(
        "system.int_lis", std::list<int>{3, 4}, "system int list");

blue::ConfigVar<std::set<int>>::ConfigVarPtr
    g_int_set_config_ptr = blue::Config::Lookup<std::set<int>>(
        "system.int_set", std::set<int>{5, 6}, "system int set");

blue::ConfigVar<std::unordered_set<int>>::ConfigVarPtr
    g_int_unordered_set_config_ptr = blue::Config::Lookup<std::unordered_set<int>>(
        "system.int_unordered_set", std::unordered_set<int>{7, 8}, "system int unordered_set");

blue::ConfigVar<std::map<std::string, int>>::ConfigVarPtr
    g_int_map_config_ptr = blue::Config::Lookup<std::map<std::string, int>>(
        "system.str_int_map",
        std::map<std::string, int>{{"first", 1}, {"second", 2}, {"third", 3}},
        "system str int map");

blue::ConfigVar<std::unordered_map<std::string, int>>::ConfigVarPtr
    g_int_unordered_map_config_ptr = blue::Config::Lookup<std::unordered_map<std::string, int>>(
        "system.str_int_unordered_map",
        std::unordered_map<std::string, int>{{"k1", 4}, {"k2", 5}, {"k3", 6}},
        "system str int unordered_map");

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
} // namespace blue

blue::ConfigVar<Person>::ConfigVarPtr
    g_person_config_ptr = blue::Config::Lookup<Person>(
        "class.person", Person(), "class person");

blue::ConfigVar<std::map<std::string, Person>>::ConfigVarPtr
    g_person_map_config_ptr = blue::Config::Lookup<std::map<std::string, Person>>(
        "class.map_p1", std::map<std::string, Person>(), "class person map person");

blue::ConfigVar<std::map<std::string, std::vector<Person>>>::ConfigVarPtr
    g_vec_person_map_config_ptr = blue::Config::Lookup<std::map<std::string, std::vector<Person>>>(
        "class.map_vec_p",
        std::map<std::string, std::vector<Person>>(),
        "class person map vec person");

// 测试用的 YAML 内容，覆盖所有已注册的配置项
const char *kTestYaml = R"(
system:
  port: 9090
  value: 3.14
  int_vec: [10, 20, 30]
  int_lis: [40, 50]
  int_set: [60, 70, 80]
  int_unordered_set: [90, 100]
  str_int_map:
    a: 1
    b: 2
  str_int_unordered_map:
    x: 10
    y: 20
    z: 30
    x: 40
class:
  person:
    name: "alice"
    age: 30
    sex: false
  map_p1:
    p1:
      name: "bob"
      age: 25
      sex: true
    p2:
      name: "carol"
      age: 28
      sex: false
  map_vec_p:
    group1:
      - name: "dave"
        age: 40
        sex: true
      - name: "eve"
        age: 35
        sex: false
    group2:
      - name: "frank"
        age: 50
        sex: true
)";

const char *kTestJson = R"({
  "system": {
    "port": 9090,
    "value": 3.14,
    "int_vec": [10, 20, 30],
    "int_lis": [40, 50],
    "int_set": [60, 70, 80],
    "int_unordered_set": [90, 100],
    "str_int_map": { "a": 1, "b": 2 },
    "str_int_unordered_map": { "x": 10, "y": 20, "z": 30 }
  },
  "class": {
    "person": { "name": "alice", "age": 30, "sex": false },
    "map_p1": {
      "p1": { "name": "bob",   "age": 25, "sex": true  },
      "p2": { "name": "carol", "age": 28, "sex": false }
    },
    "map_vec_p": {
      "group1": [
        { "name": "dave", "age": 40, "sex": true  },
        { "name": "eve",  "age": 35, "sex": false }
      ],
      "group2": [
        { "name": "frank", "age": 50, "sex": true }
      ]
    }
  }
})";

std::string writeTempFile(const std::string &name, const std::string &content)
{
    auto dir = std::filesystem::temp_directory_path() / "blue_test";
    std::filesystem::create_directories(dir);
    auto path = (dir / name).string();

    std::ofstream ofs(path, std::ios::trunc | std::ios::binary);
    EXPECT_TRUE(ofs) << "cannot write " << path;
    ofs << content;
    ofs.close();
    return path;
}

// 每个测试套件启动前跑一次：写临时 YAML + 加载 + 挂监听器
class YamlConfigTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // 1. 把测试 YAML 写到临时文件
        yamlPath_ = writeTempFile("test_config.yaml", kTestYaml);

        // 2. 加载一次（所有测试共享这份配置）
        blue::Config::LoadFromYAML(yamlPath_);
    }
public:
    static std::string yamlPath_;
};

class JsonConfigTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        jsonPath_ = writeTempFile("test_config.json", kTestJson);

        blue::Config::LoadFromJson(jsonPath_);
    }
public:
    static std::string jsonPath_;
};

std::string YamlConfigTest::yamlPath_;
std::string JsonConfigTest::jsonPath_;

#define DEFINE_CONFIG_TESTS(Fixture)                                          \
    TEST_F(Fixture, IntConfig)                                                \
    {                                                                         \
        EXPECT_EQ(g_int_config_ptr->getValue(), 9090);                        \
    }                                                                         \
    TEST_F(Fixture, FloatConfig)                                              \
    {                                                                         \
        EXPECT_FLOAT_EQ(g_float_config_ptr->getValue(), 3.14f);               \
    }                                                                         \
    TEST_F(Fixture, VectorConfig)                                             \
    {                                                                         \
        EXPECT_EQ(g_int_vec_config_ptr->getValue(),                           \
                  (std::vector<int>{10, 20, 30}));                            \
    }                                                                         \
    TEST_F(Fixture, ListConfig)                                               \
    {                                                                         \
        EXPECT_EQ(g_int_list_config_ptr->getValue(),                          \
                  (std::list<int>{40, 50}));                                  \
    }                                                                         \
    TEST_F(Fixture, SetConfig)                                                \
    {                                                                         \
        EXPECT_EQ(g_int_set_config_ptr->getValue(),                           \
                  (std::set<int>{60, 70, 80}));                               \
    }                                                                         \
    TEST_F(Fixture, UnorderedSetConfig)                                       \
    {                                                                         \
        const auto &s = g_int_unordered_set_config_ptr->getValue();           \
        EXPECT_EQ(s.size(), 2u);                                              \
        EXPECT_TRUE(s.count(90));                                             \
        EXPECT_TRUE(s.count(100));                                            \
    }                                                                         \
    TEST_F(Fixture, MapConfig)                                                \
    {                                                                         \
        const auto &m = g_int_map_config_ptr->getValue();                     \
        EXPECT_EQ(m.size(), 2u);                                              \
        EXPECT_EQ(m.at("a"), 1);                                              \
        EXPECT_EQ(m.at("b"), 2);                                              \
    }                                                                         \
    TEST_F(Fixture, UnorderedMapConfig)                                       \
    {                                                                         \
        const auto &m = g_int_unordered_map_config_ptr->getValue();           \
        EXPECT_EQ(m.size(), 3u);                                              \
        EXPECT_EQ(m.at("x"), 10);                                             \
        EXPECT_EQ(m.at("y"), 20);                                             \
        EXPECT_EQ(m.at("z"), 30);                                             \
    }                                                                         \
    TEST_F(Fixture, PersonConfig)                                             \
    {                                                                         \
        const auto &p = g_person_config_ptr->getValue();                      \
        EXPECT_EQ(p.m_name, "alice");                                         \
        EXPECT_EQ(p.m_age, 30);                                               \
        EXPECT_EQ(p.m_sex, false);                                            \
    }                                                                         \
    TEST_F(Fixture, PersonMapConfig)                                          \
    {                                                                         \
        const auto &m = g_person_map_config_ptr->getValue();                  \
        ASSERT_EQ(m.size(), 2u);                                              \
        EXPECT_EQ(m.at("p1").m_name, "bob");                                  \
        EXPECT_EQ(m.at("p1").m_age, 25);                                      \
        EXPECT_EQ(m.at("p1").m_sex, true);                                    \
        EXPECT_EQ(m.at("p2").m_name, "carol");                                \
        EXPECT_EQ(m.at("p2").m_age, 28);                                      \
        EXPECT_EQ(m.at("p2").m_sex, false);                                   \
    }                                                                         \
    TEST_F(Fixture, PersonVectorMapConfig)                                    \
    {                                                                         \
        const auto &m = g_vec_person_map_config_ptr->getValue();              \
        ASSERT_EQ(m.size(), 2u);                                              \
        ASSERT_EQ(m.at("group1").size(), 2u);                                 \
        EXPECT_EQ(m.at("group1")[0].m_name, "dave");                          \
        EXPECT_EQ(m.at("group1")[0].m_age, 40);                               \
        EXPECT_EQ(m.at("group1")[0].m_sex, true);                             \
        EXPECT_EQ(m.at("group1")[1].m_name, "eve");                           \
        EXPECT_EQ(m.at("group1")[1].m_sex, false);                            \
        ASSERT_EQ(m.at("group2").size(), 1u);                                 \
        EXPECT_EQ(m.at("group2")[0].m_name, "frank");                         \
    }                                                                         \
    TEST_F(Fixture, PersonRoundTrip)                                          \
    {                                                                         \
        const auto &p = g_person_config_ptr->getValue();                      \
        Person roundTrip = blue::LexicalCast<std::string, Person>{}(          \
            g_person_config_ptr->toString());                                 \
        EXPECT_EQ(roundTrip, p);                                              \
    }

DEFINE_CONFIG_TESTS(YamlConfigTest)
DEFINE_CONFIG_TESTS(JsonConfigTest)

TEST(YamlStructure, IsValid)
{
    YAML::Node root = YAML::LoadFile(YamlConfigTest::yamlPath_);

    ASSERT_TRUE(root.IsMap());
    ASSERT_TRUE(root["system"].IsMap());
    ASSERT_TRUE(root["class"].IsMap());

    EXPECT_EQ(root["system"]["port"].as<int>(), 9090);
    EXPECT_FLOAT_EQ(root["system"]["value"].as<float>(), 3.14f);

    ASSERT_TRUE(root["system"]["int_vec"].IsSequence());
    EXPECT_EQ(root["system"]["int_vec"].size(), 3u);

    ASSERT_TRUE(root["class"]["person"].IsMap());
    EXPECT_EQ(root["class"]["person"]["name"].as<std::string>(), "alice");
    EXPECT_EQ(root["class"]["person"]["age"].as<int>(), 30);
    EXPECT_EQ(root["class"]["person"]["sex"].as<bool>(), false);
}

TEST(ConfigCrossFormat, JsonAndYamlProduceSameResult)
{
    // 先加载 YAML，记录所有配置值
    blue::Config::LoadFromYAML(YamlConfigTest::yamlPath_);

    auto yPort    = g_int_config_ptr->getValue();
    auto yFloat   = g_float_config_ptr->getValue();
    auto yVec     = g_int_vec_config_ptr->getValue();
    auto yList    = g_int_list_config_ptr->getValue();
    auto ySet     = g_int_set_config_ptr->getValue();
    auto yUSet    = g_int_unordered_set_config_ptr->getValue();
    auto yMap     = g_int_map_config_ptr->getValue();
    auto yUMap    = g_int_unordered_map_config_ptr->getValue();
    auto yPerson  = g_person_config_ptr->getValue();
    auto yPMap    = g_person_map_config_ptr->getValue();
    auto yPVecMap = g_vec_person_map_config_ptr->getValue();

    // 再加载 JSON
    blue::Config::LoadFromJson(JsonConfigTest::jsonPath_);

    EXPECT_EQ(g_int_config_ptr->getValue(), yPort);
    EXPECT_FLOAT_EQ(g_float_config_ptr->getValue(), yFloat);
    EXPECT_EQ(g_int_vec_config_ptr->getValue(), yVec);
    EXPECT_EQ(g_int_list_config_ptr->getValue(), yList);
    EXPECT_EQ(g_int_set_config_ptr->getValue(), ySet);
    EXPECT_EQ(g_int_unordered_set_config_ptr->getValue(), yUSet);
    EXPECT_EQ(g_int_map_config_ptr->getValue(), yMap);
    EXPECT_EQ(g_int_unordered_map_config_ptr->getValue(), yUMap);
    EXPECT_EQ(g_person_config_ptr->getValue(), yPerson);
    EXPECT_EQ(g_person_map_config_ptr->getValue(), yPMap);
    EXPECT_EQ(g_vec_person_map_config_ptr->getValue(), yPVecMap);
}

TEST(ConfigListener, TriggeredOnChange)
{
    auto local = blue::Config::Lookup<int>("test.listener_local", 0, "listener test");

    int callCount = 0;
    int lastOld = -1;
    int lastNew = -1;

    local->addListener([&](const int &oldVal, const int &newVal)
                       {
        ++callCount;
        lastOld = oldVal;
        lastNew = newVal; });

    local->setValue(42);

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastOld, 0);
    EXPECT_EQ(lastNew, 42);

    // 相同值不触发
    local->setValue(42);
    EXPECT_EQ(callCount, 1);

    // 再改一次
    local->setValue(100);
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(lastOld, 42);
    EXPECT_EQ(lastNew, 100);
}

TEST(ConfigLoad, MissingFileDoesNotCrash)
{
    EXPECT_THROW(blue::Config::LoadFromYAML("/nonexistent/path/xx.yml"),
                 YAML::BadFile);
    EXPECT_THROW(blue::Config::LoadFromJson("/nonexistent/path/xx.json"),
                 std::invalid_argument);
}

TEST(ConfigLoad, CaseInsensitiveKeys)
{
    const char *mixed = R"(
System:
  PORT: 7070
)";
    // auto filepath = 
    blue::Config::LoadFromYAML(writeTempFile("case.yml", mixed));
    EXPECT_EQ(g_int_config_ptr->getValue(), 7070);
}