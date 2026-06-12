#include "resp_parser.h"
#include "blue/log.h"
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
    std::pair<RespValue, size_t> RespValue::parse(std::string_view data)
    {
        RespValue res;
        switch (data[0])
        {
        case '+':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return std::make_pair(RespValue(),0);
            }
            res.str = std::string(data.substr(1,pos - 1));
            res.type = Type::SIMPLE_STRING;
            return std::make_pair(res,pos + 2);
        }
        break;
        case '-':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos) 
            {
                return std::make_pair(RespValue(),0);
            }
            res.str = std::string(data.substr(1,pos - 1));
            res.type = Type::ERROR;
            return std::make_pair(res,pos + 2);
        }
        break;
        case ':':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos) 
            {
                return std::make_pair(RespValue(),0);
            }
            res.integ = std::stoll(std::string(data.substr(1, pos - 1)));
            res.type = Type::INTEGER;
            return std::make_pair(res,pos + 2);
        }
        break;
        case '$':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos)
            {
                return std::make_pair(RespValue(),0);
            }
            int64_t len = std::stoll(std::string(data.substr(1, pos - 1)));
            if (len == -1)
            {
                res.type = Type::NULL_VAL;
                return {res, pos + 2};
            }
            size_t content_start = pos + 2;
            size_t content_end = data.find("\r\n", content_start);
            if (content_end == std::string_view::npos) {
                return {RespValue(), 0};
            }
            if (content_end - content_start != len) {
                return {RespValue(), 0};  // 长度不匹配
            }
            res.str = std::string(data.substr(content_start, len));
            res.type = Type::BULK_STRING;
            return {res, content_end + 2};  // 消耗字节数：到 \r\n 结尾
        }
        break;
        case '*':
        {
            size_t pos = data.find("\r\n");
            if (pos == std::string_view::npos) {
                return {RespValue(), 0};
            }
            
            int64_t count;
            try
            {
                count = std::stoll(std::string(data.substr(1, pos - 1)));
            }
            catch(...)
            {
                return {RespValue(),0};
            }
            if (count < 0)
            {
                return {RespValue(), 0};
            }
            
            res.type = Type::ARRAY;
            size_t start_pos = pos + 2;
            size_t total_consumed = start_pos;
            
            for (int64_t i = 0; i < count; ++i)
            {
                if (start_pos >= data.size())
                {
                    return {RespValue(), 0};  // 数据不完整
                }
                auto [elem, consumed] = parse(data.substr(start_pos));
                if (consumed == 0)
                {
                    return {RespValue(), 0};  // 数据不完整
                }
                res.arr.push_back(std::move(elem));
                start_pos += consumed;
                total_consumed += consumed;
            }
            
            return {res, total_consumed};
        }
        break;
        };
        return {RespValue(), 0};
    }

    std::string RespValue::encode(const RespValue &val)
    {
        switch (val.type)
        {
        case Type::SIMPLE_STRING:
        {
            return "+" + val.str + "\r\n";
        }
        break;
        case Type::ERROR:
        {
            return "-" + val.str + "\r\n";
        }
        break;
        case Type::INTEGER:
        {
            return ":" + std::to_string(val.integ) + "\r\n";
        }
        break;
        case Type::BULK_STRING:
        {
            return "$" + std::to_string(val.str.size()) + "\r\n" + val.str + "\r\n";
        }
        break;
        case Type::ARRAY:
        {
            std::string result = "*" + std::to_string(val.arr.size()) + "\r\n";
            for (auto& elem : val.arr)
            {
                result += encode(elem);
            }
            return result;
        }
        break;
        case Type::NULL_VAL:
        {
            return "$-1\r\n";
        }
        break;
        };
        return "";
    }

    RespValue RespValue::simple_string(const std::string &str)
    {
        RespValue res;
        res.str = str;
        res.type = Type::SIMPLE_STRING;
        return res;
    }

    RespValue RespValue::error(const std::string &err)
    {
        RespValue res;
        res.str = err;
        res.type = Type::ERROR;
        return res;
    }

    RespValue RespValue::integer(int64_t val)
    {
        RespValue res;
        res.integ = val;
        res.type = Type::INTEGER;
        return res;
    }

    RespValue RespValue::bulk_string(const std::string &bulk)
    {
        RespValue res;
        res.str = bulk;
        res.type = Type::BULK_STRING;
        return res;
    }

    RespValue RespValue::null_bulk()
    {
        RespValue res;
        res.type = Type::NULL_VAL;
        return res;
    }

    RespValue RespValue::array(const std::vector<RespValue> &&elems)
    {
        RespValue res;
        res.arr = std::move(elems);
        res.type = Type::ARRAY;
        return res;
    }

    bool RespStreamParser::feed(std::string_view data)
    {
        if (buffer_.size() + data.size() > max_buffer_size_)
        {
            return false;
        }
        buffer_.append(data);
        return true;
    }

    bool RespStreamParser::next(RespValue& out)
    {
        if (parse_offset_ >= buffer_.size())
        {
            buffer_.clear();
            parse_offset_ = 0;
            return false;
        }

        auto [cmd, consumed] = RespValue::parse(
            std::string_view(buffer_.data() + parse_offset_, 
                           buffer_.size() - parse_offset_)
        );

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
        else if (parse_offset_ > 65536)  // 每64KB清理一次
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
}