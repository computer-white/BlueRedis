#include <string>
#include "src/util/log.h"
#include "src/util/url.h"

namespace blue
{
    namespace spider
    {
        // 相对路径转绝对路径
        inline std::string resolveUrl(const std::string &base, const std::string &loc)
        {
            auto logger = BLUE_LOG_NAME("system");
            if (loc.empty() || loc[0] == '#')
            {
                return "";
            }
            if (loc.rfind("javascript:", 0) == 0)
            {
                return "";
            }
            if (loc.rfind("mailto:", 0) == 0)
            {
                return "";
            }
            if (loc.rfind("data:", 0) == 0)
            {
                return "";
            }
            if (loc.rfind("http://", 0) == 0 || loc.rfind("https://", 0) == 0)
            {
                return "";
            }

            auto baseUrl = blue::Url::CreateUrl(base);
            if (!baseUrl)
            {
                return "";
            }

            std::string scheme = baseUrl->getScheme();
            std::string host = baseUrl->getHost();
            int port = baseUrl->getPort();

            // 端口拼接：非默认端口才拼
            std::string origin = scheme + "://" + host;
            if (port > 0 && !((scheme == "http" && port == 80) || (scheme == "https" && port == 443)))
            {
                origin += ":" + std::to_string(port);
            }

            if (loc[0] == '/')
            {
                return origin + loc;
            }

            auto pos = base.rfind('/');
            if (pos == std::string::npos)
            {
                return "";
            }
            return base.substr(0, pos + 1) + loc;
        }
    }
}