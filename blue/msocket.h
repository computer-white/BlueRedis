#ifndef BLUE_MSOCKET_H
#define BLUE_MSOCKET_H
#include <memory>
#include "address.h"
#include "blue/task.h"

// socket 模块
namespace blue
{
    class MSocket : public std::enable_shared_from_this<MSocket>
    {
    public:
        using MSocketPtr = std::shared_ptr<MSocket>;
        using MSocketWPtr = std::weak_ptr<MSocket>;
        MSocket(const MSocket &lhs) = delete;
        MSocket &operator=(const MSocket &lhs) = delete;

    private:

        enum Type
        {
            TCP = SOCK_STREAM,
            UDP = SOCK_DGRAM,
        };

        enum Family
        {
            UNIX = AF_UNIX,
            IPV4 = AF_INET,
            IPV6 = AF_INET6,
        };

        /**
         * @brief  获取socket选项的私有方法
         * @param level 指定要操作哪个协议的选项
         * @param option_name 指定选项的名字,通常有SOL_SOCKET...
         * @param option_val 被操作选项的指针,例如sockaddr*
         * @param option_len 被操作选项的长度指针,例如socklen_t*
         * @return 成功返回true
         */
        bool _getOption(int level, int option_name, void *option_val, socklen_t *option_len);

        /**
         * @brief  设置socket选项的私有方法
         * @param level 指定要操作哪个协议的选项
         * @param option_name 指定选项的名字,通常有SOL_SOCKET...
         * @param option_val 被操作选项的指针,例如sockaddr*
         * @param option_len 被操作选项的长度
         * @return 成功返回true
         */
        bool _setOption(int level, int option_name, const void *option_val, socklen_t option_len);

    public:
        /**
         * @brief  构造函数
         * @param domain 地址家族
         * @param type sock类型 tcp 或 udp
         * @param protocol 协议,默认0
         * @return
         */
        MSocket(int domain, int type, int protocol = 0);

        /**
         * @brief  析构函数
         * @return
         * @note 关闭socketfd
         */
        ~MSocket();

        /**
         * @brief  创建tcp
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateTcp(std::shared_ptr<Address> address);

        /**
         * @brief  创建udp
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateUdp(std::shared_ptr<Address> address);

        /**
         * @brief  创建ipv4 tcp socket
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateTcpSocket();

        /**
         * @brief  创建ipv4 udp socket
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateUdpSocket();

        /**
         * @brief  创建ipv6 tcp socket
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateTcpSocket6();

        /**
         * @brief  创建ipv6 udp socket
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateUdpSocket6();

        /**
         * @brief  创建unix tcp socket
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateUnixTcpSocket();

        /**
         * @brief  创建unix udp socket
         * @return socket ptr
         */
        static std::shared_ptr<MSocket> CreateUnixUdpSocket();

        /**
         * @brief  获取send的超时时长
         * @return 返回超时时长,通常是ms
         */
        int64_t getSendTimeout() const;

        /**
         * @brief  设置send的超时时长
         * @return
         */
        void setSendTimeout(int64_t val);

        /**
         * @brief  获取recv的超时时长
         * @return 返回超时时长,通常是ms
         */
        int64_t getRecvTimeout() const;

        /**
         * @brief  设置recv的超时时长
         * @return
         */
        void setRecvTimeout(int64_t val);

        /**
         * @brief  获取socket的选项
         * @return 成功时返回true 失败返回 false
         */
        template <typename T>
        bool getOption(int level, int option_name, T &option)
        {
            unsigned int length = sizeof(T);
            return _getOption(level, option_name, &option, (socklen_t *)&length);
        }

        /**
         * @brief  设置socket的选项
         * @return 成功时返回true 失败返回false
         */
        template <typename T>
        bool setOption(int level, int option_name, T &option)
        {
            return _setOption(level, option_name, &option, (socklen_t)sizeof(T));
        }

        /**
         * @brief 接受一个客户端连接，返回封装好的 MSocket 对象
         * @return 成功时返回一个复用当前 socket 协议族、类型、协议的新连接 MSocket 对象；
         *         失败时返回 nullptr（通常可检查 errno）
         */
        Task<std::shared_ptr<MSocket>> accept();

        /**
         * @brief 初始化socket属性,TCP禁用nagle算法,socket本地地址端口重用
         * @param address socketfd需要绑定的地址,地址的family必须与调用此函数对象的family相同
         * @return 成功返回 true
         */
        bool bind(const Address::AddressPtr address);

        /**
         * @brief 监听socket
         * @param backlog 监听队列的最大值
         * @return 成功返回true
         */
        bool listen(int backlog = SOMAXCONN);

        /**
         * @brief 建立连接
         * @param address 需要连接的地址,必须和调用对象的family一致
         * @param timeout 建立连接的超时时长. -1 表示立即建立
         * @return 成功返回 true
         */
        Task<bool> connect(const Address::AddressPtr address, uint32_t timeout = -1);

        /**
         * @brief 关闭连接
         * @return 成功返回true 失败返回 false
         */
        bool close();

        /**
         * @brief 向已连接的远程主机发送数据（TCP/SCTP 等）
         * @param buf 待发送数据的缓冲区
         * @param len 待发送数据的字节数
         * @param flags 发送标志位，如 MSG_DONTWAIT, MSG_NOSIGNAL 等，默认 0
         * @return 成功时返回实际发送的字节数；失败时返回 -1 并设置 errno
         * @note TCP 下发送成功仅表示数据进入内核缓冲区，不保证对方已收到
         */
        Task<ssize_t> send(const void *buf, size_t len, int flags = 0);

        /**
         * @brief 以聚合 I/O 方式向已连接的远程主机发送数据
         * @param bufs iovec 数组指针，每个元素指向一块独立缓冲区
         * @param len iovec 数组的元素个数，即 bufs 中有几块缓冲区
         * @param flags 发送标志位，默认 0
         * @return 成功时返回实际发送的总字节数；失败返回 -1
         */
        Task<ssize_t> send(const iovec *bufs, size_t len, int flags = 0);
        /**
         * @brief 向指定的目标地址发送数据（UDP 等非连接协议常用，也可用于连接后的 TCP）
         * @param buf 待发送数据的缓冲区
         * @param len 待发送数据的字节数
         * @param dest_addr 目标地址对象，需包含 IP 和端口
         * @param flags 发送标志位，默认 0
         * @return 成功返回实际发送的字节数；失败返回 -1
         */
        Task<ssize_t> sendTo(const void *buf, size_t len, Address::AddressPtr dest_addr, int flags = 0);

        /**
         * @brief 以聚合 I/O 方式向指定的目标地址发送数据
         * @param bufs iovec 数组指针
         * @param len iovec 数组的元素个数
         * @param dest_addr 目标地址对象
         * @param flags 发送标志位，默认 0
         * @return 成功返回实际发送的总字节数；失败返回 -1
         */
        Task<ssize_t> sendTo(const iovec *bufs, size_t len, Address::AddressPtr dest_addr, int flags = 0);

        /**
         * @brief 从已连接的远程主机接收数据
         * @param buf 接收缓冲区指针
         * @param len 缓冲区最多能容纳的字节数
         * @param flags 接收标志位，如 MSG_DONTWAIT, MSG_PEEK 等，默认 0
         * @return 成功时返回实际读取的字节数；对方已关闭连接时返回 0；失败返回 -1
         */
        Task<ssize_t> recv(void *buf, size_t len, int flags = 0);

        /**
         * @brief 以聚合 I/O 方式从已连接的远程主机接收数据到多块缓冲区
         * @param buf iovec 数组指针，用于存放接收数据
         * @param len iovec 数组的元素个数
         * @param flags 接收标志位，默认 0
         * @return 成功返回读取的总字节数；对方关闭连接返回 0；失败返回 -1
         */
        Task<ssize_t> recv(iovec *buf, size_t len, int flags = 0);

        /**
         * @brief 从远程主机接收数据并获取数据来源地址
         * @param buf 接收缓冲区指针
         * @param len 缓冲区最大长度
         * @param src_addr [输入输出] 传入期望协议族的地址对象（如 IPv4），函数返回时将被填充为数据发送方的地址
         * @param flags 接收标志位，默认 0
         * @return 成功返回读取的字节数；失败返回 -1
         */
        Task<ssize_t> recvFrom(void *buf, size_t len, Address::AddressPtr src_addr, int flags = 0);

        /**
         * @brief 以聚合 I/O 方式从远程主机接收数据并获取来源地址
         * @param buf iovec 数组指针
         * @param len iovec 数组元素个数
         * @param src_addr [输入输出] 传入期望协议族的地址对象（如 IPv4），函数返回时将被填充为数据发送方的地址
         * @param flags 接收标志位，默认 0
         * @return 成功返回读取的总字节数；失败返回 -1
         */
        Task<ssize_t> recvFrom(iovec *buf, size_t len, Address::AddressPtr src_addr, int flags = 0);

        // 带有超时的
        
        /**
         * @brief 接受一个客户端连接，返回封装好的 MSocket 对象
         * @return 成功时返回一个复用当前 socket 协议族、类型、协议的新连接 MSocket 对象；
         *         失败时返回 nullptr（通常可检查 errno）
         * @note 带有超时的
         */
        Task<std::shared_ptr<MSocket>> acceptT(uint64_t ms = 0);

        /**
         * @brief 向已连接的远程主机发送数据（TCP/SCTP 等）
         * @param buf 待发送数据的缓冲区
         * @param len 待发送数据的字节数
         * @param flags 发送标志位，如 MSG_DONTWAIT, MSG_NOSIGNAL 等，默认 0
         * @return 成功时返回实际发送的字节数；失败时返回 -1 并设置 errno
         * @note TCP 下发送成功仅表示数据进入内核缓冲区，不保证对方已收到
         * @note 带有超时的
         */
        Task<ssize_t> sendT(const void *buf, size_t len, int flags = 0, uint64_t ms = 0);

        /**
         * @brief 以聚合 I/O 方式向已连接的远程主机发送数据
         * @param bufs iovec 数组指针，每个元素指向一块独立缓冲区
         * @param len iovec 数组的元素个数，即 bufs 中有几块缓冲区
         * @param flags 发送标志位，默认 0
         * @return 成功时返回实际发送的总字节数；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> sendT(const iovec *bufs, size_t len, int flags = 0, uint64_t ms = 0);
        /**
         * @brief 向指定的目标地址发送数据（UDP 等非连接协议常用，也可用于连接后的 TCP）
         * @param buf 待发送数据的缓冲区
         * @param len 待发送数据的字节数
         * @param dest_addr 目标地址对象，需包含 IP 和端口
         * @param flags 发送标志位，默认 0
         * @return 成功返回实际发送的字节数；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> sendToT(const void *buf, size_t len, Address::AddressPtr dest_addr, int flags = 0, uint64_t ms = 0);

        /**
         * @brief 以聚合 I/O 方式向指定的目标地址发送数据
         * @param bufs iovec 数组指针
         * @param len iovec 数组的元素个数
         * @param dest_addr 目标地址对象
         * @param flags 发送标志位，默认 0
         * @return 成功返回实际发送的总字节数；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> sendToT(const iovec *bufs, size_t len, Address::AddressPtr dest_addr, int flags = 0, uint64_t ms = 0);

        /**
         * @brief 从已连接的远程主机接收数据
         * @param buf 接收缓冲区指针
         * @param len 缓冲区最多能容纳的字节数
         * @param flags 接收标志位，如 MSG_DONTWAIT, MSG_PEEK 等，默认 0
         * @return 成功时返回实际读取的字节数；对方已关闭连接时返回 0；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> recvT(void *buf, size_t len, int flags = 0, uint64_t ms = 0);

        /**
         * @brief 以聚合 I/O 方式从已连接的远程主机接收数据到多块缓冲区
         * @param buf iovec 数组指针，用于存放接收数据
         * @param len iovec 数组的元素个数
         * @param flags 接收标志位，默认 0
         * @return 成功返回读取的总字节数；对方关闭连接返回 0；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> recvT(iovec *buf, size_t len, int flags = 0, uint64_t ms = 0);

        /**
         * @brief 从远程主机接收数据并获取数据来源地址
         * @param buf 接收缓冲区指针
         * @param len 缓冲区最大长度
         * @param src_addr [输入输出] 传入期望协议族的地址对象（如 IPv4），函数返回时将被填充为数据发送方的地址
         * @param flags 接收标志位，默认 0
         * @return 成功返回读取的字节数；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> recvFromT(void *buf, size_t len, Address::AddressPtr src_addr, int flags = 0, uint64_t ms = 0);

        /**
         * @brief 以聚合 I/O 方式从远程主机接收数据并获取来源地址
         * @param buf iovec 数组指针
         * @param len iovec 数组元素个数
         * @param src_addr [输入输出] 传入期望协议族的地址对象（如 IPv4），函数返回时将被填充为数据发送方的地址
         * @param flags 接收标志位，默认 0
         * @return 成功返回读取的总字节数；失败返回 -1
         * @note 带有超时的
         */
        Task<ssize_t> recvFromT(iovec *buf, size_t len, Address::AddressPtr src_addr, int flags = 0, uint64_t ms = 0);
        
        /**
         * @brief 获取远程主机地址
         * @return 返回远程主机地址
         */
        std::shared_ptr<Address> getRemoteAddress();

        /**
         * @brief 从本主机获取地址
         * @return 返回本机的主机地址
         */
        std::shared_ptr<Address> getLocalAddress();

        /**
         * @brief 获取地址家族,比如AF_INET...
         * @return 返回地址家族
         */
        int getFamily() const { return m_family; }

        /**
         * @brief 获取socket类型,比如SOCK_STREAM
         * @return 返回socket类型
         */
        int getType() const { return m_type; }

        /**
         * @brief 获取socket协议
         * @return 返回socket协议
         */
        int getProtocol() const { return m_protocol; }

        /**
         * @brief 获取socket文件描述符
         * @return 返回socket文件描述符
         */
        int getSocketfd() const { return m_sockfd; }

        /**
         * @brief 判断socket是否成功连接
         * @return 成功返回true；失败返回 false
         */
        bool isConnected() const { return m_isConnected; }

        /**
         * @brief 判断socket描述符是否有效
         * @return 有效返回true；无效返回 false
         */
        bool isVaild() const;

        /**
         * @brief 获取socket上的错误
         * @return 返回一个整数值,不为0表示有错误
         */
        int getErrno();

        /**
         * @brief 将内容写入os流
         * @return ostream
         */
        std::ostream &dump(std::ostream &os) const;

        /**
         * @brief 转为string输出
         * @return string内容
         */
        std::string toString() const;

        /*
            取消accept事件
        */
        bool cancelAccept();

        /*
            取消read事件
        */
        bool cancelRead();

        /*
            取消write事件
        */
        bool cancelWrite();

        /*
            取消所有socket上事件
        */
        bool cancelAll();

    private:
        /**
         * @brief 初始化socket属性,TCP禁用nagle算法,socket本地地址重用
         * @return
         */
        void _initSocket();

        /**
         * @brief 重新设置一个新的sockfd(利用旧的family,type,protocol)
         * @return
         */
        void _newSocket();

        /**
         * @brief 初始化传入的fd,如果不是socketfd,那么不给予初始化
         * @return 成功返回true, 失败返回false
         */
        bool _init(int fd);

    private:
        int m_sockfd;
        int m_family;
        int m_type;
        int m_protocol;
        bool m_isConnected;

        std::shared_ptr<Address> m_localAddress;
        std::shared_ptr<Address> m_remoteAddress;
    };
}

#endif