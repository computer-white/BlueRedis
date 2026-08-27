#include <iostream>
#include <yaml-cpp/yaml.h>
#include "blue/log.h"
#include "blue/config.h"


int main()
{
    std::cout << "Before:\n" << blue::FileoutLogAppender::RotateConfigToString();
    YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/log_rotate.yml");
    blue::Config::LoadFromYAML(root);
    std::cout << "End:\n" << blue::FileoutLogAppender::RotateConfigToString();

}