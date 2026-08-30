#include <iostream>
#include <yaml-cpp/yaml.h>
#include "blue/config.h"
#include "blue/configinit.h"

int main()
{
    std::cout << "Before:\nAOFDefine_config:\n"
              << blue::redisServerAOFConfig::g_AOFDefine_config_ptr->toString() << std::endl;
    std::cout << "AOFMax_file_buffer_size:"
              << blue::redisServerAOFConfig::g_AOFMaxBufferSize_config_ptr->toString() << std::endl;

    YAML::Node root = YAML::LoadFile("/etc/blueRedis/redis_cof/blueredis.yml");
    blue::Config::LoadFromYAML(root);

    std::cout << "After:\nAOFDefine_config:\n"
              << blue::redisServerAOFConfig::g_AOFDefine_config_ptr->toString() << std::endl;
    std::cout << "AOFMax_file_buffer_size:"
              << blue::redisServerAOFConfig::g_AOFMaxBufferSize_config_ptr->toString() << std::endl;
}