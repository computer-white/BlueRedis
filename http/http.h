#ifndef BLUE_HTTP_H
#define BLUE_HTTP_H
#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <functional>
#include "llhttp.h"

// http 模块
namespace blue
{
    namespace http
    {
        /* Status Codes */
#define BLUE_HTTP_STATUS_MAP(XX)                                                                    \
    XX(100, CONTINUE, CONTINUE)                                                                     \
    XX(101, SWITCHING_PROTOCOLS, SWITCHING_PROTOCOLS)                                               \
    XX(102, PROCESSING, PROCESSING)                                                                 \
    XX(103, EARLY_HINTS, EARLY_HINTS)                                                               \
    XX(110, RESPONSE_IS_STALE, RESPONSE_IS_STALE)                                                   \
    XX(111, REVALIDATION_FAILED, REVALIDATION_FAILED)                                               \
    XX(112, DISCONNECTED_OPERATION, DISCONNECTED_OPERATION)                                         \
    XX(113, HEURISTIC_EXPIRATION, HEURISTIC_EXPIRATION)                                             \
    XX(199, MISCELLANEOUS_WARNING, MISCELLANEOUS_WARNING)                                           \
    XX(200, OK, OK)                                                                                 \
    XX(201, CREATED, CREATED)                                                                       \
    XX(202, ACCEPTED, ACCEPTED)                                                                     \
    XX(203, NON_AUTHORITATIVE_INFORMATION, NON_AUTHORITATIVE_INFORMATION)                           \
    XX(204, NO_CONTENT, NO_CONTENT)                                                                 \
    XX(205, RESET_CONTENT, RESET_CONTENT)                                                           \
    XX(206, PARTIAL_CONTENT, PARTIAL_CONTENT)                                                       \
    XX(207, MULTI_STATUS, MULTI_STATUS)                                                             \
    XX(208, ALREADY_REPORTED, ALREADY_REPORTED)                                                     \
    XX(214, TRANSFORMATION_APPLIED, TRANSFORMATION_APPLIED)                                         \
    XX(226, IM_USED, IM_USED)                                                                       \
    XX(299, MISCELLANEOUS_PERSISTENT_WARNING, MISCELLANEOUS_PERSISTENT_WARNING)                     \
    XX(300, MULTIPLE_CHOICES, MULTIPLE_CHOICES)                                                     \
    XX(301, MOVED_PERMANENTLY, MOVED_PERMANENTLY)                                                   \
    XX(302, FOUND, FOUND)                                                                           \
    XX(303, SEE_OTHER, SEE_OTHER)                                                                   \
    XX(304, NOT_MODIFIED, NOT_MODIFIED)                                                             \
    XX(305, USE_PROXY, USE_PROXY)                                                                   \
    XX(306, SWITCH_PROXY, SWITCH_PROXY)                                                             \
    XX(307, TEMPORARY_REDIRECT, TEMPORARY_REDIRECT)                                                 \
    XX(308, PERMANENT_REDIRECT, PERMANENT_REDIRECT)                                                 \
    XX(400, BAD_REQUEST, BAD_REQUEST)                                                               \
    XX(401, UNAUTHORIZED, UNAUTHORIZED)                                                             \
    XX(402, PAYMENT_REQUIRED, PAYMENT_REQUIRED)                                                     \
    XX(403, FORBIDDEN, FORBIDDEN)                                                                   \
    XX(404, NOT_FOUND, NOT_FOUND)                                                                   \
    XX(405, METHOD_NOT_ALLOWED, METHOD_NOT_ALLOWED)                                                 \
    XX(406, NOT_ACCEPTABLE, NOT_ACCEPTABLE)                                                         \
    XX(407, PROXY_AUTHENTICATION_REQUIRED, PROXY_AUTHENTICATION_REQUIRED)                           \
    XX(408, REQUEST_TIMEOUT, REQUEST_TIMEOUT)                                                       \
    XX(409, CONFLICT, CONFLICT)                                                                     \
    XX(410, GONE, GONE)                                                                             \
    XX(411, LENGTH_REQUIRED, LENGTH_REQUIRED)                                                       \
    XX(412, PRECONDITION_FAILED, PRECONDITION_FAILED)                                               \
    XX(413, PAYLOAD_TOO_LARGE, PAYLOAD_TOO_LARGE)                                                   \
    XX(414, URI_TOO_LONG, URI_TOO_LONG)                                                             \
    XX(415, UNSUPPORTED_MEDIA_TYPE, UNSUPPORTED_MEDIA_TYPE)                                         \
    XX(416, RANGE_NOT_SATISFIABLE, RANGE_NOT_SATISFIABLE)                                           \
    XX(417, EXPECTATION_FAILED, EXPECTATION_FAILED)                                                 \
    XX(418, IM_A_TEAPOT, IM_A_TEAPOT)                                                               \
    XX(419, PAGE_EXPIRED, PAGE_EXPIRED)                                                             \
    XX(420, ENHANCE_YOUR_CALM, ENHANCE_YOUR_CALM)                                                   \
    XX(421, MISDIRECTED_REQUEST, MISDIRECTED_REQUEST)                                               \
    XX(422, UNPROCESSABLE_ENTITY, UNPROCESSABLE_ENTITY)                                             \
    XX(423, LOCKED, LOCKED)                                                                         \
    XX(424, FAILED_DEPENDENCY, FAILED_DEPENDENCY)                                                   \
    XX(425, TOO_EARLY, TOO_EARLY)                                                                   \
    XX(426, UPGRADE_REQUIRED, UPGRADE_REQUIRED)                                                     \
    XX(428, PRECONDITION_REQUIRED, PRECONDITION_REQUIRED)                                           \
    XX(429, TOO_MANY_REQUESTS, TOO_MANY_REQUESTS)                                                   \
    XX(430, REQUEST_HEADER_FIELDS_TOO_LARGE_UNOFFICIAL, REQUEST_HEADER_FIELDS_TOO_LARGE_UNOFFICIAL) \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, REQUEST_HEADER_FIELDS_TOO_LARGE)                       \
    XX(440, LOGIN_TIMEOUT, LOGIN_TIMEOUT)                                                           \
    XX(444, NO_RESPONSE, NO_RESPONSE)                                                               \
    XX(449, RETRY_WITH, RETRY_WITH)                                                                 \
    XX(450, BLOCKED_BY_PARENTAL_CONTROL, BLOCKED_BY_PARENTAL_CONTROL)                               \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, UNAVAILABLE_FOR_LEGAL_REASONS)                           \
    XX(460, CLIENT_CLOSED_LOAD_BALANCED_REQUEST, CLIENT_CLOSED_LOAD_BALANCED_REQUEST)               \
    XX(463, INVALID_X_FORWARDED_FOR, INVALID_X_FORWARDED_FOR)                                       \
    XX(494, REQUEST_HEADER_TOO_LARGE, REQUEST_HEADER_TOO_LARGE)                                     \
    XX(495, SSL_CERTIFICATE_ERROR, SSL_CERTIFICATE_ERROR)                                           \
    XX(496, SSL_CERTIFICATE_REQUIRED, SSL_CERTIFICATE_REQUIRED)                                     \
    XX(497, HTTP_REQUEST_SENT_TO_HTTPS_PORT, HTTP_REQUEST_SENT_TO_HTTPS_PORT)                       \
    XX(498, INVALID_TOKEN, INVALID_TOKEN)                                                           \
    XX(499, CLIENT_CLOSED_REQUEST, CLIENT_CLOSED_REQUEST)                                           \
    XX(500, INTERNAL_SERVER_ERROR, INTERNAL_SERVER_ERROR)                                           \
    XX(501, NOT_IMPLEMENTED, NOT_IMPLEMENTED)                                                       \
    XX(502, BAD_GATEWAY, BAD_GATEWAY)                                                               \
    XX(503, SERVICE_UNAVAILABLE, SERVICE_UNAVAILABLE)                                               \
    XX(504, GATEWAY_TIMEOUT, GATEWAY_TIMEOUT)                                                       \
    XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP_VERSION_NOT_SUPPORTED)                                 \
    XX(506, VARIANT_ALSO_NEGOTIATES, VARIANT_ALSO_NEGOTIATES)                                       \
    XX(507, INSUFFICIENT_STORAGE, INSUFFICIENT_STORAGE)                                             \
    XX(508, LOOP_DETECTED, LOOP_DETECTED)                                                           \
    XX(509, BANDWIDTH_LIMIT_EXCEEDED, BANDWIDTH_LIMIT_EXCEEDED)                                     \
    XX(510, NOT_EXTENDED, NOT_EXTENDED)                                                             \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, NETWORK_AUTHENTICATION_REQUIRED)                       \
    XX(520, WEB_SERVER_UNKNOWN_ERROR, WEB_SERVER_UNKNOWN_ERROR)                                     \
    XX(521, WEB_SERVER_IS_DOWN, WEB_SERVER_IS_DOWN)                                                 \
    XX(522, CONNECTION_TIMEOUT, CONNECTION_TIMEOUT)                                                 \
    XX(523, ORIGIN_IS_UNREACHABLE, ORIGIN_IS_UNREACHABLE)                                           \
    XX(524, TIMEOUT_OCCURED, TIMEOUT_OCCURED)                                                       \
    XX(525, SSL_HANDSHAKE_FAILED, SSL_HANDSHAKE_FAILED)                                             \
    XX(526, INVALID_SSL_CERTIFICATE, INVALID_SSL_CERTIFICATE)                                       \
    XX(527, RAILGUN_ERROR, RAILGUN_ERROR)                                                           \
    XX(529, SITE_IS_OVERLOADED, SITE_IS_OVERLOADED)                                                 \
    XX(530, SITE_IS_FROZEN, SITE_IS_FROZEN)                                                         \
    XX(561, IDENTITY_PROVIDER_AUTHENTICATION_ERROR, IDENTITY_PROVIDER_AUTHENTICATION_ERROR)         \
    XX(598, NETWORK_READ_TIMEOUT, NETWORK_READ_TIMEOUT)                                             \
    XX(599, NETWORK_CONNECT_TIMEOUT, NETWORK_CONNECT_TIMEOUT)

        /* Request Methods */
#define BLUE_HTTP_METHOD_MAP(XX)         \
    XX(0, DELETE, DELETE)                \
    XX(1, GET, GET)                      \
    XX(2, HEAD, HEAD)                    \
    XX(3, POST, POST)                    \
    XX(4, PUT, PUT)                      \
    /* pathological */                   \
    XX(5, CONNECT, CONNECT)              \
    XX(6, OPTIONS, OPTIONS)              \
    XX(7, TRACE, TRACE)                  \
    /* WebDAV */                         \
    XX(8, COPY, COPY)                    \
    XX(9, LOCK, LOCK)                    \
    XX(10, MKCOL, MKCOL)                 \
    XX(11, MOVE, MOVE)                   \
    XX(12, PROPFIND, PROPFIND)           \
    XX(13, PROPPATCH, PROPPATCH)         \
    XX(14, SEARCH, SEARCH)               \
    XX(15, UNLOCK, UNLOCK)               \
    XX(16, BIND, BIND)                   \
    XX(17, REBIND, REBIND)               \
    XX(18, UNBIND, UNBIND)               \
    XX(19, ACL, ACL)                     \
    /* subversion */                     \
    XX(20, REPORT, REPORT)               \
    XX(21, MKACTIVITY, MKACTIVITY)       \
    XX(22, CHECKOUT, CHECKOUT)           \
    XX(23, MERGE, MERGE)                 \
    /* upnp */                           \
    XX(24, MSEARCH, M - SEARCH)          \
    XX(25, NOTIFY, NOTIFY)               \
    XX(26, SUBSCRIBE, SUBSCRIBE)         \
    XX(27, UNSUBSCRIBE, UNSUBSCRIBE)     \
    /* RFC-5789 */                       \
    XX(28, PATCH, PATCH)                 \
    XX(29, PURGE, PURGE)                 \
    /* CalDAV */                         \
    XX(30, MKCALENDAR, MKCALENDAR)       \
    /* RFC-2068, section 19.6.1.2 */     \
    XX(31, LINK, LINK)                   \
    XX(32, UNLINK, UNLINK)               \
    /* icecast */                        \
    XX(33, SOURCE, SOURCE)               \
    XX(34, PRI, PRI)                     \
    XX(35, DESCRIBE, DESCRIBE)           \
    XX(36, ANNOUNCE, ANNOUNCE)           \
    XX(37, SETUP, SETUP)                 \
    XX(38, PLAY, PLAY)                   \
    XX(39, PAUSE, PAUSE)                 \
    XX(40, TEARDOWN, TEARDOWN)           \
    XX(41, GET_PARAMETER, GET_PARAMETER) \
    XX(42, SET_PARAMETER, SET_PARAMETER) \
    XX(43, REDIRECT, REDIRECT)           \
    XX(44, RECORD, RECORD)               \
    XX(45, FLUSH, FLUSH)                 \
    XX(46, QUERY, QUERY)

        enum class HttpMethod
        {
#define XX(num, name, string) name = num,
            BLUE_HTTP_METHOD_MAP(XX)
#undef XX
                INVAILD_METHOD
        };

        enum class HttpStatus
        {
#define XX(code, name, desc) name = code,
            BLUE_HTTP_STATUS_MAP(XX)
#undef XX
                INVAILD_STATUS
        };

        /**
         * @brief string 转为HttpMethod
         * @param ht method字符串
         * @return HttpMethod
         * @note 严格区分大小写
         */
        HttpMethod StringToHttpMethod(const std::string &ht);

        /**
         * @brief string 转为HttpStatus
         * @param ht status字符串
         * @return HttpStatus
         * @note 不区分大小写
         */
        HttpStatus StringToHttpStatus(const std::string &ht);

        /**
         * @brief const char* 转为httpMethod
         * @param ht method字符串
         * @return HttpMethod
         * @note 严格区分大小写
         */
        HttpMethod CharsToHttpMethod(const char *ht);

        /**
         * @brief const char* 转为httpStatus
         * @param ht status字符串
         * @return HttpStatus
         * @note 不区分大小写
         */
        HttpStatus CharsToHttpStatus(const char *ht);

        /**
         * @brief httpMethod 转为 std::string
         * @param ht 需要转化的httpMethod
         * @return httpMethod字符串
         */
        std::string HttpMethodToChars(const HttpMethod &ht);

        /**
         * @brief httpStatus 转为 std::string
         * @param ht 需要转化的httpStatus
         * @return httpStatus字符串
         */
        std::string HttpStatusToChars(const HttpStatus &ht);

        /* map自定义比较器 */
        struct CompareInsensitiveLess
        {
            bool operator()(const std::string &lhs, const std::string &rhs) const;
        };

        /**
         * @brief 检查map中是否具有key-val,没有就拿出def值，有就转化为T类型交给val
         * @param m map类型
         * @param key 需要检查的key
         * @param val 拿出key对应的val(m[key])，转换为T类型在赋值给val
         * @param def 默认val
         * @return 有返回true,否则返回false
         */
        template <typename MapType, typename T>
        bool CheckGetAs(const MapType &m, const std::string &key, T &val, const T &def = T())
        {
            auto it = m.find(key);
            if (it == m.end())
            {
                val = def;
                return false;
            }
            try
            {
                val = boost::lexical_cast<T>(it->second);
                return true;
            }
            catch (...)
            {
                val = def;
                return false;
            }
        }

        /**
         * @brief 检查map中是否具有key-val,没有就返回def值
         * @param m map类型
         * @param key 需要检查的key
         * @param def 默认val
         * @return 有将m[key]转换为T类型后返回,否则返回def
         */
        template <typename MapType, typename T>
        T GetAs(const MapType &m, const std::string &key, const T &def = T())
        {
            auto it = m.find(key);
            if (it == m.end())
            {
                return def;
            }
            try
            {
                return boost::lexical_cast<T>(it->second);
            }
            catch (...)
            {
                return def;
            }
        }
        class HttpRequest
        {
        public:
            using HttpRequestPtr = std::shared_ptr<HttpRequest>;
            using MapType = std::map<std::string, std::string, CompareInsensitiveLess>;
        public:
            /**
             * @brief httprequest构造函数
             * @param version 0x11表示http/1.1,0x10表示http/1.0
             * @param keep_alive 默认true,表示长连接
             */
            HttpRequest(uint8_t version = 0x11, bool keep_alive = true);

            /**
             * @brief 获取http版本
             * @return http版本 0x11(http/1.1)或0x10(http/1.0)
             */
            uint8_t getVersion() const { return m_version; }

            /**
             * @brief 获取http请求方法
             * @return 请求方法
             */
            HttpMethod getMethod() const { return m_method; }

            /**
             * @brief 获取url中query部分
             * @return url中query部分
             */
            const std::string &getQuery() const { return m_query; }

            /**
             * @brief 获取url中fragment部分
             * @return url中fragment部分
             */
            const std::string &getFragment() const { return m_fragment; }

            /**
             * @brief 获取请求的body部分
             * @return 请求的body部分
             */
            const std::string &getBody() const { return m_body; }

            /**
             * @brief 获取url的path部分
             * @return url的path部分
             */
            const std::string &getPath() const { return m_path; }

            /**
             * @brief 获取cookies
             * @return cookies
             */
            const MapType &getCookies() const { return m_cookie; }

            /**
             * @brief 获取url中的params(查询参数)
             * @return url中的params(查询参数)
             */
            const MapType &getParams() const { return m_param; }

            /**
             * @brief 获取请求头headers(以键值对存在)
             * @return 请求头
             */
            const MapType &getHeaders() const { return m_header; }

            /**
             * @brief 设置cookies
             * @param val cookies(Map类型)
             */
            void setCookies(const MapType &val) { m_cookie = val; }

            /**
             * @brief 设置headers
             * @param val headers(Map类型)
             */
            void setHeaders(const MapType &val) { m_header = val; }

            /**
             * @brief 设置params
             * @param val params(Map类型)
             */
            void setParams(const MapType &val) { m_param = val; }

            /**
             * @brief 设置version
             * @param val 0x11 或 0x10
             */
            void setVersion(uint8_t val) { m_version = val; }

            /**
             * @brief 设置method
             * @param val httpMethod类型(GET POST...)
             */
            void setMethod(HttpMethod val) { m_method = val; }
            
            /**
             * @brief 设置query(params把query细分了)
             * @param val query值(string)
             */
            void setQuery(const std::string &val) { m_query = val; }

            /**
             * @brief 设置body
             * @param val body值(string)
             */
            void setBody(const std::string &val) { m_body = val; }

            /**
             * @brief 设置path
             * @param val path值(string)
             */
            void setPath(const std::string &val) { m_path = val; }

            /**
             * @brief 设置fragment(http请求行中url中#之后的内容)
             * @param val fragment值(string)
             */
            void setFragment(const std::string &val) { m_fragment = val; }

            /**
             * @brief 是否长连接
             * @return true 表示长连接
             */
            bool isKeepAlive() const { return m_keepAlive; }

            /**
             * @brief 设置长连接
             * @param val true 表示长连接
             */
            void setKeepAlive(bool val) { m_keepAlive = val; }

            /**
             * @brief 根据key获取请求头对应的值
             * @param key key
             * @param def 默认值
             * @return val
             */
            std::string getHeader(const std::string &key, const std::string &def = "") const;

            /**
             * @brief 根据key获取Param对应的值
             * @param key key
             * @param def 默认值
             * @return val
             */
            std::string getParam(const std::string &key, const std::string &def = "") const;

            /**
             * @brief 根据key获取Cookie对应的值
             * @param key key
             * @param def 默认值
             * @return val
             */
            std::string getCookie(const std::string &key, const std::string &def = "") const;

            /**
             * @brief 设置header的key-val
             * @param key key
             * @param val val
             * @return
             */
            void setHeader(const std::string &key, const std::string &val);

            /**
             * @brief 设置Param的key-val
             * @param key key
             * @param val val
             * @return
             */
            void setParam(const std::string &key, const std::string &val);

            /**
             * @brief 设置Cookie的key-val
             * @param key key
             * @param val val
             * @return
             */
            void setCookie(const std::string &key, const std::string &val);

            /**
             * @brief 删除header中key对应的所有值
             * @param key key
             * @return
             */
            void delHeader(const std::string &key);

            /**
             * @brief 删除Param中key对应的所有值
             * @param key key
             * @return
             */
            void delParam(const std::string &key);

            /**
             * @brief 删除Cookie中key对应的所有值
             * @param key key
             * @return
             */
            void delCookie(const std::string &key);

            /**
             * @brief 检查header中存在key的值
             * @param key key
             * @param val 若存在key对应的值，利用val拿出来(val是指针)
             * @return 存在返回true,否则返回false
             */
            bool hasHeader(const std::string &key, std::string *val = nullptr);

            /**
             * @brief 检查Param中存在key的值
             * @param key key
             * @param val 若存在key对应的值，利用val拿出来(val是指针)
             * @return 存在返回true,否则返回false
             */
            bool hasParam(const std::string &key, std::string *val = nullptr);

            /**
             * @brief 检查Cookie中存在key的值
             * @param key key
             * @param val 若存在key对应的值，利用val拿出来(val是指针)
             * @return 存在返回true,否则返回false
             */
            bool hasCookie(const std::string &key, std::string *val = nullptr);

            /**
             * @brief 对gzip进行压缩
             * @param data 需要压缩的数据
             * @return 返回压缩后的数据
             */
            std::string compress(const std::string &data) const;

            /**
             * @brief 对gzip进行解压
             * @param data 需要解压的数据
             * @return 返回解压后的数据
             */
            std::string decompress(const std::string &data) const;

            /**
             * @brief 重置所有变量
             */
            void reset();

            /**
             * @brief 检查header中是否有key，并通过val拿出来，没有就拿到的是def
             * @param m header的map
             * @param key key
             * @param val val
             * @param def def
             * @return 有返回true,否则返回false
             */
            template <typename T>
            bool checkGetHeaderAs(const MapType &m, const std::string &key, T &val, const T &def = T())
            {
                return CheckGetAs(m, key, val, def);
            }

            /**
             * @brief 检查header中是否有key，并返回val,没有返回def
             * @param m header的map
             * @param key key
             * @param def def
             * @return m[key] 或 def
             */
            template <typename T>
            T getHeaderAs(const MapType &m, const std::string &key, const T &def = T())
            {
                return GetAs(m, key, def);
            }

            /**
             * @brief 检查param中是否有key，并通过val拿出来，没有就拿到的是def
             * @param m param的map
             * @param key key
             * @param val val
             * @param def def
             * @return 有返回true,否则返回false
             */
            template <typename T>
            bool checkGetParamAs(const MapType &m, const std::string &key, T &val, const T &def = T())
            {
                return CheckGetAs(m, key, val, def);
            }

            /**
             * @brief 检查param中是否有key，并返回val,没有返回def
             * @param m param的map
             * @param key key
             * @param def def
             * @return m[key] 或 def
             */
            template <typename T>
            T getParamAs(const MapType &m, const std::string &key, const T &def = T())
            {
                return GetAs(m, key, def);
            }

            /**
             * @brief 检查cookie中是否有key，并通过val拿出来，没有就拿到的是def
             * @param m cookie的map
             * @param key key
             * @param val val
             * @param def def
             * @return 有返回true,否则返回false
             */
            template <typename T>
            bool checkGetCookieAs(const MapType &m, const std::string &key, T &val, const T &def = T())
            {
                return CheckGetAs(m, key, val, def);
            }

            /**
             * @brief 检查cookie中是否有key，并返回val,没有返回def
             * @param m cookie的map
             * @param key key
             * @param def def
             * @return m[key] 或 def
             */
            template <typename T>
            T getCookieAs(const MapType &m, const std::string &key, const T &def = T())
            {
                return GetAs(m, key, def);
            }

            /**
             * @brief 将httpRequest中的内容输出到os
             * @param os std::ostream
             */
            std::ostream &dump(std::ostream &os) const;

            /**
             * @brief 内容输出为string
             */
            std::string toString() const;

            /**
             * @brief version 转为 字符串
             */
            std::string versionToStr() const;

        private:
            HttpMethod m_method;
            uint8_t m_version;
            bool m_keepAlive;
            std::string m_scheme;
            std::string m_path;
            std::string m_query;
            std::string m_fragment;
            std::string m_body;
            MapType m_header;
            MapType m_param;
            MapType m_cookie;
        };

        class HttpResponse
        {
        public:
            using HttpResponsePtr = std::shared_ptr<HttpResponse>;
            using MapType = std::map<std::string, std::string, CompareInsensitiveLess>;

        public:
            /**
             * @brief httpresponse构造函数
             * @param version 0x11表示http/1.1,0x10表示http/1.0
             * @param keep_alive 默认true,表示长连接
             */
            HttpResponse(uint8_t version = 0x11, bool keepAlive = true);

            /**
             * @brief 获取response状态
             * @return response状态
             */
            HttpStatus getStatus() const { return m_status; }

            /**
             * @brief 获取http版本
             * @return http版本 0x11(http/1.1)或0x10(http/1.0)
             */
            uint8_t getVersion() const { return m_version; }

            /**
             * @brief 获取响应的body部分
             * @return 响应的body部分
             */
            const std::string &getBody() const { return m_body; }

            /**
             * @brief 获取响应的状态结果 (OK...)
             * @return 响应的响应的状态结果 (OK...)
             */
            const std::string &getReason() const { return m_reason; }

            /**
             * @brief 获取响应头部headers(以键值对存在)
             * @return 响应头
             */
            const MapType &getHeaders() const { return m_header; }

            /**
             * @brief 设置response状态
             * @param val response状态
             */
            void setStatus(HttpStatus val) { m_status = val; }

            /**
             * @brief 设置version
             * @param val 0x11 或 0x10
             */
            void setVersion(uint8_t val) { m_version = val; }

            /**
             * @brief 设置body
             * @param val body值(string)
             */
            void setBody(const std::string &val) { m_body = val; }

            /**
             * @brief 设置响应的状态结果 (OK...)
             * @param val 响应的响应的状态结果 (OK...)
             */
            void setReason(const std::string &val) { m_reason = val; }

            /**
             * @brief 设置headers
             * @param val headers(Map类型)
             */
            void setHeaders(const MapType &val) { m_header = val; }

            /**
             * @brief 是否长连接
             * @return true 表示长连接
             */
            bool isKeepAlive() const { return m_keepAlive; }

            /**
             * @brief 设置长连接
             * @param val true 表示长连接
             */
            void setKeepAlive(bool val) { m_keepAlive = val; }

            /**
             * @brief 根据key获取请求头对应的值
             * @param key key
             * @param def 默认值
             * @return val
             */
            std::string getHeader(const std::string &key, const std::string &def = "") const;

            /**
             * @brief 设置header的key-val
             * @param key key
             * @param val val
             * @return
             */
            void setHeader(const std::string &key, const std::string &val);

            /**
             * @brief 删除header中key对应的所有值
             * @param key key
             * @return
             */
            void delHeader(const std::string &key);

            /**
             * @brief 对gzip进行压缩
             * @param data 需要压缩的数据
             * @return 返回压缩后的数据
             */
            std::string compress(const std::string &data) const;

            /**
             * @brief 对gzip进行解压
             * @param data 需要解压的数据
             * @return 返回解压后的数据
             */
            std::string decompress(const std::string &data) const;

            /**
             * @brief 重置所有变量
             */
            void reset();

            /**
             * @brief 检查header中是否有key，并通过val拿出来，没有就拿到的是def
             * @param m header的map
             * @param key key
             * @param val val
             * @param def def
             * @return 有返回true,否则返回false
             */
            template <typename T>
            bool checkGetHeaderAs(const MapType &m, const std::string &key, T &val, const T &def = T())
            {
                return CheckGetAs(m, key, val, def);
            }

            /**
             * @brief 检查header中是否有key，并返回val,没有返回def
             * @param m header的map
             * @param key key
             * @param def def
             * @return m[key] 或 def
             */
            template <typename T>
            T getHeaderAs(const MapType &m, const std::string &key, const T &def = T())
            {
                return GetAs(m, key, def);
            }

            /**
             * @brief 将httpResponset中的内容输出到os
             * @param os std::ostream
             */
            std::ostream &dump(std::ostream &os) const;

            /**
             * @brief 内容输出为string
             */
            std::string toString() const;

            /**
             * @brief version 转为 字符串
             */
            std::string versionToStr() const;

        private:
            HttpStatus m_status;
            uint8_t m_version;
            bool m_keepAlive;
            std::string m_body;
            std::string m_reason;
            MapType m_header;
        };
    }
}

#endif