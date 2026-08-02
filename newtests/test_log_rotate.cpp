#include <iostream>
#include "blue/log.h"

int main()
{
    auto g_logger = std::make_shared<blue::Logger>();
    auto appender = std::make_shared<blue::FileoutLogAppender>("blue.log");
    g_logger->addAppender(appender);
    for (int x = 0; x < 100; x++)
    {
        BLUE_LOG_INFO(g_logger) << "hello blue, this is a test file rotate";
    }
    std::cout << g_logger->toyamlString() << std::endl;
    return 0;
}