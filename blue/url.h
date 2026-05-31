#ifndef BLUE_URL_H
#define BLUE_URL_H
#include <array>
#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

// url和url解析
namespace blue
{
    /*
        http://user@example.com:8042/over/there?name=ferret#nose：
        组成部分	示例值	             说明
        scheme	    http	            协议名，与后面的 : 一起标识使用的协议
        userinfo    user                用户名
        authority	example.com:8042	主机名/IP 及端口号 (前置 // 或 @)
        path	    /over/there	        资源路径
        query	    name=ferret	        查询参数 (前置 ?)
        fragment	nose	            片段标识符 (前置 #)
    */
    enum class HostType
    {
        HOSTNAME = 0,
        IPv4 = 4,
        IPv6 = 6
    };
    class Url
    {
    public:
        using UrlPtr = std::shared_ptr<Url>;

    public:
        /**
         * @brief 构造函数
         */
        Url();

        /**
         * @brief 创建url实例智能指针
         * @param url url字符串
         * @return url实例智能指针
         */
        static std::shared_ptr<Url> CreateUrl(const std::string &url);

        /**
         * @brief 获取Scheme
         * @return Scheme
         */
        const std::string &getScheme() const { return m_scheme; }

        /**
         * @brief 获取Userinfo
         * @return Userinfo
         */
        const std::string &getUserinfo() const { return m_userinfo; }

        /**
         * @brief 获取Host
         * @return Host
         */
        const std::string &getHost() const { return m_host; }

        /**
         * @brief 获取Path
         * @return Path
         */
        const std::string &getPath() const;

        /**
         * @brief 获取Query
         * @return Query
         */
        const std::string &getQuery() const { return m_query; }

        /**
         * @brief 获取Fragment
         * @return Fragment
         */
        const std::string &getFragment() const { return m_fragment; }

        /**
         * @brief 获取UnicodeHost
         * @return UnicodeHost
         */
        const std::string &getUnicodeHost() const { return m_unicodeHost.empty() ? m_host : m_unicodeHost; }

        /**
         * @brief 获取DecodedUserinfo
         * @return DecodedUserinfo
         */
        const std::string &getDecodedUserinfo() const { return m_decodedUserinfo.empty() ? m_userinfo : m_decodedUserinfo; }

        /**
         * @brief 获取DecodedPath
         * @return DecodePath
         */
        const std::string &getDecodedPath() const { return m_decodedPath.empty() ? m_path : m_decodedPath; }

        /**
         * @brief 获取DecodedQuery
         * @return DecodedQuery
         */
        const std::string &getDecodedQuery() const { return m_decodedQuery.empty() ? m_query : m_decodedQuery; }

        /**
         * @brief 获取DecodedFragment
         * @return DecodedFragment
         */
        const std::string &getDecodedFragment() const { return m_decodedFragment.empty() ? m_fragment : m_decodedFragment; }

        /**
         * @brief 获取Port
         * @return Port
         */
        uint16_t getPort() const;

        /**
         * @brief 获取HostType(IPv4,IPV6...)
         * @return HostType
         */
        HostType getHostType() const { return m_hostType; }

        /**
         * @brief 获取IPv6Binary
         * @return IPv6Binary
         */
        std::array<uint8_t, 16> getIPv6Binary() const { return m_ipBinary; }

        /**
         * @brief 获取IPv4Binary
         * @return IPv4Binary
         */
        uint32_t getIPv4Binary() const;

        /**
         * @brief 设置Scheme
         * @param val new Scheme
         */
        void setScheme(const std::string &val) { m_scheme = val; }

        /**
         * @brief 设置Userinfo
         * @param val new Userinfo
         */
        void setUserinfo(const std::string &val) { m_userinfo = val; }

        /**
         * @brief 设置Host
         * @param val new Host
         */
        void setHost(const std::string &val) { m_host = val; }

        /**
         * @brief 设置Path
         * @param val new Path
         */
        void setPath(const std::string &val) { m_path = val; }

        /**
         * @brief 设置Query
         * @param val new Query
         */
        void setQuery(const std::string &val) { m_query = val; }

        /**
         * @brief 设置Fragment
         * @param val new Fragment
         */
        void setFragment(const std::string &val) { m_fragment = val; }

        /**
         * @brief 设置Port
         * @param val new Port
         */
        void setPort(uint16_t val) { m_port = val; }

        /**
         * @brief 设置UnicodeHost
         * @param val new UnicodeHost
         */
        void setUnicodeHost(const std::string &host) { m_unicodeHost = host; }

        /**
         * @brief 设置DecodedUserinfo
         * @param val new DecodedUserinfo
         */
        void setDecodedUserinfo(const std::string &userinfo) { m_decodedUserinfo = userinfo; }

        /**
         * @brief 设置DecodedPath
         * @param val new DecodedPath
         */
        void setDecodedPath(const std::string &path) { m_decodedPath = path; }

        /**
         * @brief 设置DecodedQuery
         * @param val new DecodedQuery
         */
        void setDecodedQuery(const std::string &query) { m_decodedQuery = query; }

        /**
         * @brief 设置DecodedFragment
         * @param val new DecodedFragment
         */
        void setDecodedFragment(const std::string &fragment) { m_decodedFragment = fragment; }

        /**
         * @brief 将内容输入到os流中
         * @return ostream
         * @note 如果是默认端口比如scheme=http,port=80则不输出port
         */
        std::ostream &dump(std::ostream &os) const;

        /**
         * @brief 转换为str输出
         * @return str
         */
        std::string toString() const;

        /**
         * @brief 由url中host创建address
         * @return addressPtr
         */
        std::shared_ptr<Address> createAddress() const;

        /**
         * @brief 获取显示完整Url(Unicode形式)
         * @return str
         */
        std::string getUnicodeURL() const;

        /**
         * @brief 获取编码后的 URL(ascii形式)
         * @return str
         */
        std::string getAsciiURL() const;

        /**
         * @brief 获取userinfo + host
         * @return str
         */
        std::string getAuthority() const;

    private:
        /**
         * @brief 判断是否是默认端口
         * @return 是返回true,否则false
         */
        bool _isDefaultPort() const;

    private:
        HostType m_hostType = HostType::HOSTNAME;
        std::array<uint8_t, 16> m_ipBinary = {};
        std::string m_scheme;
        std::string m_userinfo;
        std::string m_host;
        std::string m_path;
        std::string m_query;
        std::string m_fragment;
        uint16_t m_port;

        std::string m_unicodeHost;
        std::string m_decodedUserinfo;
        std::string m_decodedPath;
        std::string m_decodedQuery;
        std::string m_decodedFragment;
    };
}

#endif