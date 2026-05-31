#include <sstream>
#include <string.h>
#include <arpa/inet.h>
#include "url.h"
#include "log.h"

// url和url解析
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    Url::Url()
        : m_port(0)
    {
        m_ipBinary.fill(0);
    }

    const std::string &Url::getPath() const
    {
        static std::string s_default_path = "/";
        return m_path.empty() ? s_default_path : m_path;
    }

    uint16_t Url::getPort() const
    {
        if (m_port)
        {
            return m_port;
        }
        if (m_scheme == "https")
        {
            return 443;
        }
        return 80;
    }

    uint32_t Url::getIPv4Binary() const
    {
        uint32_t val;
        memcpy(&val, m_ipBinary.data(), 4);
        return ntohl(val);
    }

    std::string Url::getUnicodeURL() const
    {
        std::string result;
        if (!m_scheme.empty())
        {
            result += m_scheme + "://";
        }
        if (!m_userinfo.empty())
        {
            result += getDecodedUserinfo() + "@";
        }
        result += getUnicodeHost();
        if (m_port > 0)
        {
            result += ":" + std::to_string(m_port);
        }
        result += getDecodedPath();
        if (!m_query.empty())
        {
            result += "?" + getDecodedQuery();
        }
        if (!m_fragment.empty())
        {
            result += "#" + getDecodedFragment();
        }
        return result;
    }

    std::string Url::getAsciiURL() const
    {
        std::string result;
        if (!m_scheme.empty())
        {
            result += m_scheme + "://";
        }
        if (!m_userinfo.empty())
        {
            result += m_userinfo + "@";
        }
        result += m_host;
        if (m_port > 0)
        {
            result += ":" + std::to_string(m_port);
        }
        result += m_path;
        if (!m_query.empty())
        {
            result += "?" + m_query;
        }
        if (!m_fragment.empty())
        {
            result += "#" + m_fragment;
        }
        return result;
    }

    std::string Url::getAuthority() const
    {
        std::string result;
        if (!m_userinfo.empty())
        {
            result += m_userinfo + "@";
        }
        result += m_host;
        if (m_port > 0)
        {
            result += ":" + std::to_string(m_port);
        }
        return result;
    }

    bool Url::_isDefaultPort() const
    {
        if (m_port == 0)
        {
            return true;
        }
        if (m_scheme == "http")
        {
            return m_port == 80;
        }
        if (m_scheme == "https")
        {
            return m_port == 443;
        }
        return false;
    }

    std::ostream &Url::dump(std::ostream &os) const
    {
        os << m_scheme << "://"
           << m_userinfo
           << (m_userinfo.empty() ? "" : "@")
           << m_host
           << (_isDefaultPort() ? "" : ":" + std::to_string(m_port))
           << getPath()
           << (m_query.empty() ? "" : "?")
           << m_query
           << (m_fragment.empty() ? "" : "#")
           << m_fragment;
        return os;
    }

    std::string Url::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    std::shared_ptr<Address> Url::createAddress() const
    {
        BLUE_LOG_WARN(g_logger) << "createAddress m_host=" << m_host;
        auto addr = blue::Address::LookupAnyIpAddress(m_host);
        BLUE_LOG_WARN(g_logger) << "addr=" << (addr ? addr->toString() : "null");
        if (addr)
        {
            addr->setPort(getPort());
        }
        return addr;
    }
}