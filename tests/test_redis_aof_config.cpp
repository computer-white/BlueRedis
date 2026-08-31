#include <iostream>
#include <yaml-cpp/yaml.h>
#include "blue/config.h"
#include "blue/configinit.h"

int main()
{
    std::cout << "Before:\nAOFDefine_config:\n"
              << blue::redisServerAOFConfig::g_AOFDefine_config_ptr->toString() << std::endl;

    std::cout << "SlowLogDefine_config:\n"
              << blue::redisServerAOFConfig::g_SlowLogDefine_config_ptr->toString() << std::endl;

    YAML::Node root = YAML::LoadFile("/etc/blueRedis/redis_cof/blueredis.yml");
    blue::Config::LoadFromYAML(root);

    std::cout << "After:\nAOFDefine_config:\n"
              << blue::redisServerAOFConfig::g_AOFDefine_config_ptr->toString() << std::endl;

    std::cout << "SlowLogDefine_config:\n"
              << blue::redisServerAOFConfig::g_SlowLogDefine_config_ptr->toString() << std::endl;
}