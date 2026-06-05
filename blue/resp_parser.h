#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace blue
{
    struct RespValue
    {
        enum class Type
        {
            SIMPLE_STRING, // +OK
            ERROR,         // -ERR
            INTEGER,       // :100
            BULK_STRING,   // $5\r\nhello
            ARRAY,         // *2\r\n...
            NULL_VAL       // $-1\r\n
        };

        static std::pair<RespValue, size_t> parse(std::string_view data);
        static std::string encode(const RespValue& val);

        static RespValue simple_string(const std::string &str);
        static RespValue error(const std::string &err);
        static RespValue integer(int64_t val);
        static RespValue bulk_string(const std::string &bulk);
        static RespValue null_bulk();
        static RespValue array(const std::vector<RespValue> &&elems);
        Type type;
        std::string str;
        int64_t integ;
        std::vector<RespValue> arr;
    };
}