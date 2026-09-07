/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <iostream>
#include <yaml-cpp/yaml.h>
#include "blue/config.h"
#include "blue/configinit.h"

int main()
{
    std::cout << "Before:\nAOFDefine_config:\n"
              << blue::RedisServerConfig::g_AOFDefine_config_ptr->toString() << std::endl;

    std::cout << "SlowLogDefine_config:\n"
              << blue::RedisServerConfig::g_SlowLogDefine_config_ptr->toString() << std::endl;

    std::cout << "adminPassword:\n"
              << blue::RedisServerConfig::g_admin_password->toString() << std::endl;

    std::cout << "RedisServerConfigDefine:\n"
              << blue::RedisServerConfig::g_RedisServerConfigDefine_config_ptr->toString() << std::endl;

    std::cout << "ReplicationConfig:\n"
              << blue::RedisServerConfig::g_ReplicationDefine_config_ptr->toString() << std::endl;

    YAML::Node root = YAML::LoadFile("/etc/blueRedis/redis_cof/blueredis.yml");
    blue::Config::LoadFromYAML(root);

    std::cout << "After:\nAOFDefine_config:\n"
              << blue::RedisServerConfig::g_AOFDefine_config_ptr->toString() << std::endl;

    std::cout << "SlowLogDefine_config:\n"
              << blue::RedisServerConfig::g_SlowLogDefine_config_ptr->toString() << std::endl;

    std::cout << "adminPassword:\n"
              << blue::RedisServerConfig::g_admin_password->toString() << std::endl;

    std::cout << "RedisServerConfigDefine:\n"
              << blue::RedisServerConfig::g_RedisServerConfigDefine_config_ptr->toString() << std::endl;
    
    std::cout << "ReplicationConfig:\n"
              << blue::RedisServerConfig::g_ReplicationDefine_config_ptr->toString() << std::endl;
}