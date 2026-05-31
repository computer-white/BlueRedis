#ifndef BLUE_ADDRESS_H
#define BLUE_ADDRESS_H
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <string>
#include <iostream>
#include <map>
#include <vector>

// ip地址解析
namespace blue
{
	class IPAddress;
	class Address
	{
	public:
		using AddressPtr = std::shared_ptr<Address>;
		Address() = default;
		virtual ~Address() = default;

		/**
		 * @brief  解析host主机名
		 * @param host 需要被解析的主机名,其中若包含的端口号为空设置为默认端口(80)
		 * @param family 地址家族,比如AF_INET.默认值为AF_INET
		 * @param type 类型,比如SOCK_STREAM... 默认0
		 * @param protocol 协议 默认0
		 * @return 返回任意一个Address的智能指针
		 */
		static std::shared_ptr<Address> LookupAny(const std::string &host,
												  int family = AF_INET, int type = 0, int protocol = 0);

		/**
		 * @brief  解析host主机名
		 * @param host 需要被解析的主机名,其中若包含的端口号为空设置为默认端口(80)
		 * @param family 地址家族,比如AF_INET.默认值为AF_INET
		 * @param type 类型,比如SOCK_STREAM... 默认0
		 * @param protocol 协议 默认0
		 * @return 返回任意一个IPAddress的智能指针
		 */
		static std::shared_ptr<IPAddress> LookupAnyIpAddress(const std::string &host,
															 int family = AF_INET, int type = 0, int protocol = 0);

		/**
		 * @brief  解析host主机名
		 * @param results 存放解析出来的Address的智能指针的容器
		 * @param host 需要被解析的主机名,其中若包含的端口号为空设置为默认端口(80)
		 * @param family 地址家族,比如AF_INET.默认值为AF_INET
		 * @param type 类型,比如SOCK_STREAM... 默认0
		 * @param protocol 协议 默认0
		 * @return 成功返回true 失败返回false
		 */
		static bool Lookup(std::vector<Address::AddressPtr> &results, const std::string &host,
						   int family = AF_INET, int type = 0, int protocol = 0);

		/**
		 * @brief  获取本机接口和ip地址
		 * @param results 存放接口名和ip地址的映射,例如 enth0 172.2953.218/20
		 * @param family 地址家族,比如AF_INET.默认值为AF_INET
		 * @return 成功返回true 失败返回false
		 */
		static bool GetInterfaceAddress(std::multimap<std::string,
													  std::pair<std::shared_ptr<Address>, uint32_t>> &results,
										int family = AF_INET);

		/**
		 * @brief  获取本机接口和ip地址
		 * @param results 存放ip地址的容器,例如 172.2953.218/20
		 * @param interface 接口名 例如 enth0
		 * @param family 地址家族,比如AF_INET.默认值为AF_INET
		 * @return 成功返回true 失败返回false
		 */
		static bool GetInterfaceAddress(std::vector<std::pair<std::shared_ptr<Address>, uint32_t>> &results,
										std::string &interface, int family = AF_INET);

		/**
		 * @brief  创建Address
		 * @param addr socket地址
		 * @param addrlen sockaddr的长度
		 * @return 成功返回一个Address的智能指针
		 * @note 通过addr->sa_family来构造相应的ip地址
		 */
		static std::shared_ptr<Address> Create(const sockaddr *addr, socklen_t addrlen);

		/**
		 * @brief  获取socket地址家族
		 * @return socket地址家族
		 */
		int getFamily() const;

		/**
		 * @brief  将地址转为字符串格式输出
		 * @param show_port 是否显示端口号,true表示显示
		 * @return 返回一个较为完整的网络地址字符串字符串
		 */
		std::string toString(bool show_port = true) const;

		/**
		 * @brief  纯虚函数
		 */
		virtual const sockaddr *getAddr() const = 0;
		virtual sockaddr *getAddr() = 0;
		virtual socklen_t getAddrLen() const = 0;
		virtual std::ostream &insert(std::ostream &os, bool show_port = true) const = 0;

		/**
		 * @brief  比较运算符重载
		 */
		bool operator<(const Address &rhs) const;
		bool operator==(const Address &rhs) const;
		bool operator!=(const Address &rhs) const;
	};

	class IPAddress : public Address
	{
	public:
		using IPAddressPtr = std::shared_ptr<IPAddress>;
		/**
		 * @brief  通过ip地址字符串和端口号创建ipaddress
		 * @param addr 一个ip地址,可以是ipv,ipv6
		 * @param port 端口号
		 * @return 返回一个ip地址的智能指针
		 */
		static std::shared_ptr<IPAddress>
		Create(const char *addr, uint16_t port = 0);

		/**
		 * @brief 广播地址
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个广播地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) = 0;

		/**
		 * @brief 网络地址
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个网络地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> networkAddress(uint32_t prefix_len) = 0;

		/**
		 * @brief 子网掩码
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个子网掩码地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) = 0;

		/**
		 * @brief 获取端口号
		 * @return 返回端口号
		 */
		virtual uint16_t getPort() = 0;

		/**
		 * @brief 设置端口号
		 * @param v 需要设置的端口值
		 * @return
		 */
		virtual void setPort(uint16_t v) = 0;
	};
	class IPv4Address : public IPAddress
	{
	public:
		/**
		 * @brief IPv4默认构造(仅初始化sin.family)
		 * @return
		 */
		IPv4Address();

		/**
		 * @brief IPv4构造(通过sockaddr_in ipv4地址构造)
		 * @param addr 一个sockaddr_in 的结构体
		 * @return
		 */
		IPv4Address(const sockaddr_in &addr);

		/**
		 * @brief 通过uint32_t的ipv4地址以及十进制端口号来构造ipv4地址
		 * @param address uint32_t的ipv4地址
		 * @param port 端口号
		 * @return
		 */
		IPv4Address(uint32_t address, uint16_t port = 0);

		/**
		 * @brief 通过点分十进制字符串地址和端口号构造ipv4地址
		 * @param addr 一个点分十进制字符串地址
		 * @param port 端口号
		 * @return 返回一个构建好的ipv4地址的智能指针
		 */
		static std::shared_ptr<IPv4Address> Create(const char *addr, uint16_t port = 0);

		/**
		 * @brief 获取ipv4地址,const
		 * @return
		 */
		virtual const sockaddr *getAddr() const override;

		/**
		 * @brief 获取ipv4地址,非const
		 * @return
		 */
		virtual sockaddr *getAddr() override;

		/**
		 * @brief 获取ipv4地址长度
		 * @return
		 */
		virtual socklen_t getAddrLen() const override;

		/**
		 * @brief 将ipv4地址和端口号写入os流,是Address::toString的辅助函数
		 * @param show_port 是否在最后的输出结果显示端口号
		 * @return os
		 */
		virtual std::ostream &insert(std::ostream &os, bool show_port = true) const override;

		/**
		 * @brief ipv4广播地址
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个广播地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;

		/**
		 * @brief ipv4网络地址
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个网络地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> networkAddress(uint32_t prefix_len) override;

		/**
		 * @brief ipv4子网掩码
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个子网掩码地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;

		/**
		 * @brief 获取端口号
		 * @return 返回端口号
		 */
		virtual uint16_t getPort() override;

		/**
		 * @brief 设置端口号
		 * @param v 需要设置的端口值
		 * @return
		 */
		virtual void setPort(uint16_t v) override;

		/**
		 * @brief 获取ip地址
		 */
		std::string getIp() const;

	private:
		sockaddr_in m_addr;
	};

	class IPv6Address : public IPAddress
	{
	public:
		using IPv6AddressPtr = std::shared_ptr<IPv6Address>;

		/**
		 * @brief IPv6默认构造(仅初始化sin6.family)
		 * @return
		 */
		IPv6Address();

		/**
		 * @brief IPv6通过16组每组8位的ipv6地址以及端口号构造
		 * @param address 一个组每组用八位二进制表示的16进制
		 * @param port 端口号
		 * @return
		 */
		IPv6Address(const uint8_t address[16], uint16_t port = 0);

		/**
		 * @brief 通过sockaddr_in6 ipv6地址构造
		 * @param addr sockaddr_in6 的ipv6结构体
		 * @return
		 */
		IPv6Address(const sockaddr_in6 &addr);

		/**
		 * @brief 通过16进制的iov6地址字符串以及端口号创建ipv6
		 * @param addr sockaddr_in6 的ipv6字符串
		 * @param port 端口号
		 * @return
		 */
		static std::shared_ptr<IPv6Address> Create(const char *addr, uint16_t port = 0);

		/**
		 * @brief 获得ipv6地址,const
		 * @return
		 */
		virtual const sockaddr *getAddr() const override;

		/**
		 * @brief 获取ipv6地址,非const
		 * @return
		 */
		virtual sockaddr *getAddr() override;

		/**
		 * @brief 设置ipv6地址
		 * @return
		 */
		virtual socklen_t getAddrLen() const override;

		/**
		 * @brief 将ipv6地址和端口号写入os流,是Address::toString的辅助函数
		 * @param show_port 是否在最后的输出结果显示端口号
		 * @return os
		 */
		virtual std::ostream &insert(std::ostream &os, bool show_port = true) const override;

		/**
		 * @brief ipv6广播地址
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个广播地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;

		/**
		 * @brief ipv6网络地址
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个网络地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> networkAddress(uint32_t prefix_len) override;

		/**
		 * @brief ipv6子网掩码
		 * @param prefix_len 高prefix_len位全部为0
		 * @return 返回一个子网掩码地址的智能指针
		 */
		virtual std::shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;

		/**
		 * @brief 获取端口号
		 * @return 返回端口号
		 */
		virtual uint16_t getPort() override;

		/**
		 * @brief 设置端口号
		 * @param v 需要设置的端口值
		 * @return
		 */
		virtual void setPort(uint16_t v) override;

		/**
		 * @brief 获取ip地址
		 */
		std::string getIp() const;

	private:
		sockaddr_in6 m_addr;
	};

	class UnixAddress : public Address
	{
	public:
		using UnixAddressPtr = std::shared_ptr<UnixAddress>;

		/**
		 * @brief 构造
		 * @param path 本地文件名作为两个本地进程通信的"接头点"
		 * @return
		 */
		UnixAddress(const std::string &path);

		/**
		 * @brief 默认构造
		 * @return
		 */
		UnixAddress();

		/**
		 * @brief 设置unix地址长度
		 * @param val 需要设置的unix地址长度
		 * @return
		 */
		void setAddrlen(uint32_t val) { m_length = val; }

		/**
		 * @brief 获取unix地址,const
		 * @return
		 */
		virtual const sockaddr *getAddr() const override;

		/**
		 * @brief 获取unix地址,非const
		 * @return
		 */
		virtual sockaddr *getAddr() override;

		/**
		 * @brief 获取unix地址长度
		 * @return
		 */
		virtual socklen_t getAddrLen() const override;

		/**
		 * @brief 将unix协议家族family写入os流
		 * @param show_port 是否在最后的输出结果显示端口号
		 * @return os
		 */
		virtual std::ostream &insert(std::ostream &os, bool show_port = true) const override;

	private:
		sockaddr_un m_addr;
		socklen_t m_length;
	};

	class UnknowAddress : public Address
	{
	public:
		using UnknowAddressPtr = std::shared_ptr<UnknowAddress>;

		/**
		 * @brief 未知ip地址的构造(通过family)
		 * @param family 地址家族
		 * @return os
		 */
		UnknowAddress(int family);

		/**
		 * @brief 通过sockaddr构造
		 * @param addr const sockaddr
		 * @return
		 */
		UnknowAddress(const sockaddr &addr);

		/**
		 * @brief 获取地址,const
		 * @return
		 */
		virtual const sockaddr *getAddr() const override;

		/**
		 * @brief 获取地址,非const
		 * @return
		 */
		virtual sockaddr *getAddr() override;

		/**
		 * @brief 获取地址长度
		 * @return 地址长度
		 */
		virtual socklen_t getAddrLen() const override;

		/**
		 * @brief 获取ip地址
		 */
		std::string getIp() const;

		/**
		 * @brief 将family写到os流
		 * @return os
		 */
		virtual std::ostream &insert(std::ostream &os, bool show_port = true) const override;

	private:
		sockaddr m_addr;
	};
}
#endif
