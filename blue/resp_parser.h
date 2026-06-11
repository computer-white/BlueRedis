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

    // 流式解析器：支持部分数据
    class RespStreamParser
    {
    public:
        RespStreamParser(size_t max_buffer_size = 1024 * 1024)  // 默认1MB限制
            : max_buffer_size_(max_buffer_size) {}

        // 喂入新数据
        bool feed(std::string_view data);
        
        // 尝试解析出一个完整的命令
        bool next(RespValue& out);
        
        // 检查是否有未完成的数据
        bool hasPending() const { return !buffer_.empty(); }
        
        // 获取缓冲区大小（用于调试）
        size_t bufferSize() const { return buffer_.size(); }
        
        // 重置解析器（用于清理异常连接）
        void reset();

    private:
        std::string buffer_;
        size_t parse_offset_ = 0;
        size_t max_buffer_size_;
    };
}