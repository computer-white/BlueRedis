#include "blue/tcpServer.h"
#include "blue/io_manager.h"
#include "blue/log.h"
#include "blue/await.h"

using namespace blue;
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

Task<void> test()
{
    auto address = blue::Address::LookupAny("0.0.0.0:8082");

    std::vector<Address::AddressPtr> addresses = {address};
    std::vector<Address::AddressPtr> failed;
    auto* iom = IOManager::GetThis();
    auto tcp = std::make_shared<blue::TcpServer<int>>();
    while(!tcp->bind(addresses,failed))
	{
		co_await sleepFor(2);
	};
	bool ans = co_await tcp->start();
    if (ans)
    {
        BLUE_LOG_INFO(g_logger) << "start 成功";
    }
    while (!tcp->getIsStop())
    {
        co_await sleepFor(2);
    }
}

int main()
{
    IOManager iom(4);
    iom.schedule(test());
    BLUE_LOG_INFO(g_logger) << "wait all";
    iom.wait_all();
}