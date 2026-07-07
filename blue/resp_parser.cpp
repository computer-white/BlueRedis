/**
 * @file resp_parser.cpp
 * @brief RESP协议解析实现
 */
#include "resp_parser.h"
#include "blue/log.h"
#include <stdexcept>

namespace blue
{
    /*

        类型	前缀	格式	示例
        简单字符串	+	+内容\r\n	+OK\r\n
        错误	-	-错误信息\r\n	-ERR unknown\r\n
        整数	:	:数字\r\n	:1000\r\n
        批量字符串	$	$长度\r\n内容\r\n	$5\r\nhello\r\n
        数组	*	*元素个数\r\n元素...	*2\r\n$3\r\nGET\r\n$4\r\nkey\r\n

    */
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    RespValuePool::RespValuePool()
    {
        for (int i = 0; i < 256; ++i)
        {
            m_pool.push(new RespValue());
        }
    }

    RespValuePool::~RespValuePool()
    {
        while (!m_pool.empty())
        {
            delete m_pool.top();
            m_pool.pop();
        }
    }

    RespValue *RespValuePool::acquire()
    {
        if (m_pool.empty())
        {
            return new RespValue();
        }
        auto *ptr = m_pool.top();
        m_pool.pop();
        ptr->reset();
        return ptr;
    }

    void RespValuePool::release(RespValue *ptr)
    {
        if (!ptr)
            return;
        ptr->reset();

        if (m_pool.size() > 2048)
        {
            delete ptr;
            return;
        }
        m_pool.push(ptr);
    }

    RespValuePool &RespValuePool::instance()
    {
        static thread_local RespValuePool pool;
        return pool;
    }

    AutoRespValue::AutoRespValue()
        : m_ptr(RespValuePool::instance().acquire())
    {
    }

    AutoRespValue::AutoRespValue(RespValue *ptr)
        : m_ptr(ptr)
    {
    }

    AutoRespValue::~AutoRespValue()
    {
        if (m_ptr)
        {
            RespValuePool::instance().release(m_ptr);
        }
    }

    AutoRespValue::AutoRespValue(AutoRespValue &&other) noexcept
        : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    AutoRespValue &AutoRespValue::operator=(AutoRespValue &&other) noexcept
    {
        if (this != &other)
        {
            if (m_ptr)
            {
                RespValuePool::instance().release(m_ptr);
            }
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    void RespValue::encodeTo(std::string &out) const
    {
        switch (type)
        {
        case Type::SIMPLE_STRING:
            out.reserve(out.size() + str.size() + 3);
            out.push_back('+');
            out.append(str);
            out.append("\r\n");
            break;
        case Type::BULK_STRING:
            out.reserve(out.size() + str.size() + 20);
            out.push_back('$');
            out.append(std::to_string(str.size()));
            out.append("\r\n");
            out.append(str);
            out.append("\r\n");
            break;
        case Type::INTEGER:
            out.reserve(out.size() + 20);
            out.push_back(':');
            out.append(std::to_string(integ));
            out.append("\r\n");
            break;
        case Type::ERROR:
            out.reserve(out.size() + str.size() + 3);
            out.push_back('-');
            out.append(str);
            out.append("\r\n");
            break;
        case Type::ARRAY:
            out.reserve(out.size() + 20 + arr.size() * 16);
            out.push_back('*');
            out.append(std::to_string(arr.size()));
            out.append("\r\n");
            for (const auto &item : arr)
            {
                item.encodeTo(out);
            }
            break;
        case Type::NULL_VAL:
            out.append("$-1\r\n");
            break;
        default:
            out.append("$-1\r\n");
            break;
        }
    }

    std::string RespValue::encode(const RespValue &val)
    {
        std::string out;
        out.reserve(64);
        val.encodeTo(out);
        return out;
    }

    AutoRespValue RespValue::simple_string(const std::string &s)
    {
        AutoRespValue v;
        v->type = Type::SIMPLE_STRING;
        v->str = s;
        return v;
    }

    AutoRespValue RespValue::error(const std::string &err)
    {
        AutoRespValue v;
        v->type = Type::ERROR;
        v->str = err;
        return v;
    }

    AutoRespValue RespValue::integer(int64_t val)
    {
        AutoRespValue v;
        v->type = Type::INTEGER;
        v->integ = val;
        return v;
    }

    AutoRespValue RespValue::bulk_string(const std::string &bulk)
    {
        AutoRespValue v;
        v->type = Type::BULK_STRING;
        v->str = bulk;
        return v;
    }

    AutoRespValue RespValue::null_bulk()
    {
        AutoRespValue v;
        v->type = Type::NULL_VAL;
        v->str.clear();
        v->arr.clear();
        return v;
    }

    AutoRespValue RespValue::array(std::vector<RespValue> &&elems)
    {
        AutoRespValue v;
        v->type = Type::ARRAY;
        v->arr = std::move(elems);
        return v;
    }

    AutoRespValue RespValue::array(const std::vector<RespValue> &elems)
    {
        AutoRespValue v;
        v->type = Type::ARRAY;
        v->arr = elems;
        return v;
    }

    void RespValue::reserve(size_t size)
    {
        str.reserve(size);
        arr.reserve(size);
    }

    void RespValue::reset()
    {
        type = Type::NIL;
        str.clear();
        arr.clear();
        integ = 0;
    }

    std::pair<RespValue, size_t> RespValue::parse(std::string_view data)
    {
        if (data.empty())
        {
            return {RespValue(), 0};
        }

        RespValue res;
        switch (data[0]) {
        // 简单字符串
        case '+':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return {RespValue(), 0};
            }
            res.str = std::string(data.substr(1, pos - 1));
            res.type = Type::SIMPLE_STRING;
            return {std::move(res), pos + 2};
        }
        // 错误
        case '-':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return std::make_pair(RespValue(), 0);
            }
            res.str = std::string(data.substr(1, pos - 1));
            res.type = Type::ERROR;
            return std::make_pair(res, pos + 2);
        }
        // 整数
        case ':':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return std::make_pair(RespValue(), 0);
            }
            try
            {
                res.integ = std::stoll(std::string(data.substr(1, pos - 1)));
            }
            catch (...)
            {
                return {RespValue(), 0};
            }
            res.type = Type::INTEGER;
            return std::make_pair(res, pos + 2);
        }
        // 批量字符串
        case '$':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return std::make_pair(RespValue(), 0);
            }

            int64_t len;
            try
            {
                len = std::stoll(std::string(data.substr(1, pos - 1)));
            }
            catch (...)
            {
                return {RespValue(), 0};
            }

            if (len == -1)
            {
                res.type = Type::NULL_VAL;
                return {std::move(res), pos + 2};
            }

            if (len < 0)
            {
                return {RespValue(), 0};
            }

            size_t content_start = pos + 2;
            if (content_start + len > data.size())
            {
                return {RespValue(), 0};
            }

            size_t content_end = data.find("\r\n", content_start);
            if (content_end == std::string_view::npos)
            {
                return {RespValue(), 0};
            }

            if (static_cast<int64_t>(content_end - content_start) != len)
            {
                return {RespValue(), 0};
            }

            res.type = Type::BULK_STRING;
            res.str = std::string(data.substr(content_start, len));
            return std::make_pair(std::move(res), content_end + 2);
        }
        // 数组
        case '*':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return {RespValue(), 0};
            }

            int64_t count;
            try
            {
                count = std::stoll(std::string(data.substr(1, pos - 1)));
            }
            catch (...)
            {
                return {RespValue(), 0};
            }
            if (count < 0)
            {
                return {RespValue(), 0};
            }
            res.type = Type::ARRAY;
            res.arr.reserve(static_cast<size_t>(count));
            size_t start_pos = pos + 2;
            size_t total_consumed = start_pos;

            for (int64_t i = 0; i < count; ++i)
            {
                if (start_pos >= data.size())
                {
                    return {RespValue(), 0}; // 数据不完整
                }
                auto [elem, consumed] = parse(data.substr(start_pos));
                if (consumed == 0)
                {
                    return {RespValue(), 0}; // 数据不完整
                }
                res.arr.push_back(std::move(elem));
                start_pos += consumed;
                total_consumed += consumed;
            }
            return std::make_pair(std::move(res), total_consumed);
        }
        default:
            return {RespValue(), 0};
        }
    }

    bool RespStreamParser::feed(std::string_view data)
    {
        if (buffer_.size() + data.size() > max_buffer_size_)
        {
            BLUE_LOG_WARN(g_logger) << "RespStreamParser: buffer overflow, size="
                                    << buffer_.size() + data.size();
            return false;
        }
        buffer_.append(data.data(), data.size());
        return true;
    }

    bool RespStreamParser::next(RespValue &out)
    {
        if (parse_offset_ >= buffer_.size())
        {
            buffer_.clear();
            parse_offset_ = 0;
            return false;
        }

        auto [cmd, consumed] = RespValue::parse(
            std::string_view(buffer_.data() + parse_offset_,
                             buffer_.size() - parse_offset_));

        if (consumed == 0)
        {
            return false;
        }

        parse_offset_ += consumed;
        out = std::move(cmd);

        // 定期清理已解析的数据，避免缓冲区无限增长
        if (parse_offset_ == buffer_.size())
        {
            buffer_.clear();
            parse_offset_ = 0;
        }
        else if (parse_offset_ > 65536) // 每64KB清理一次
        {
            buffer_.erase(0, parse_offset_);
            parse_offset_ = 0;
        }

        return true;
    }
    void RespStreamParser::reset()
    {
        buffer_.clear();
        parse_offset_ = 0;
    }

} // namespace blue