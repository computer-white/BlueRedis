#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "http/http.h"
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

// http 模块
namespace blue
{
    namespace http
    {
        namespace boostios = boost::iostreams;

        static std::string SpaceToUnderLine(const char *tem)
        {
            std::string res(tem);
            if (res.empty())
            {
                return res;
            }
            for (auto &ch : res)
            {
                if (ch == ' ')
                {
                    ch = '_';
                }
            }
            return res;
        }

        static std::string UnderLineToSapce(const char *tem)
        {
            std::string res(tem);
            if (res.empty())
            {
                return res;
            }
            for (auto &ch : res)
            {
                if (ch == '_')
                {
                    ch = ' ';
                }
            }
            return res;
        }
        static const char *s_methods[] = {
#define XX(num, name, string) #string,
            BLUE_HTTP_METHOD_MAP(XX)
#undef XX
        };

        static const std::unordered_map<std::string, HttpMethod> s_method_map = {
#define XX(num, name, string) {#string, HttpMethod::name},
            BLUE_HTTP_METHOD_MAP(XX)
#undef XX
        };

        static const std::map<const uint32_t, const char *> s_status_map = {
#define XX(num, name, string) {num, #string},
            BLUE_HTTP_STATUS_MAP(XX)
#undef XX
        };

        HttpStatus StringToHttpStatus(const std::string &ht)
        {
            std::string tem = SpaceToUnderLine(ht.c_str());
#define XX(num, name, msg)                  \
    if (strcasecmp(#msg, tem.c_str()) == 0) \
    {                                       \
        return HttpStatus::name;            \
    }

            BLUE_HTTP_STATUS_MAP(XX)
#undef XX
            return HttpStatus::INVAILD_STATUS;
        }

        HttpStatus CharsToHttpStatus(const char *ht)
        {
            std::string tem = SpaceToUnderLine(ht);
#define XX(num, name, msg)             \
    if (strcasecmp(#msg, tem.c_str())) \
    {                                  \
        return HttpStatus::name;       \
    }

            BLUE_HTTP_STATUS_MAP(XX)
#undef XX
            return HttpStatus::INVAILD_STATUS;
        }

        HttpMethod StringToHttpMethod(const std::string &ht)
        {
            std::string tem = SpaceToUnderLine(ht.c_str());
#define XX(num, name, string)              \
    if (strcmp(#string, tem.c_str()) == 0) \
    {                                      \
        return HttpMethod::name;           \
    }

            BLUE_HTTP_METHOD_MAP(XX)
#undef XX
            return HttpMethod::INVAILD_METHOD;

            // auto it = s_method_map.find(ht);
            // if (it != s_method_map.end())
            // {
            //     return it->second;
            // }
            // return HttpMethod::INVAILD_METHOD;
        }

        HttpMethod CharsToHttpMethod(const char *ht)
        {
            std::string tem = SpaceToUnderLine(ht);
#define XX(num, name, string)              \
    if (strcmp(#string, tem.c_str()) == 0) \
    {                                      \
        return HttpMethod::name;           \
    }
            BLUE_HTTP_METHOD_MAP(XX);
#undef XX
            return HttpMethod::INVAILD_METHOD;
        }

        std::string HttpMethodToChars(const HttpMethod &ht)
        {
            const uint16_t idx = static_cast<const uint16_t>(ht);
            if (idx >= sizeof(s_methods) / sizeof(s_methods[0]))
            {
                return "<unknow>";
            }
            return UnderLineToSapce(s_methods[idx]);

            // switch (ht)
            // {
            // #define XX(num,name,string) \
            //     case HttpMethod::name:  \
            //         return UnderLineToSapce(#string);
            //     BLUE_HTTP_METHOD_MAP(XX);
            // #undef XX
            //     default :
            //         return "<unknow>";
            // }
        }

        std::string HttpStatusToChars(const HttpStatus &ht)
        {
            // const uint16_t idx = static_cast<const uint16_t>(ht);
            // auto it = s_status_map.find(idx);
            // if (it != s_status_map.end())
            // {
            //     return it->second;
            // }
            // return "<unknow>";

            switch (ht)
            {
#define XX(num, name, msg) \
    case HttpStatus::name: \
        return UnderLineToSapce(#msg);
                BLUE_HTTP_STATUS_MAP(XX);
#undef XX
            default:
                return "<unknow>";
            }
        }

        bool CompareInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const
        {
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }

        HttpRequest::HttpRequest(uint8_t version, bool keepAlive)
            : m_method(HttpMethod::GET),
              m_version(version),
              m_keepAlive(keepAlive),
              m_scheme("http"),
              m_path("/")
        {
        }

        std::string HttpRequest::getHeader(const std::string &key,
                                           const std::string &def) const
        {
            auto it = m_header.find(key);
            return it == m_header.end() ? def : it->second;
        }

        std::string HttpRequest::getParam(const std::string &key,
                                          const std::string &def) const
        {
            auto it = m_param.find(key);
            return it == m_param.end() ? def : it->second;
        }

        std::string HttpRequest::getCookie(const std::string &key,
                                           const std::string &def) const
        {
            auto it = m_cookie.find(key);
            return it == m_cookie.end() ? def : it->second;
        }

        void HttpRequest::setHeader(const std::string &key, const std::string &val)
        {
            m_header[key] = val;
        }

        void HttpRequest::setParam(const std::string &key, const std::string &val)
        {
            m_param[key] = val;
        }

        void HttpRequest::setCookie(const std::string &key, const std::string &val)
        {
            m_cookie[key] = val;
        }

        void HttpRequest::delHeader(const std::string &key)
        {
            m_header.erase(key);
        }

        void HttpRequest::delParam(const std::string &key)
        {
            m_param.erase(key);
        }

        void HttpRequest::delCookie(const std::string &key)
        {
            m_cookie.erase(key);
        }

        bool HttpRequest::hasHeader(const std::string &key, std::string *val)
        {
            auto it = m_header.find(key);
            if (it == m_header.end())
            {
                return false;
            }
            *val = it->second;
            return true;
        }

        bool HttpRequest::hasParam(const std::string &key, std::string *val)
        {
            auto it = m_param.find(key);
            if (it == m_param.end())
            {
                return false;
            }
            *val = it->second;
            return true;
        }

        bool HttpRequest::hasCookie(const std::string &key, std::string *val)
        {
            auto it = m_cookie.find(key);
            if (it == m_cookie.end())
            {
                return false;
            }
            *val = it->second;
            return true;
        }

        std::string HttpRequest::compress(const std::string &data) const
        {
            try
            {
                std::ostringstream result;
                boostios::filtering_ostream out;
                out.push(boostios::gzip_compressor(boostios::gzip::default_compression));
                out.push(result);
                out.write(data.data(), data.size());
                out.reset();
                return result.str();
            }
            catch (const std::exception &e)
            {
                throw std::runtime_error("gzip compress failed: " + std::string(e.what()));
                // BLUE_LOG_ERROR(g_logger) << "http requestParser gzip compress error : " << errno
                //                          << " strerror : " << strerror(errno);
                // return nullptr;
            }
        }

        std::string HttpRequest::decompress(const std::string &data) const
        {
            try
            {
                std::istringstream input(data);
                boostios::filtering_istream in;
                in.push(boostios::gzip_decompressor());
                in.push(input);
                std::ostringstream result;
                result << in.rdbuf();
                return result.str();
            }
            catch (const std::exception &e)
            {
                throw std::runtime_error("gzip decompress failed: " + std::string(e.what()));
                // BLUE_LOG_ERROR(g_logger) << "http requestParser gzip compress error : " << errno
                //                          << " strerror : " << strerror(errno);
                // return nullptr;
            }
        }

        void HttpRequest::reset()
        {
            m_method = HttpMethod::GET;
            m_version = 0x11;
            m_keepAlive = true;
            m_path.clear();
            m_scheme.clear();
            m_query.clear();
            m_fragment.clear();
            m_body.clear();
            m_header.clear();
            m_param.clear();
            m_cookie.clear();
        }

        std::ostream &HttpRequest::dump(std::ostream &os) const
        {
            os << HttpMethodToChars(m_method) << " "
               << m_path
               << (m_query.empty() ? "" : "?")
               << m_query
               << (m_fragment.empty() ? "" : "#")
               << m_fragment
               << " HTTP/"
               << (uint32_t)(m_version >> 4)
               << "."
               << (uint32_t)(m_version & 0x0F)
               << "\r\n";
            for (auto &i : m_header)
            {
                if (strcasecmp(i.first.c_str(), "connection") == 0)
                {
                    continue;
                }
                // if (strcasecmp(i.first.c_str(),"content-length") == 0)
                // {
                //     continue;
                // }
                os << i.first << ": " << i.second << "\r\n";
            }
            os << "Connection: " << (m_keepAlive ? "keep-alive" : "close") << "\r\n";
            os << "\r\n";
            if (!m_body.empty())
            {
                os << m_body;
            }
            // if (!m_body.empty())
            // {
            //     os << "Content-Length: " << std::to_string(m_body.size()) << "\r\n\r\n"
            //        << m_body;
            // }
            // else
            // {
            //     os << "\r\n";
            // }
            return os;
        }

        std::string HttpRequest::toString() const
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }

        std::string HttpRequest::versionToStr() const
        {
            int major = m_version >> 4;   // 高 4 位
            int minor = m_version & 0x0F; // 低 4 位
            return std::to_string(major) + "." + std::to_string(minor);
        }

        HttpResponse::HttpResponse(uint8_t version, bool keepAlive)
            : m_status(HttpStatus::OK),
              m_version(version),
              m_keepAlive(keepAlive)
        {
        }

        std::string HttpResponse::getHeader(const std::string &key, const std::string &def) const
        {
            auto it = m_header.find(key);
            return it == m_header.end() ? def : it->second;
        }

        void HttpResponse::setHeader(const std::string &key, const std::string &val)
        {
            m_header[key] = val;
        }

        void HttpResponse::delHeader(const std::string &key)
        {
            m_header.erase(key);
        }

        std::ostream &HttpResponse::dump(std::ostream &os) const
        {
            os << "HTTP/"
               << (uint32_t)(m_version >> 4)
               << "."
               << (uint32_t)(m_version & 0x0F)
               << " "
               << std::to_string((uint32_t)m_status)
               << " "
               << (m_reason.empty() ? HttpStatusToChars(m_status) : m_reason)
               << "\r\n";

            for (auto &i : m_header)
            {
                if (strcasecmp(i.first.c_str(), "connection") == 0)
                {
                    continue;
                }
                // if (strcasecmp(i.first.c_str(),"content-length") == 0)
                // {
                //     continue;
                // }
                os << i.first << ": " << i.second << "\r\n";
            }
            os << "Connection: " << (m_keepAlive ? "keep-alive" : "close") << "\r\n";
            os << "\r\n";
            if (!m_body.empty())
            {
                os << m_body;
            }
            // if (!m_body.empty())
            // {
            //     os << "Content-Length: " << std::to_string(m_body.size()) << "\r\n\r\n"
            //        << m_body;
            // }
            // else
            // {
            //     os << "\r\n";
            // }
            return os;
        }

        std::string HttpResponse::toString() const
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }

        std::string HttpResponse::versionToStr() const
        {
            int major = m_version >> 4;   // 高 4 位
            int minor = m_version & 0x0F; // 低 4 位
            return std::to_string(major) + "." + std::to_string(minor);
        }

        std::string HttpResponse::compress(const std::string &data) const
        {
            try
            {
                std::ostringstream result;
                boostios::filtering_ostream out;
                out.push(boostios::gzip_compressor(boostios::gzip::default_compression));
                out.push(result);
                out.write(data.data(), data.size());
                out.reset();
                return result.str();
            }
            catch (const std::exception &e)
            {
                throw std::runtime_error("gzip compress failed: " + std::string(e.what()));
                // BLUE_LOG_ERROR(g_logger) << "http requestParser gzip compress error : " << errno
                //                          << " strerror : " << strerror(errno);
                // return nullptr;
            }
        }

        std::string HttpResponse::decompress(const std::string &data) const
        {
            try
            {
                std::istringstream input(data);
                boostios::filtering_istream in;
                in.push(boostios::gzip_decompressor());
                in.push(input);
                std::ostringstream result;
                result << in.rdbuf();
                return result.str();
            }
            catch (const std::exception &e)
            {
                throw std::runtime_error("gzip decompress failed: " + std::string(e.what()));
                // BLUE_LOG_ERROR(g_logger) << "http requestParser gzip compress error : " << errno
                //                          << " strerror : " << strerror(errno);
                // return nullptr;
            }
        }

        void HttpResponse::reset()
        {
            m_status = HttpStatus::OK;
            m_version = 0x11;
            m_keepAlive = true;
            m_body.clear();
            m_reason.clear();
            m_header.clear();
        }
    }
}