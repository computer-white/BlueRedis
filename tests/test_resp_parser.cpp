#include <iostream>
#include <cassert>
#include "blue/resp_parser.h"

using namespace blue;

void test_simple_string() {
    std::string data = "+OK\r\n";
    auto [val, consumed] = RespValue::parse(data);
    assert(val.type == RespValue::Type::SIMPLE_STRING);
    assert(val.str == "OK");
    assert(consumed == 5);
    std::cout << "✅ test_simple_string passed\n";
}

void test_error() {
    std::string data = "-ERR unknown command\r\n";
    auto [val, consumed] = RespValue::parse(data);
    assert(val.type == RespValue::Type::ERROR);
    assert(val.str == "ERR unknown command");
    assert(consumed == 22);
    std::cout << "✅ test_error passed\n";
}

void test_integer() {
    std::string data = ":1000\r\n";
    auto [val, consumed] = RespValue::parse(data);
    assert(val.type == RespValue::Type::INTEGER);
    assert(val.integ == 1000);
    assert(consumed == 7);
    std::cout << "✅ test_integer passed\n";
}

void test_bulk_string() {
    std::string data = "$5\r\nhello\r\n";
    auto [val, consumed] = RespValue::parse(data);
    assert(val.type == RespValue::Type::BULK_STRING);
    assert(val.str == "hello");
    assert(consumed == 11);
    std::cout << "✅ test_bulk_string passed\n";
}

void test_null_bulk() {
    std::string data = "$-1\r\n";
    auto [val, consumed] = RespValue::parse(data);
    assert(val.type == RespValue::Type::NULL_VAL);
    assert(consumed == 5);
    std::cout << "✅ test_null_bulk passed\n";
}

void test_array() {
    std::string data = "*2\r\n$3\r\nGET\r\n$4\r\nkey1\r\n";
    auto [val, consumed] = RespValue::parse(data);
    assert(val.type == RespValue::Type::ARRAY);
    assert(val.arr.size() == 2);
    assert(val.arr[0].type == RespValue::Type::BULK_STRING);
    assert(val.arr[0].str == "GET");
    assert(val.arr[1].type == RespValue::Type::BULK_STRING);
    assert(val.arr[1].str == "key1");
    assert(consumed == data.size());
    std::cout << "✅ test_array passed\n";
}

void test_incomplete_data() {
    RespStreamParser parser;
    
    // 发送不完整的数据
    parser.feed("*2\r\n$3\r\nGET\r\n$4\r\nke");
    RespValue cmd;
    assert(!parser.next(cmd));
    
    // 发送剩余数据
    parser.feed("y1\r\n");
    assert(parser.next(cmd));
    assert(cmd.arr[1].str == "key1");
    std::cout << "✅ test_incomplete_data passed\n";
}

void test_multiple_commands() {
    RespStreamParser parser;
    
    // 一次发送多个命令
    parser.feed("*1\r\n$4\r\nPING\r\n*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n");
    
    RespValue cmd;
    assert(parser.next(cmd));
    assert(cmd.arr[0].str == "PING");
    
    assert(parser.next(cmd));
    assert(cmd.arr[0].str == "GET");
    assert(cmd.arr[1].str == "key");
    
    assert(!parser.next(cmd));
    std::cout << "✅ test_multiple_commands passed\n";
}

void test_buffer_overflow_protection() {
    RespStreamParser parser(100);  // 小缓冲区用于测试
    
    std::string large_data(200, 'a');
    bool result = parser.feed(large_data);
    assert(!result);  // 应该失败
    
    std::cout << "✅ test_buffer_overflow_protection passed\n";
}

void test_encoding() {
    // 测试编码和解码的一致性
    AutoRespValue original = RespValue::array({
        *RespValue::bulk_string("SET"),
        *RespValue::bulk_string("key"),
        *RespValue::bulk_string("value")
    });
    
    std::string encoded = RespValue::encode(*original);
    auto [decoded, consumed] = RespValue::parse(encoded);
    
    assert(decoded.type == RespValue::Type::ARRAY);
    assert(decoded.arr.size() == 3);
    assert(decoded.arr[0].str == "SET");
    assert(decoded.arr[1].str == "key");
    assert(decoded.arr[2].str == "value");
    
    std::cout << "✅ test_encoding passed\n";
}

int main() {
    std::cout << "Running RESP parser tests...\n\n";
    
    test_simple_string();
    test_error();
    test_integer();
    test_bulk_string();
    test_null_bulk();
    test_array();
    test_incomplete_data();
    test_multiple_commands();
    test_buffer_overflow_protection();
    test_encoding();
    
    std::cout << "\n🎉 All tests passed!\n";
    return 0;
}