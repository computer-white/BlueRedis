#include <iostream>
#include <yaml-cpp/yaml.h>
#include "blue/log.h"
#include "blue/config.h"
#include "blue/configinit.h"


int main()
{
    blue::blueIniteConfig();
    std::cout << "Before:\n" << blue::FileoutLogAppender::RotateConfigToString();
    YAML::Node root = YAML::LoadFile("/etc/blueRedis/logs_cof/log_rotate.yml");
    blue::Config::LoadFromYAML(root);
    std::cout << "End:\n" << blue::FileoutLogAppender::RotateConfigToString();

}