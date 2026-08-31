/*
 * BlueRedis - Test file
 * Copyright (C) 2026 blue
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <iostream>
#include <yaml-cpp/yaml.h>
#include "blue/log.h"
#include "blue/config.h"
#include "blue/configinit.h"


int main()
{
    std::cout << "Before:\n" << blue::FileoutLogAppender::RotateConfigToString() << std::endl;
    YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/log_rotate.yml");
    blue::Config::LoadFromYAML(root);
    std::cout << "End:\n" << blue::FileoutLogAppender::RotateConfigToString() << std::endl;

}