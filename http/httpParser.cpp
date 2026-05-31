#include <blue/url.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <exception>
#include "httpParser.h"
#include "blue/log.h"
#include "blue/config.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

static blue::ConfigVar<uint64_t>::ConfigVarPtr g_http_request_buffer_size =
    blue::Config::Lookup<uint64_t>("http.request.buffer_size", 4 * 1024ull, "http request buffer size");

static blue::ConfigVar<uint64_t>::ConfigVarPtr g_http_request_max_body_size =
    blue::Config::Lookup<uint64_t>("http.request.max_body_size", 64 * 1024ull * 1024ull, "http request max body size");

static blue::ConfigVar<uint64_t>::ConfigVarPtr g_http_response_buffer_size =
    blue::Config::Lookup<uint64_t>("http.response.buffer_size", 4 * 1024ull, "http response buffer size");

static blue::ConfigVar<uint64_t>::ConfigVarPtr g_http_response_max_body_size =
    blue::Config::Lookup<uint64_t>("http.response.max_body_size", 64 * 1024ull * 1024ull, "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;

static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

struct __RequestSizeIniter__
{
    __RequestSizeIniter__()
    {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();
        g_http_request_buffer_size->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                                { s_http_request_buffer_size = new_val; });

        g_http_request_max_body_size->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                                  { s_http_request_max_body_size = new_val; });

        g_http_response_buffer_size->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                                 { s_http_response_buffer_size = new_val; });

        g_http_response_max_body_size->addListener([](const uint64_t &old_val, const uint64_t &new_val)
                                                   { s_http_response_max_body_size = new_val; });
    }
};

static __RequestSizeIniter__ __S_Request_Size_Initer__;

// http 解析(llhttp)
namespace blue
{
    namespace http
    {
        namespace boostios = boost::iostreams;

        std::shared_ptr<HttpRequestParser> Parser::CreateHttpRequestParser()
        {
            return std::make_shared<HttpRequestParser>();
        }
        std::shared_ptr<HttpResponseParser> Parser::CreateHttpResponseParser()
        {
            return std::make_shared<HttpResponseParser>();
        }

        std::string Parser::compress(const std::string &data) const
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

        std::string Parser::decompress(const std::string &data) const
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

        bool Parser::checkEncoding(const std::string &encoding, const char *tem)
        {
            std::stringstream ss(encoding);
            std::string item;
            bool ans = false;
            while (std::getline(ss, item, ','))
            {
                item.erase(0, item.find_first_not_of(' '));
                item.erase(item.find_last_not_of(' ') + 1);
                if (item == tem || item.find(tem) != std::string::npos)
                {
                    ans = true;
                    break;
                }
            }
            return ans;
        }

        void HttpRequestParser::SetRequestBufferSize(size_t size)
        {
            if (size == 0)
            {
                size = g_http_request_buffer_size->getValue() * 2;
            }
            g_http_request_buffer_size->setValue(size);
        }

        uint64_t HttpRequestParser::GetHttpRequestBufferSize()
        {
            return s_http_request_buffer_size;
        }
        uint64_t HttpRequestParser::GetHttpRequestMaxBodySize()
        {
            return s_http_request_max_body_size;
        }

        HttpRequestParser::HttpRequestParser()
        {
            m_data.reset(new blue::http::HttpRequest());
        }

        void HttpRequestParser::Init()
        {
            llhttp_settings_init(&m_settings);
            m_settings.on_url = _on_Url;
            m_settings.on_url_complete = _on_Url_complate;
            m_settings.on_method = _on_Method;
            m_settings.on_method_complete = _on_Method_complate;
            m_settings.on_version = _on_Version;
            m_settings.on_version_complete = _on_Version_complate;
            m_settings.on_header_field = _on_header_field;
            m_settings.on_header_value = _on_header_value;
            m_settings.on_headers_complete = _on_Header_complate;
            m_settings.on_message_begin = _on_message_begin;
            m_settings.on_message_complete = _on_Message_complate;
            m_settings.on_body = _on_body;
            llhttp_init(&m_parser, HTTP_REQUEST, &m_settings);
            m_parser.data = this;
        }

        void HttpRequestParser::Pause()
        {
            llhttp_pause(&m_parser);
        }

        void HttpRequestParser::Resume()
        {
            llhttp_resume(&m_parser);
        }

        void HttpRequestParser::Execute(const char *data, size_t len)
        {
            llhttp_errno_t err = llhttp_execute(&m_parser, data, len);
            // 不等于HPE_OK表示有格式错误，或者调用了Pause暂停解析
            // 此时可以打印出error name; error reason;以及error pos(错误发生的位置)
            if (err != HPE_OK)
            {
                const char *name = llhttp_errno_name((llhttp_errno_t)m_parser.error);
                const char *reason = llhttp_get_error_reason(&m_parser);
                const char *pos = llhttp_get_error_pos(&m_parser);

                size_t offset = pos - data;

                BLUE_LOG_ERROR(g_logger) << "HTTPrequestParser::Execute error, name : " << name
                                         << " reason : " << reason << " position : " << offset;
            }
        }

        void HttpRequestParser::Execute(const std::string &data)
        {
            Execute(data.c_str(), data.size());
        }

        llhttp_errno_t HttpRequestParser::Finalize()
        {

            return m_errno = llhttp_finish(&m_parser);
        }

        bool HttpRequestParser::isFinalized() const
        {
            return m_errno == HPE_OK;
        }

        void HttpRequestParser::lenientMode(bool on)
        {
            if (on)
            {
                llhttp_set_lenient_headers(&m_parser, on);
                llhttp_set_lenient_chunked_length(&m_parser, on);
                llhttp_set_lenient_keep_alive(&m_parser, on);
                llhttp_set_lenient_transfer_encoding(&m_parser, on);
                llhttp_set_lenient_version(&m_parser, on);
                llhttp_set_lenient_data_after_close(&m_parser, on);
                llhttp_set_lenient_optional_lf_after_cr(&m_parser, on);
                llhttp_set_lenient_optional_crlf_after_chunk(&m_parser, on);
                llhttp_set_lenient_optional_cr_before_lf(&m_parser, on);
                llhttp_set_lenient_spaces_after_chunk_size(&m_parser, on);
            }
            return;
        }

        void HttpRequestParser::on_Url(std::function<int()> cb)
        {
            m_urlcb = std::move(cb);
        }

        void HttpRequestParser::on_UrlComplate(std::function<int(const std::string &url)> cb)
        {
            m_urlCmpcb = std::move(cb);
        }

        void HttpRequestParser::on_Method(std::function<int()> cb)
        {
            m_methodcb = std::move(cb);
        }

        void HttpRequestParser::on_MethodComplate(std::function<int(const std::string &method)> cb)
        {
            m_methodCmpcb = std::move(cb);
        }

        void HttpRequestParser::on_Version(std::function<int()> cb)
        {
            m_versioncb = std::move(cb);
        }

        void HttpRequestParser::on_VersionComplate(std::function<int(const std::string &version)> cb)
        {
            m_versionCmpcb = std::move(cb);
        }

        void HttpRequestParser::on_HeaderField(std::function<int()> cb)
        {
            m_headerFieldcb = std::move(cb);
        }

        void HttpRequestParser::on_HeaderValue(std::function<int()> cb)
        {
            m_headerValuecb = std::move(cb);
        }

        void HttpRequestParser::on_MessageBegin(std::function<int()> cb)
        {
            m_messageBegcb = std::move(cb);
        }

        void HttpRequestParser::on_HeaderCompalte(std::function<int()> cb)
        {
            m_headerCmpcb = std::move(cb);
        }

        void HttpRequestParser::on_Body(std::function<int(const std::string &data)> cb)
        {
            m_Bodycb = std::move(cb);
        }

        void HttpRequestParser::on_MessageComplate(std::function<int(std::shared_ptr<HttpRequest>)> cb)
        {
            m_messageCmpcb = std::move(cb);
        }

        void HttpRequestParser::reset()
        {
            llhttp_reset(&m_parser);
            m_curr_body.clear();
            m_curr_headerfield.clear();
            m_curr_headervalue.clear();
            m_curr_method.clear();
            m_curr_url.clear();
            m_curr_version.clear();
        }

        /*
            // 这些 llhttp 没做，需要你加工
            - Cookie 解析成 key-value map  ok
            - Multipart/form-data 解析文件
            - Chunked 编码自动解码（llhttp 只识别的 chunk 边界，不解码）
            - Content-Encoding 解压（gzip） ok
            - URL 解码（%20 → 空格） ok (中文不支持)
            - Header 大小写归一化  ok
            - 连接管理（keep-alive 复用） ok (httpserver中)
            - 超时控制（接收超时、解析超时） ok (hook设置超时)
            - 请求体大小限制  ok (s_http_request_buffer_size设置默认大小,但支持热更新,可以说没有设置超大小后的处理)
            - 恶意请求防护（慢速攻击等）
            - 请求路由匹配  ok (servlet)
            - 参数绑定（path、query、body 里提取参数 ok
        */

        void HttpRequestParser::_parseQuery()
        {
            std::string query = m_data->getQuery();
            // BLUE_LOG_INFO(g_logger) << "query : " << query;
            if (query.empty())
            {
                return;
            }
            size_t start = 0;
            while (start < query.size())
            {
                size_t key_pos_end = query.find('=', start);
                size_t val_pos_end = query.find('&', start);
                if (val_pos_end == std::string::npos)
                {
                    val_pos_end = query.size();
                }

                if (key_pos_end != std::string::npos && key_pos_end < val_pos_end)
                {
                    std::string key = _urlDecode(query.substr(start, key_pos_end - start));
                    std::string val = _urlDecode(query.substr(key_pos_end + 1, val_pos_end - key_pos_end - 1));
                    // BLUE_LOG_INFO(g_logger) << "key : " << key << " val : " << val;
                    m_data->setParam(key, val);
                }
                start = val_pos_end + 1;
            }
        }

        std::string HttpRequestParser::_urlDecode(const std::string &param)
        {
            std::string result;
            result.reserve(param.length());

            for (size_t i = 0; i < param.length(); ++i)
            {
                if (param[i] == '%' && i + 2 < param.length() &&
                    std::isxdigit(param[i + 1]) && std::isxdigit(param[i + 2]))
                {
                    // 百分号编码
                    int value;
                    std::istringstream is(param.substr(i + 1, 2));
                    if (is >> std::hex >> value)
                    {
                        result += static_cast<char>(value);
                        i += 2;
                    }
                    else
                    {
                        result += param[i];
                    }
                }
                else if (param[i] == '+')
                {
                    // 加号转为空格（application/x-www-form-urlencoded 标准）
                    result += ' ';
                }
                else
                {
                    result += param[i];
                }
            }
            return result;
        }

        void HttpRequestParser::_parseCookie()
        {
            auto &key_val = m_data->getHeaders();
            auto it = key_val.find("Cookie");
            if (it == key_val.end())
            {
                return;
            }
            std::string Cookie = it->second;
            size_t start = 0;
            while (start < Cookie.size())
            {
                // 跳过空格
                while (start < Cookie.size() && Cookie[start] == ' ')
                    start++;
                if (start >= Cookie.size())
                {
                    break;
                }
                size_t key_pos_end = Cookie.find('=', start);
                size_t val_pos_end = Cookie.find(';', start);
                if (val_pos_end == std::string::npos)
                {
                    val_pos_end = Cookie.size();
                }
                if (key_pos_end != std::string::npos && key_pos_end < val_pos_end)
                {
                    std::string key = Cookie.substr(start, key_pos_end - start);
                    std::string val = Cookie.substr(key_pos_end + 1, val_pos_end - key_pos_end - 1);
                    // 去掉开头和末尾空格
                    key.erase(0, key.find_first_not_of(' '));
                    key.erase(key.find_last_not_of(' ') + 1);
                    m_data->setCookie(key, val);
                }
                start = val_pos_end + 1;
            }
        }

        void HttpRequestParser::_headerFieldFormat(std::string &headerfield)
        {
            size_t n = headerfield.size();
            for (int i = 0; i < n; i++)
            {
                char ch = headerfield[i];
                if (i == 0)
                {
                    headerfield[i] = std::toupper(ch);
                }
                else if (ch == '-' && i < n - 1)
                {
                    headerfield[i + 1] = std::toupper(headerfield[i + 1]);
                }
                else if (std::isalpha(ch))
                {
                    headerfield[i] = std::tolower(ch);
                }
            }
        }

        // static

        int HttpRequestParser::_on_Url(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            if (self->m_curr_url.size() + len > 8192)
            {
                llhttp_set_error_reason(p, "URL too long");
                return -1;
            }
            self->m_curr_url.append(at, len);
            return self->m_urlcb ? self->m_urlcb() : 0;
        }

        int HttpRequestParser::_on_Url_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            auto url = blue::Url::CreateUrl(self->m_curr_url);
            if (url)
            {
                self->m_data->setPath(url->getPath());
                self->m_data->setQuery(url->getQuery());
                self->m_data->setFragment(url->getFragment());
                if (!url->getHost().empty())
                {
                    self->m_data->setHeader("Host", url->getHost());
                }
            }
            self->_parseQuery();
            return self->m_urlCmpcb ? self->m_urlCmpcb(self->m_curr_url) : 0;
        }

        int HttpRequestParser::_on_Method(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            self->m_curr_method.append(at, len);
            return self->m_methodcb ? self->m_methodcb() : 0;
        }

        int HttpRequestParser::_on_Method_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            http::HttpMethod method = http::StringToHttpMethod(self->m_curr_method);
            if (method == http::HttpMethod::INVAILD_METHOD)
            {
                BLUE_LOG_WARN(g_logger) << "invaild http request method: " << self->m_curr_method;
                return -1;
            }
            self->m_data->setMethod(method);
            return self->m_methodCmpcb ? self->m_methodCmpcb(self->m_curr_method) : 0;
        }

        int HttpRequestParser::_on_Version(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            self->m_curr_version.append(at, len);
            return self->m_versioncb ? self->m_versioncb() : 0;
        }

        int HttpRequestParser::_on_Version_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            std::string tem = "HTTP/" + self->m_curr_version;
            if (strcmp(tem.c_str(), "HTTP/1.1") != 0 && strcmp(tem.c_str(), "HTTP/1.0") != 0)
            {
                BLUE_LOG_WARN(g_logger) << "invaild http request version: " << tem;
                return -1;
            }
            uint8_t version = ((p->http_major << 4) | p->http_minor);
            self->m_data->setVersion(version);
            return self->m_versionCmpcb ? self->m_versionCmpcb(tem) : 0;
        }

        int HttpRequestParser::_on_message_begin(llhttp_t *p)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            return self->m_messageBegcb ? self->m_messageBegcb() : 0;
        }

        int HttpRequestParser::_on_Message_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            std::string body = self->m_curr_body;
            // 对解析完成的进行gzip解压
            std::string encoding = self->m_data->getHeader("Content-Encoding", "");
            if (self->checkEncoding(encoding, "gzip"))
            {
                // BLUE_LOG_INFO(g_logger) << "has gzip";
                if (body.empty() || body.size() < 20)
                {
                    BLUE_LOG_ERROR(g_logger) << "body is empty or body size too less,can't gzip for this";
                }
                else
                {
                    // BLUE_LOG_INFO(g_logger) << "request body will decompress, body size : " << body.size();
                    body = self->decompress(body);
                    self->m_data->setHeader("Content-Length", std::to_string(body.size()));
                    self->m_data->delHeader("Content-Encoding"); // 后序解压完的body大小会被重设,需要删除编码标识,防止下游重复decompress
                    // BLUE_LOG_INFO(g_logger) << "request body decompress after, body size : " << body.size();
                }
            }
            self->m_data->setBody(body);
            auto old_data = self->m_data;
            int ret = self->m_messageCmpcb ? self->m_messageCmpcb(old_data) : 0;
            return ret;
        }

        int HttpRequestParser::_on_header_field(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            if (self->m_curr_headerfield.size() + len > 8192)
            {
                llhttp_set_error_reason(p, "URL too long");
                return -1;
            }

            if (!self->m_curr_headerfield.empty() && !self->m_curr_headervalue.empty())
            {
                self->m_data->setHeader(self->m_curr_headerfield, self->m_curr_headervalue);
            }
            self->m_curr_headerfield = std::string(at, len);
            // 头部格式化 connection -> Connection; content-type -> Content-Type
            self->_headerFieldFormat(self->m_curr_headerfield);
            self->m_curr_headervalue.clear();
            return self->m_headerFieldcb ? self->m_headerFieldcb() : 0;
        }

        int HttpRequestParser::_on_header_value(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            self->m_curr_headervalue.append(at, len);
            return self->m_headerValuecb ? self->m_headerValuecb() : 0;
        }

        int HttpRequestParser::_on_Header_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            if (!self->m_curr_headerfield.empty())
            {
                self->m_data->setHeader(self->m_curr_headerfield, self->m_curr_headervalue);
            }
            self->m_data->setKeepAlive(llhttp_should_keep_alive(p));
            self->_parseCookie();
            return self->m_headerCmpcb ? self->m_headerCmpcb() : 0;
        }

        int HttpRequestParser::_on_body(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpRequestParser *>(p->data);
            std::string line(at, len);
            self->m_curr_body.append(at, len);
            return self->m_Bodycb ? self->m_Bodycb(line) : 0;
        }

        /* response */

        uint64_t HttpResponseParser::GetHttpResponseBufferSize()
        {
            return s_http_response_buffer_size;
        }

        uint64_t HttpResponseParser::GetHttpResponseMaxBodySize()
        {
            return s_http_response_max_body_size;
        }

        void HttpResponseParser::SetResponseBufferSize(size_t size)
        {
            if (size == 0)
            {
                size = g_http_response_buffer_size->getValue() * 2;
            }
            g_http_response_buffer_size->setValue(size);
        }

        HttpResponseParser::HttpResponseParser()
        {
            m_data.reset(new blue::http::HttpResponse());
        }

        void HttpResponseParser::Init()
        {
            llhttp_settings_init(&m_settings);
            m_settings.on_status = _on_Status;
            m_settings.on_status_complete = _on_Status_complate;
            m_settings.on_version = _on_Version;
            m_settings.on_version_complete = _on_Version_complate;
            m_settings.on_body = _on_body;
            m_settings.on_header_field = _on_header_field;
            m_settings.on_header_value = _on_header_value;
            m_settings.on_headers_complete = _on_Header_complate;
            m_settings.on_message_begin = _on_message_begin;
            m_settings.on_message_complete = _on_Message_complate;
            llhttp_init(&m_parser, HTTP_RESPONSE, &m_settings);
            m_parser.data = this;
        }

        void HttpResponseParser::on_Status(std::function<int()> cb)
        {
            m_Statuscb = std::move(cb);
        }

        void HttpResponseParser::on_StatusComplate(std::function<int(const std::string &)> cb)
        {
            m_StatusCmpcb = std::move(cb);
        }

        void HttpResponseParser::on_Version(std::function<int()> cb)
        {
            m_versioncb = std::move(cb);
        }

        void HttpResponseParser::on_VersionComplate(std::function<int(const std::string &version)> cb)
        {
            m_versionCmpcb = std::move(cb);
        }

        void HttpResponseParser::on_HeaderField(std::function<int()> cb)
        {
            m_headerFieldcb = std::move(cb);
        }

        void HttpResponseParser::on_HeaderValue(std::function<int()> cb)
        {
            m_headerValuecb = std::move(cb);
        }

        void HttpResponseParser::on_HeaderCompalte(std::function<int()> cb)
        {
            m_headerCmpcb = std::move(cb);
        }

        void HttpResponseParser::on_Body(std::function<int(const std::string &data)> cb)
        {
            m_Bodycb = std::move(cb);
        }

        void HttpResponseParser::on_MessageComplate(std::function<int(std::shared_ptr<HttpResponse>)> cb)
        {
            m_messageCmpcb = std::move(cb);
        }

        void HttpResponseParser::on_MessageBegin(std::function<int()> cb)
        {
            m_messageBegcb = std::move(cb);
        }

        void HttpResponseParser::reset()
        {
            llhttp_reset(&m_parser);
            m_curr_body.clear();
            m_curr_headerfield.clear();
            m_curr_headervalue.clear();
            m_curr_reason.clear();
            m_curr_version.clear();
        }

        void HttpResponseParser::lenientMode(bool on)
        {
            if (on)
            {
                llhttp_set_lenient_headers(&m_parser, on);
                llhttp_set_lenient_chunked_length(&m_parser, on);
                llhttp_set_lenient_keep_alive(&m_parser, on);
                llhttp_set_lenient_transfer_encoding(&m_parser, on);
                llhttp_set_lenient_version(&m_parser, on);
                llhttp_set_lenient_data_after_close(&m_parser, on);
                llhttp_set_lenient_optional_lf_after_cr(&m_parser, on);
                llhttp_set_lenient_optional_crlf_after_chunk(&m_parser, on);
                llhttp_set_lenient_optional_cr_before_lf(&m_parser, on);
                llhttp_set_lenient_spaces_after_chunk_size(&m_parser, on);
            }
            return;
        }

        void HttpResponseParser::Pause()
        {
            llhttp_pause(&m_parser);
        }

        void HttpResponseParser::Resume()
        {
            llhttp_resume(&m_parser);
        }

        void HttpResponseParser::Execute(const char *data, size_t len)
        {
            llhttp_errno_t err = llhttp_execute(&m_parser, data, len);
            if (err != HPE_OK)
            {
                const char *name = llhttp_errno_name((llhttp_errno_t)m_parser.error);
                const char *reason = llhttp_get_error_reason(&m_parser);
                const char *pos = llhttp_get_error_pos(&m_parser);

                size_t offset = pos - data;

                BLUE_LOG_ERROR(g_logger) << "HTTPrequestParser::Execute error, name : " << name
                                         << " reason : " << reason << " position : " << offset;
            }
        }

        void HttpResponseParser::Execute(const std::string &data)
        {
            Execute(data.c_str(), data.size());
        }

        llhttp_errno_t HttpResponseParser::Finalize()
        {
            return m_errno = llhttp_finish(&m_parser);
        }

        bool HttpResponseParser::isFinalized() const
        {
            return m_errno == HPE_OK;
        }

        void HttpResponseParser::_headerFieldFormat(std::string &headerfield)
        {
            size_t n = headerfield.size();
            for (int i = 0; i < n; i++)
            {
                char ch = headerfield[i];
                if (i == 0)
                {
                    headerfield[i] = std::toupper(ch);
                }
                else if (ch == '-' && i < n - 1)
                {
                    headerfield[i + 1] = std::toupper(headerfield[i + 1]);
                }
                else if (std::isalpha(ch))
                {
                    headerfield[i] = std::tolower(ch);
                }
            }
        }

        int HttpResponseParser::_on_Status(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            self->m_curr_reason.append(at, len);
            return self->m_Statuscb ? self->m_Statuscb() : 0;
        }

        int HttpResponseParser::_on_Status_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            http::HttpStatus status = blue::http::StringToHttpStatus(self->m_curr_reason);
            if (status == http::HttpStatus::INVAILD_STATUS)
            {
                BLUE_LOG_WARN(g_logger) << "invaild http response status: " << self->m_curr_reason;
                return -1;
            }
            self->m_data->setReason(self->m_curr_reason);
            // self->m_data->setStatus((http::HttpStatus)p->status_code);
            self->m_data->setStatus(status);
            return self->m_StatusCmpcb ? self->m_StatusCmpcb(self->m_curr_reason) : 0;
        }

        int HttpResponseParser::_on_Version(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            self->m_curr_version.append(at, len);
            return self->m_versioncb ? self->m_versioncb() : 0;
        }

        int HttpResponseParser::_on_Version_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            std::string tem = "HTTP/" + self->m_curr_version;
            if (strcmp(tem.c_str(), "HTTP/1.1") != 0 && strcmp(tem.c_str(), "HTTP/1.0") != 0)
            {
                BLUE_LOG_WARN(g_logger) << "invaild http request version: " << tem;
                return -1;
            }
            uint8_t version = ((p->http_major << 4) | p->http_minor);
            self->m_data->setVersion(version);
            return self->m_versionCmpcb ? self->m_versionCmpcb(tem) : 0;
        }

        int HttpResponseParser::_on_header_field(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            if (self->m_curr_headerfield.size() + len > 8192)
            {
                llhttp_set_error_reason(p, "header field too long");
                return -1;
            }

            if (!self->m_curr_headerfield.empty() && !self->m_curr_headervalue.empty())
            {
                self->m_data->setHeader(self->m_curr_headerfield, self->m_curr_headervalue);
            }
            self->m_curr_headerfield = std::string(at, len);
            self->_headerFieldFormat(self->m_curr_headerfield);
            self->m_curr_headervalue.clear();
            return self->m_headerFieldcb ? self->m_headerFieldcb() : 0;
        }

        int HttpResponseParser::_on_header_value(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            self->m_curr_headervalue.append(at, len);
            return self->m_headerValuecb ? self->m_headerValuecb() : 0;
        }

        int HttpResponseParser::_on_Header_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            if (!self->m_curr_headerfield.empty())
            {
                self->m_data->setHeader(self->m_curr_headerfield, self->m_curr_headervalue);
            }
            self->m_data->setKeepAlive(llhttp_should_keep_alive(p));
            return self->m_headerCmpcb ? self->m_headerCmpcb() : 0;
        }

        int HttpResponseParser::_on_message_begin(llhttp_t *p)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            return self->m_messageBegcb ? self->m_messageBegcb() : 0;
        }

        int HttpResponseParser::_on_Message_complate(llhttp_t *p)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            std::string body = self->m_curr_body;
            std::string encoding = self->m_data->getHeader("Content-Encoding", "");
            if (self->checkEncoding(encoding, "gzip"))
            {
                if (body.empty() || body.size() < 20)
                {
                    BLUE_LOG_ERROR(g_logger) << "body is empty or body size too less,can't gzip for this";
                }
                else
                {
                    // BLUE_LOG_INFO(g_logger) << "response body will decompress,body size : " << body.size();
                    body = self->decompress(body);
                    self->m_data->setHeader("Content-Length", std::to_string(body.size()));
                    self->m_data->delHeader("Content-Encoding"); // 后序解压完的body大小会被重设,需要删除编码标识,防止下游重复decompress
                    // BLUE_LOG_INFO(g_logger) << "response body decompress after,body size : " << body.size();
                }
            }
            self->m_data->setBody(body);
            self->m_curr_body.clear();
            auto old_data = self->m_data;
            int ret = self->m_messageCmpcb ? self->m_messageCmpcb(old_data) : 0;
            return ret;
        }

        int HttpResponseParser::_on_body(llhttp_t *p, const char *at, size_t len)
        {
            auto *self = static_cast<HttpResponseParser *>(p->data);
            std::string line(at, len);
            self->m_curr_body.append(at, len);
            return self->m_Bodycb ? self->m_Bodycb(line) : 0;
        }

    }
}