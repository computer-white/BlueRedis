/**
 * @file resp_parser.h
 * @brief 对于RESP协议命令的格式化处理和解析
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.6.25
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>

namespace blue
{
    struct RespValue;
    class RespValuePool
    {
    public:
        RespValuePool();
        ~RespValuePool();

        RespValue *acquire();
        void release(RespValue *ptr);

        static RespValuePool &instance();

    private:
        std::stack<RespValue *> m_pool;
    };

    class AutoRespValue
    {
    public:
        AutoRespValue();
        explicit AutoRespValue(RespValue *ptr);
        ~AutoRespValue();

        AutoRespValue(const AutoRespValue &) = delete;
        AutoRespValue &operator=(const AutoRespValue &) = delete;

        AutoRespValue(AutoRespValue &&other) noexcept;
        AutoRespValue &operator=(AutoRespValue &&other) noexcept;

        RespValue *operator->() { return m_ptr; }
        const RespValue *operator->() const { return m_ptr; }
        RespValue &operator*() { return *m_ptr; }
        const RespValue &operator*() const { return *m_ptr; }
        RespValue *get() { return m_ptr; }
        const RespValue *get() const { return m_ptr; }

        operator RespValue *() { return m_ptr; }
        operator const RespValue *() const { return m_ptr; }

    private:
        RespValue *m_ptr = nullptr;
    };

    struct RespValue
    {
        enum class Type
        {
            SIMPLE_STRING, // +OK
            ERROR,         // -ERR
            INTEGER,       // :100
            BULK_STRING,   // $5\r\nhello
            ARRAY,         // *2\r\n...
            NULL_VAL,      // $-1\r\n
            NIL
        };

        RespValue() = default;
        RespValue(const RespValue &) = default;
        RespValue(RespValue &&) noexcept = default;
        RespValue &operator=(const RespValue &) = default;
        RespValue &operator=(RespValue &&) noexcept = default;

        static std::pair<RespValue, size_t> parse(std::string_view data);
        static std::string encode(const RespValue &val);

        void encodeTo(std::string &out) const;

        // ========== 静态工厂方法 ==========
        static AutoRespValue simple_string(const std::string &s);
        static AutoRespValue error(const std::string &err);
        static AutoRespValue integer(int64_t val);
        static AutoRespValue bulk_string(const std::string &bulk);
        static AutoRespValue null_bulk();
        static AutoRespValue array(std::vector<RespValue> &&elems);
        static AutoRespValue array(const std::vector<RespValue> &elems);

        operator AutoRespValue()
        {
            AutoRespValue v;
            *v = *this;
            return v;
        }

        void reserve(size_t size);
        void reset();

        // ========== 成员变量 ==========
        Type type = Type::NIL;
        std::string str;
        int64_t integ = 0;
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

} // namespace blue