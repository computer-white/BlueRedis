#ifndef BLUE_MSOCKET_H
#define BLUE_MSOCKET_H
#include <memory>
#include <functional>
#include <unordered_set>
#include "address.h"
#include "blue/task.h"
#include "blue/resp_parser.h"

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

    public:
        using VersionChecker = std::function<uint64_t(const std::string &)>;
        void setVersionChecker(VersionChecker checker)
        {
            m_version_checker = checker;
        }

    private:
        struct WatchedKey;
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
         * @brief 设置为非阻塞socket fd
         */
        bool setNoBlocking();

        /**
         * @brief 设置为阻塞socket fd
         */
        bool setBlocking();

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
         * @brief shutdown关闭
         * @param how 关闭方式
         * @note SHUT_RD = No more receptions;
         * @note SHUT_WR = No more transmissions;
         * @note SHUT_RDWR = No more receptions or transmissions.
         */
        bool shutdown(int how);

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

        /**
         * @brief 设置有效的fd
         */
        bool setVaildFd();

        /**
         * @brief 设置为连接成功，并设置远端和本端地址
         */
        void setConnection() { m_isConnected = true; getRemoteAddress(); getLocalAddress(); }

        /**
         * @brief 获取客户端密码
         */
        const std::string &getClientPassword() const noexcept { return m_client_password; }

        /**
         * @brief 设置客户端密码
         * @param val 客户端密码
         */
        void setClientPassword(const std::string &val) { m_client_password = val; }

        /**
         * @brief 获取客户端级别
         */
        const int getClientlevel() const noexcept { return m_auth_level; }

        /**
         * @brief 设置客户端级别
         * @param val 级别
         */
        void setClientlevel(int val) noexcept { m_auth_level = val; }

        /**
         * @brief 获取客户端名称
         */
        const std::string &getClientName() const noexcept { return m_client_name; }

        /**
         * @brief 获取客户端数据库索引id
         */
        const int getClientId() const noexcept { return m_client_db_id; }

        /**
         * @brief 设置客户端名称
         * @param 客户端名称
         */
        void setClientName(const std::string &val) { m_client_name = val; }

        /**
         * @brief 设置客户端数据库索引id
         * @param val 客户端数据库索引id
         */
        void setClientId(int val) noexcept { m_client_db_id = val; }

        /**
         * @brief 开始redis事务
         * @note 如果在订阅模式，那么不会进入事务模式
         */
        bool beginTransaction() noexcept { if (m_in_subScription) { return false; } m_in_transaction = true; return true; }

        /**
         * @brief 是否在事务中
         * @return true 表示在事务中
         */
        bool inTransaction() const noexcept { return m_in_transaction; }

        /**
         * @brief 获取所有事务
         * @return 事务
         */
        const std::vector<std::vector<blue::RespValue>> &getTransaction() const { return m_transaction_cmds; }

        /**
         * @brief 添加一组事务
         * @param transaction 事务数组
         */
        void addTransaction(std::vector<blue::RespValue> transaction) { m_transaction_cmds.push_back(std::move(transaction)); }

        /**
         * @brief 清理事务
         */
        void clearTransaction()
        {
            m_transaction_cmds.clear();
            m_in_transaction = false;
        }

        /**
         * @brief 添加监视key,version
         * @param key 需要监视的key
         * @param version 需要监视的key当前的version
         */
        void addWatchKey(const std::string &key, uint64_t version)
        {
            m_watchedKeys.emplace_back(key, version);
        }

        /**
         * @brief 清空所有被监视的key
         */
        void clearWatchedKey() { m_watchedKeys.clear(); }

        /**
         * @brief key的版本是否被改变
         * @param key 需要检测的key
         * @param curr_version key当前的版本
         * @return true表示有修改,false 也可能表示没有监视这个key
         */
        bool isKeyModified(const std::string &key, uint64_t curr_version) const
        {
            bool modified = false;
            for (const auto &watchedkey : m_watchedKeys)
            {
                if (watchedkey.key == key && watchedkey.version != curr_version)
                {
                    modified = true;
                    break;
                }
            }
            return modified;
        }

        /**
         * @brief 是否有key的版本被改变
         * @param getversoin 获取version的函数
         * @return true表示有修改,false 也可能表示没有监视这个key
         */
        bool hasKeyModified() const
        {
            bool modified = false;
            for (const auto &watchedkey : m_watchedKeys)
            {
                if (watchedkey.version != m_version_checker(watchedkey.key))
                {
                    modified = true;
                    break;
                }
            }
            return modified;
        }

        /**
         * @brief 获取watchedKey
         */
        const std::vector<WatchedKey> &getWatchedKey() const noexcept { return m_watchedKeys; }

        /**
         * @brief 开始订阅模式
         * @note 如果在事务模式，那么不会进入订阅模式
         */
        bool beginSubScription() { if (m_in_transaction) { return false; } m_in_subScription = true; return true; }

        /**
         * @brief 退出订阅模式
         */
        bool endSubScription() { m_in_subScription = false; return true; }

        /**
         * @brief 是否在订阅模式
         */
        bool inSubScription() const noexcept { return m_in_subScription; }

        /**
         * @brief 添加订阅channel
         * @param channel 需要添加的channel
         */
        void addSubScriptionChannel(const std::string &channel) { m_subScription_channels.insert(channel); }

        /**
         * @brief 删除订阅channel
         * @param channel 需要删除的channel
         */
        void removeSubScriptionChannel(const std::string &channel) { m_subScription_channels.erase(channel); }

        /**
         * @brief 获取订阅channel
         */
        const std::unordered_set<std::string> &getSubScriptionChannels() const noexcept { return m_subScription_channels; }

        /**
         * @brief 添加订阅pattern
         * @param pattern 需要添加的pattern
         */
        void addSubScriptionPattern(const std::string &pattern) { m_subScription_patterns.insert(pattern); }

        /**
         * @brief 删除订阅channel
         * @param channel 需要删除的channel
         */
        void removeSubScriptionPattern(const std::string &pattern) { m_subScription_patterns.erase(pattern); }

        /**
         * @brief 获取订阅pattern
         */
        const std::unordered_set<std::string> &getSubScriptionPatterns() const noexcept { return m_subScription_patterns; }

        /**
         * @brief 清空订阅
         */
        void clearSubScription() { m_subScription_channels.clear(); m_subScription_patterns.clear(); m_in_subScription = false; }

        /**
         * @brief 设置监控模式
         */
        void setMonitorMode(bool enabled) noexcept { m_in_monitor = enabled; }

        /**
         * @brief 是否在监控模式
         */
        bool inMonitorMode() const noexcept { return m_in_monitor; }
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
        struct WatchedKey
        {
            std::string key;  // 被监视的key
            uint64_t version; // 被监视的key的值的版本
        };

        std::vector<WatchedKey> m_watchedKeys;

    private:
        // commandHandler使用
        std::string m_client_password = "client123";                  // 客户端密码 = "client123", 供commandHandler使用
        std::string m_client_name = "";                               // 客户端名称
        int m_auth_level = 0;                                         // 客户端级别, 供commandHandler使用, 0普通用户(没有密码不可以访问),1客户端(除了个别危险命令不可访问),2管理员(最高权,主要负责shutdown)
        int m_client_db_id = 0;                                       // 客户端数据库的索引

        // 事务模式
        bool m_in_transaction = false;                                // 事务是否进行中
        std::vector<std::vector<blue::RespValue>> m_transaction_cmds; // 事务命令数组
        VersionChecker m_version_checker;                             // 版本检查回调函数

        // 订阅模式
        bool m_in_subScription = false;                               // 是否处在订阅模式
        std::unordered_set<std::string> m_subScription_channels;      // 每个连接订阅的频道
        std::unordered_set<std::string> m_subScription_patterns;      // 每个连接订阅的模式订阅

        // 监控模式
        bool m_in_monitor = false;

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