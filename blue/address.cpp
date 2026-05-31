#include <sstream>
#include <string.h>
#include <netdb.h>
#include <vector>
#include <sys/types.h>
#include <ifaddrs.h>
#include "address.h"
#include "macro.h"
#include "mendian.h"
#include "log.h"
#include "url.h"

// ip地址解析
namespace blue
{
	static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

	// 计算掩码中1的数量
	template <typename T>
	static uint32_t Count_one(T x)
	{
		uint32_t ans = 0;
		while (x)
		{
			ans++;
			// 去掉低位1
			x &= (x - 1);
		}
		return ans;
	}

	// 高high位为0的掩码
	template <typename T>
	static std::enable_if_t<std::is_unsigned_v<T>, T> CreateMask(uint32_t high)
	{
		constexpr unsigned bits = sizeof(T) << 3; // 至少为8位
		BLUE_ASSERT2(high != 0, "high 不能为 0");
		BLUE_ASSERT2(high < bits, "high 超过T类型最大位数");
		unsigned host_bits = bits - high;
		return (static_cast<T>(1) << host_bits) - 1;
	}

	// Address

	std::shared_ptr<Address> Address::LookupAny(const std::string &host,
												int family, int type, int protocol)
	{
		std::vector<Address::AddressPtr> results;
		if (Lookup(results, host, family, type, protocol))
		{
			return results[0];
		}
		return nullptr;
	}

	std::shared_ptr<IPAddress> Address::LookupAnyIpAddress(const std::string &host,
														   int family, int type, int protocol)
	{
		std::vector<Address::AddressPtr> results;
		if (Lookup(results, host, family, type, protocol))
		{
			for (auto &x : results)
			{
				std::shared_ptr<IPAddress> v = std::dynamic_pointer_cast<IPAddress>(x);
				if (v)
				{
					return v;
				}
			}
		}
		return nullptr;
	}

	bool Address::Lookup(std::vector<Address::AddressPtr> &results, const std::string &host,
						 int family, int type, int protocol)
	{
		if (host.empty())
		{
			BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), host is empty";
			return false;
		}
		struct addrinfo hints, *res, *next;
		memset(&hints, 0, sizeof(hints));
		// getaddrinfo的hints参数
		hints.ai_flags = 0;
		hints.ai_family = family;
		hints.ai_socktype = (type == 0) ? SOCK_STREAM : type;
		hints.ai_protocol = (protocol == 0 && type == 0) ? IPPROTO_TCP : protocol;
		hints.ai_addrlen = 0;
		hints.ai_canonname = NULL;
		hints.ai_next = NULL;
		hints.ai_addr = NULL;
		
		auto Url = blue::Url::CreateUrl(host);
		std::string bulk, service;
		if (Url)
		{
			bulk = Url->getHost();
			if (bulk.empty()) 
			{
				bulk = host;  // 降级
			}
			service = std::to_string(Url->getPort());
		}
		else
		{
			// Url 解析失败，降级为简单处理
			BLUE_LOG_ERROR(g_logger) << "url parser failed, host : " << host
									 << " 降级为我们自己的处理";
			std::string port = "80";
			if (host.find("https://") != std::string::npos)
			{
				port = "443";
			}

			// 统一判断是否有http://
			size_t addr_start = host.find("://");
			addr_start = addr_start == std::string::npos ? 0 : addr_start + 3;

			// 判断是否是 没有带有[] 的ipv6地址
			bool isipv6nobracket = (std::count(host.begin(), host.end(), ':') >= 2) && (host.find('.') == std::string::npos);

			// 解析ipv6
			// http://[2001:db8::1]:8080/index.html
			if (family == AF_INET6 || host[addr_start] == '[' || isipv6nobracket)
			{
				size_t ipv6_start_pos = host.find('[');
				if (ipv6_start_pos != std::string::npos)
				{
					size_t ipv6_end_pos = host.find(']');
					if (ipv6_end_pos != std::string::npos && ipv6_start_pos < ipv6_end_pos)
					{
						// ipv6地址
						bulk = host.substr(ipv6_start_pos + 1, ipv6_end_pos - ipv6_start_pos - 1);
						if (bulk.empty())
						{
							BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), host have not ipv6address,host : " << host;
							return false;
						}

						// port
						if (ipv6_end_pos + 1 < host.size() && host[ipv6_end_pos + 1] == ':')
						{
							size_t port_start = ipv6_end_pos + 2;
							size_t port_end = host.find('/', port_start);
							// 无论port_end是多少,最后都可以得到正确的长度(一般port长度不超过6)
							// 而port_end最后要么等于npos,要么等于正确的下标
							// port_end >= port_start
							service = host.substr(port_start, port_end - port_start);
						}
						if (service.empty())
						{
							service = port;
						}
						if (std::stoul(service) > 65535)
						{
							BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), port > 65535,host : " << host;
							return false;
						}
					}
					else
					{
						BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), host error,host : " << host;
						return false;
					}
				}
				else if (isipv6nobracket)
				{
					bulk = host.substr(addr_start);
					service = port;
				}
				else
				{
					BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), host error,host : " << host;
					return false;
				}
			}
			// ipv4(http://192.168.1.1:8080/index.html)
			// 或 http://www.baidu.com:8080/index.html
			else
			{
				size_t addr_end = host.find(':', addr_start);
				// 无论addr_end是多少,有效还是无效,ipv4地址或网址都可以被正确解析出来
				// addr_end >= addr_start
				// ipv4地址或网址
				bulk = host.substr(addr_start, addr_end - addr_start);
				if (bulk.empty())
				{
					BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), host have not ipv4address,host : " << host;
					return false;
				}

				// 这里必须区分,因为计算port_start需要对addr_end + 1,不区分可能导致size_t溢出
				if (addr_end == std::string::npos)
				{
					// port默认
					service = port;
				}
				else
				{
					// port
					size_t port_start = addr_end + 1;
					size_t port_end = host.find('/', port_start);
					// 无论port_end是多少,最后都可以得到正确的长度(一般port长度不超过6)
					// 而port_end最后要么等于npos,要么等于正确的下标
					// port_end >= port_start
					service = host.substr(port_start, port_end - port_start);
					if (service.empty())
					{
						service = port;
					}
					if (std::stoul(service) > 65535)
					{
						BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), port > 65535,host : " << host;
						return false;
					}
				}
			}
		}
		if (bulk.empty())
		{
			BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), host parse failed: " << host;
			return false;
		}
		int error = getaddrinfo(bulk.c_str(), service.c_str(), &hints, &res);
		if (error)
		{
			BLUE_LOG_ERROR(g_logger) << "bool Address::Lookup(), [host : " << host
									 << " family : " << family << " type : " << type
									 << " protocol : " << protocol << "],error : "
									 << error << " gai_strerror : " << gai_strerror(error);
			return false;
		}
		next = res;

		while (next)
		{
			results.push_back(Create(next->ai_addr, next->ai_addrlen));
			next = next->ai_next;
		}
		freeaddrinfo(res);
		return true;
	}

	bool Address::GetInterfaceAddress(std::multimap<std::string,
													std::pair<std::shared_ptr<Address>, uint32_t>> &results,
									  int family)
	{
		struct ifaddrs *next, *res;
		if (getifaddrs(&res) != 0)
		{
			BLUE_LOG_ERROR(g_logger) << "bool Address::GetinterfaceAddress() error, family : "
									 << family << "errno : " << errno << " strerror : "
									 << strerror(errno);
			return false;
		}

		try
		{
			for (next = res; next; next = next->ifa_next)
			{
				std::shared_ptr<Address> addr;
				uint32_t prefix_len = ~0u;
				if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
				{
					continue;
				}
				switch (next->ifa_addr->sa_family)
				{
				case (AF_INET):
				{
					addr = Create(next->ifa_addr, sizeof(sockaddr_in));
					uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
					prefix_len = Count_one<uint32_t>(netmask);
				}
				break;
				case (AF_INET6):
				{
					addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
					in6_addr &tem = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
					prefix_len = 0;
					for (int i = 0; i < 16; i++)
					{
						prefix_len += Count_one<uint8_t>(tem.s6_addr[i]);
					}
				}
				break;
				default:
					break;
				}

				if (addr)
				{
					auto tem = std::make_pair(addr, prefix_len);
					std::string name(next->ifa_name);
					auto result = std::make_pair(name, tem);
					results.insert(result);
				}
			}
		}
		catch (...)
		{
			BLUE_LOG_ERROR(g_logger) << "bool Address::GetinterfaceAddress() exception";
			freeifaddrs(res);
			return false;
		}
		freeifaddrs(res);
		return true;
	}

	bool Address::GetInterfaceAddress(std::vector<std::pair<std::shared_ptr<Address>, uint32_t>> &results,
									  std::string &interface, int family)
	{
		if (interface.empty() || interface == "*")
		{
			if (family == AF_INET || family == AF_UNSPEC)
			{
				std::shared_ptr<Address> res = std::make_shared<IPv4Address>();
				results.emplace_back(res, 0u);
			}
			if (family == AF_INET6 || family == AF_UNSPEC)
			{
				std::shared_ptr<Address> res = std::make_shared<IPv6Address>();
				results.emplace_back(res, 0u);
			}
			return true;
		}

		std::multimap<std::string,
					  std::pair<std::shared_ptr<Address>, uint32_t>>
			res;

		if (!GetInterfaceAddress(res, family))
		{
			return false;
		}

		auto its = res.equal_range(interface);
		for (; its.first != its.second; ++its.first)
		{
			results.emplace_back(its.first->second);
		}
		return true;
	}

	std::shared_ptr<Address> Address::Create(const sockaddr *addr, socklen_t addrlen)
	{
		if (addr == nullptr)
		{
			return nullptr;
		}
		std::shared_ptr<Address> res;
		switch (addr->sa_family)
		{
		case AF_INET:
			res.reset(new IPv4Address(*(const sockaddr_in *)(addr)));
			break;
		case AF_INET6:
			res.reset(new IPv6Address(*(const sockaddr_in6 *)(addr)));
			break;
		default:
			res.reset(new UnknowAddress(*addr));
			break;
		}
		return res;
	}

	int Address::getFamily() const
	{
		return getAddr()->sa_family;
	}

	std::string Address::toString(bool show_port) const
	{
		std::stringstream ss;
		insert(ss, show_port);
		return ss.str();
	}

	bool Address::operator<(const Address &rhs) const
	{
		socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
		int res = memcmp(getAddr(), rhs.getAddr(), minlen);
		if (res < 0)
		{
			return true;
		}
		else if (res > 0)
		{
			return false;
		}
		return getAddrLen() < rhs.getAddrLen();
	}

	bool Address::operator==(const Address &rhs) const
	{
		socklen_t l = getAddrLen(), r = rhs.getAddrLen();
		int res = memcmp(getAddr(), rhs.getAddr(), l);
		return res == 0 && l == r;
	}

	bool Address::operator!=(const Address &rhs) const
	{
		return !(*this == rhs);
	}

	// IPAddress

	std::shared_ptr<IPAddress> IPAddress::Create(const char *addr, uint16_t port)
	{
		struct addrinfo hints, *res;
		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_NUMERICHOST; // 必须用字符串表示的IP地址,仅支持点分十进制ipv4和16进制的ipv6
		hints.ai_family = AF_UNSPEC;
		int error = getaddrinfo(addr, NULL, &hints, &res);
		if (error)
		{
			BLUE_LOG_ERROR(g_logger) << "IPAddress::Create(" << addr
									 << "," << port << "), error : "
									 << error << " gai_strerror : "
									 << gai_strerror(error);
			return nullptr;
		}
		try
		{
			std::shared_ptr<IPAddress> result = std::dynamic_pointer_cast<IPAddress>(Address::Create(res->ai_addr, res->ai_addrlen));
			if (result)
			{
				result->setPort(port);
				freeaddrinfo(res);
				return result;
			}
		}
		catch (...)
		{
			freeaddrinfo(res);
			return nullptr;
		}
		freeaddrinfo(res);
		return nullptr;
	}

	// IPv4

	IPv4Address::IPv4Address()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
	}

	IPv4Address::IPv4Address(const sockaddr_in &addr)
	{
		m_addr = addr;
	}

	IPv4Address::IPv4Address(uint32_t address, uint16_t port)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = bitswaphTon(port); // 这个和htonl等价
		m_addr.sin_addr.s_addr = bitswaphTon(address);
	}

	std::shared_ptr<IPv4Address> IPv4Address::Create(const char *addr, uint16_t port)
	{
		auto res = std::make_shared<IPv4Address>();
		memset(&res->m_addr, 0, sizeof(res->m_addr));
		res->m_addr.sin_family = AF_INET;
		res->m_addr.sin_port = bitswaphTon(port);
		int ret = inet_pton(AF_INET, addr, &res->m_addr.sin_addr);
		if (ret == 0)
		{
			BLUE_LOG_ERROR(g_logger) << "IPv4Address::Create(" << addr
									 << "," << port << ")ret : " << ret
									 << " errno : " << errno
									 << " strerror : " << strerror(errno);
			return nullptr;
		}
		return res;
	}

	const sockaddr *IPv4Address::getAddr() const
	{
		return (sockaddr *)(&m_addr);
	}

	sockaddr *IPv4Address::getAddr()
	{
		return (sockaddr *)(&m_addr);
	}

	socklen_t IPv4Address::getAddrLen() const
	{
		return sizeof(m_addr);
	}

	std::string IPv4Address::getIp() const
	{
		std::string ip_str;
		char buf[INET_ADDRSTRLEN];
        auto* sin = (struct sockaddr_in*)getAddr();
        inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
        ip_str = buf;
		return ip_str;
	}

	std::ostream &IPv4Address::insert(std::ostream &os, bool show_port) const
	{
		char buf[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
		os << buf;
		if (show_port)
		{
			os << ":" << bitswapnToh(m_addr.sin_port);
		}
		return os;

		// // 采用位运算
		// // os << " [m_addr : debug: 0x" << std::hex <<  m_addr.sin_addr.s_addr << std::dec << "]";
		// uint32_t addr = bitswapnToh(m_addr.sin_addr.s_addr);
		// os << (int)((addr >> 24) & 0xff) << "."
		//    << (int)((addr >> 16) & 0xff) << "."
		//    << (int)((addr >> 8)  & 0xff) << "."
		//    << (int)((addr >> 0)  & 0xff);
		// if (show_port)
		// {
		//     os << ":" << bitswapnToh(m_addr.sin_port);
		// }
		// return os;

		// // 利用union直接读原始内存布局
		// union address
		// {
		// 	uint32_t _addr;
		// 	uint8_t _addrs[4];
		// };
		// union address addr;
		// addr._addr = m_addr.sin_addr.s_addr;
		// os << (int)addr._addrs[0] << "."
		//    << (int)addr._addrs[1] << "."
		//    << (int)addr._addrs[2] << "."
		//    << (int)addr._addrs[3];
		// if (show_port)
		// {
		//     os << ":" << bitswapnToh(m_addr.sin_port);
		// }
		// return os;
	}

	std::shared_ptr<IPAddress> IPv4Address::broadcastAddress(uint32_t prefix_len)
	{
		if (prefix_len >= 32)
		{
			return nullptr;
		}
		sockaddr_in broad(m_addr);
		// 高prefix_len位为0
		// 广播地址，主机号全为1
		broad.sin_addr.s_addr |= bitswaphTon(CreateMask<uint32_t>(prefix_len));
		return std::make_shared<IPv4Address>(broad);
	}

	std::shared_ptr<IPAddress> IPv4Address::networkAddress(uint32_t prefix_len)
	{
		if (prefix_len >= 32)
		{
			return nullptr;
		}
		sockaddr_in network(m_addr);
		// 高prefic_len位为0
		// 网络地址，主机号清0
		network.sin_addr.s_addr &= ~bitswaphTon(CreateMask<uint32_t>(prefix_len));
		return std::make_shared<IPv4Address>(network);
	}

	std::shared_ptr<IPAddress> IPv4Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in subnet;
		memset(&subnet, 0, sizeof(subnet));
		subnet.sin_family = AF_INET;
		subnet.sin_addr.s_addr = ~bitswaphTon(CreateMask<uint32_t>(prefix_len));
		return std::make_shared<IPv4Address>(subnet);
	}

	uint16_t IPv4Address::getPort()
	{
		return bitswapnToh(m_addr.sin_port);
	}

	void IPv4Address::setPort(uint16_t v)
	{
		m_addr.sin_port = bitswaphTon(v);
	}

	// IPv6

	IPv6Address::IPv6Address()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin6_family = AF_INET6;
	}

	IPv6Address::IPv6Address(const sockaddr_in6 &addr)
	{
		m_addr = addr;
	}

	std::shared_ptr<IPv6Address> IPv6Address::Create(const char *addr, uint16_t port)
	{
		auto res = std::make_shared<IPv6Address>();
		memset(&res->m_addr, 0, sizeof(res->m_addr));
		res->m_addr.sin6_family = AF_INET6;
		res->m_addr.sin6_port = bitswaphTon(port);
		int ret = inet_pton(AF_INET6, addr, &res->m_addr.sin6_addr);
		if (ret <= 0)
		{
			BLUE_LOG_ERROR(g_logger) << "IPv6Address::Create(" << addr
									 << "," << port << ")ret : " << ret
									 << " errno : " << errno
									 << " strerror : " << strerror(errno);
			return nullptr;
		}
		return res;
	}

	IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin6_family = AF_INET6;
		m_addr.sin6_port = bitswaphTon(port);
		memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
	}

	const sockaddr *IPv6Address::getAddr() const
	{
		return (sockaddr *)(&m_addr);
	}

	sockaddr *IPv6Address::getAddr()
	{
		return (sockaddr *)(&m_addr);
	}

	socklen_t IPv6Address::getAddrLen() const
	{
		return sizeof(m_addr);
	}

	std::string IPv6Address::getIp() const
	{
		std::string ip_str;
		char buf[INET6_ADDRSTRLEN];
        auto* sin6 = (struct sockaddr_in6*)getAddr();
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf));
        ip_str = buf;
		return ip_str;
	}

	std::ostream &IPv6Address::insert(std::ostream &os, bool show_port) const
	{
		char buf[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &m_addr.sin6_addr, buf, sizeof(buf));
		os << "[" << buf << "]";
		if (show_port)
		{
			os << ":" << bitswaphTon(m_addr.sin6_port);
		}
		return os;
	}

	std::shared_ptr<IPAddress> IPv6Address::broadcastAddress(uint32_t prefix_len)
	{
		if (prefix_len >= 128)
		{
			return nullptr;
		}
		sockaddr_in6 broad(m_addr);
		unsigned bit_pos = prefix_len >> 3u;
		unsigned bit_offset = prefix_len & 7u;
		// 处理边界
		if (bit_offset > 0)
		{
			broad.sin6_addr.s6_addr[bit_pos++] |= CreateMask<uint32_t>(bit_offset);
		}
		// 后序全部为1
		memset(&broad.sin6_addr.s6_addr[bit_pos], 0xff, 16 - bit_pos);
		return std::make_shared<IPv6Address>(broad);
	}

	std::shared_ptr<IPAddress> IPv6Address::networkAddress(uint32_t prefix_len)
	{
		if (prefix_len >= 128)
		{
			return nullptr;
		}
		sockaddr_in6 network(m_addr);
		unsigned bit_pos = prefix_len >> 3u;
		unsigned bit_offset = prefix_len & 7u;
		// 将prefix_len >> 3得到字节位置,ipv6共有128位,16个字节,将第prefix_len / 8个字节,从prefix_len % 8 + 1位开始设为0
		if (bit_offset > 0)
		{
			network.sin6_addr.s6_addr[bit_pos++] &= ~CreateMask<uint32_t>(bit_offset);
		}
		// 全为0
		memset(&network.sin6_addr.s6_addr[bit_pos], 0x00, 16 - bit_pos);
		return std::make_shared<IPv6Address>(network);
	}

	std::shared_ptr<IPAddress> IPv6Address::subnetMask(uint32_t prefix_len)
	{
		if (prefix_len >= 128)
		{
			return nullptr;
		}
		sockaddr_in6 subnet;
		memset(&subnet, 0, sizeof(subnet));
		subnet.sin6_family = AF_INET6;
		unsigned bit_pos = prefix_len >> 3u; // 最大15
		unsigned bit_offset = prefix_len & 7u;
		if (bit_pos > 0)
		{
			// 0 ~ bit_pos - 1
			memset(subnet.sin6_addr.s6_addr, 0xff, bit_pos);
		}
		if (bit_offset > 0)
		{
			subnet.sin6_addr.s6_addr[bit_pos] = ~CreateMask<uint32_t>(bit_offset);
		}
		return std::make_shared<IPv6Address>(subnet);
	}

	uint16_t IPv6Address::getPort()
	{
		return bitswapnToh(m_addr.sin6_port);
	}

	void IPv6Address::setPort(uint16_t v)
	{
		m_addr.sin6_port = bitswaphTon(v);
	}

	// static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;
	// 两者等价
	static const size_t MAX_PATH_LEN = sizeof(sockaddr_un::sun_path) - 1;
	// Unix

	UnixAddress::UnixAddress()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sun_family = AF_UNIX;
		m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
	}

	UnixAddress::UnixAddress(const std::string &path)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sun_family = AF_UNIX;
		m_length = path.size() + 1;

		if (!path.empty() && path[0] == '\0')
		{
			--m_length;
		}
		if (m_length > sizeof(m_addr.sun_path))
		{
			throw std::logic_error("path too long");
		}
		memcpy(m_addr.sun_path, path.c_str(), m_length);
		m_length += offsetof(sockaddr_un, sun_path);
	}

	const sockaddr *UnixAddress::getAddr() const
	{
		return (sockaddr *)(&m_addr);
	}

	sockaddr *UnixAddress::getAddr()
	{
		return (sockaddr *)(&m_addr);
	}

	socklen_t UnixAddress::getAddrLen() const
	{
		return m_length;
	}

	std::ostream &UnixAddress::insert(std::ostream &os, bool show_port) const
	{
		if (m_length > offsetof(sockaddr_un, sun_path) &&
			m_addr.sun_path[0] == '\0')
		{
			return os << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
		}
		return os << m_addr.sun_path;
	}

	// unknow

	UnknowAddress::UnknowAddress(int family)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sa_family = family;
	}

	UnknowAddress::UnknowAddress(const sockaddr &addr)
	{
		m_addr = addr;
	}

	const sockaddr *UnknowAddress::getAddr() const
	{
		return (sockaddr *)(&m_addr);
	}

	sockaddr *UnknowAddress::getAddr()
	{
		return (sockaddr *)(&m_addr);
	}

	socklen_t UnknowAddress::getAddrLen() const
	{
		return sizeof(m_addr);
	}

	std::string UnknowAddress::getIp() const
	{
		std::string ip_str;
		char buf[INET_ADDRSTRLEN];
        auto* sin = (struct sockaddr*)getAddr();
    	inet_ntop(sin->sa_family, &sin->sa_data, buf, sizeof(buf));
        ip_str = buf;
		return ip_str;
	}
	std::ostream &UnknowAddress::insert(std::ostream &os, bool show_port) const
	{
		os << "[ family = " << m_addr.sa_family << "]";
		return os;
	}
}
