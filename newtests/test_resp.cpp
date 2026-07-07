#include "blue/resp_parser.h"
#include <iostream>

int main()
{
    blue::AutoRespValue ping = blue::RespValue::array({*blue::RespValue::bulk_string("PING")});
    std::cout << blue::RespValue::encode(*ping);

    blue::AutoRespValue original = blue::RespValue::array({
        *blue::RespValue::bulk_string("PING")
    });

    std::string encoded = blue::RespValue::encode(*original);
    auto [parsed, consumed] = blue::RespValue::parse(encoded);

    // parsed 应该和 original 结构一样
    // consumed 应该等于 encoded.size()
    std::cout << "consumed: " << consumed << " == " << encoded.size() << std::endl;
    return 0;
}