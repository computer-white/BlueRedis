#include <netinet/tcp.h>
#include "fdmanager.h"
#include "hook.h"
#include "io_manager.h"
#include "log.h"
#include "macro.h"
#include "msocket.h"
#include "asyncio.h"

// socket 模块
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    
    std::shared_ptr<MSocket> MSocket::CreateTcp(std::shared_ptr<Address> address)
    {
        std::shared_ptr<MSocket> tcpsock = std::make_shared<MSocket>(address->getFamily(), TCP, 0);
        return tcpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateUdp(std::shared_ptr<Address> address)
    {
        std::shared_ptr<MSocket> udpsock = std::make_shared<MSocket>(address->getFamily(), UDP, 0);
        return udpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateTcpSocket()
    {
        std::shared_ptr<MSocket> ipv4tcpsock = std::make_shared<MSocket>(IPV4, TCP, 0);
        return ipv4tcpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateUdpSocket()
    {
        std::shared_ptr<MSocket> ipv4udpsock = std::make_shared<MSocket>(IPV4, UDP, 0);
        return ipv4udpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateTcpSocket6()
    {
        std::shared_ptr<MSocket> ipv6tcpsock = std::make_shared<MSocket>(IPV6, TCP, 0);
        return ipv6tcpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateUdpSocket6()
    {
        std::shared_ptr<MSocket> ipv6udpsock = std::make_shared<MSocket>(IPV6, UDP, 0);
        return ipv6udpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateUnixTcpSocket()
    {
        std::shared_ptr<MSocket> unixtcpsock = std::make_shared<MSocket>(UNIX, TCP, 0);
        return unixtcpsock;
    }

    std::shared_ptr<MSocket> MSocket::CreateUnixUdpSocket()
    {
        std::shared_ptr<MSocket> unixudpsock = std::make_shared<MSocket>(UNIX, UDP, 0);
        return unixudpsock;
    }

    MSocket::MSocket(int domain, int type, int protocol)
        : m_sockfd(-1),
          m_family(domain),
          m_type(type),
          m_protocol(protocol),
          m_isConnected(false)
    {
    }

    MSocket::~MSocket()
    {
        close();
    }

    bool MSocket::_getOption(int level, int option_name,
                             void *option_val, socklen_t *option_len)
    {
        int ret = getsockopt(m_sockfd, level,
                             option_name, option_val, option_len);
        if (ret)
        {
            BLUE_LOG_DEBUGE(g_logger) << "getsockopt(" << m_sockfd
                                      << "," << level << ","
                                      << option_name << "," << option_val
                                      << "," << option_len << ",errno : "
                                      << errno << ",strerrno : " << strerror(errno);
            return false;
        }
        return true;
    }

    bool MSocket::_setOption(int level, int option_name,
                             const void *option_val, socklen_t option_len)
    {
        int ret = setsockopt(m_sockfd, level,
                             option_name, option_val, option_len);
        if (ret)
        {
            BLUE_LOG_DEBUGE(g_logger) << "setsockopt(" << m_sockfd
                                      << "," << level << ","
                                      << option_name << "," << option_val
                                      << "," << option_len << ",errno : "
                                      << errno << ",strerrno : " << strerror(errno);
            return false;
        }
        return true;
    }

    bool MSocket::setNoBlocking()
    {
        if (isVaild())
        {
            int flags = fcntl(m_sockfd,F_GETFL,0);
            if (flags == -1)
            {
                return false;
            }
            return fcntl(m_sockfd,F_SETFL,flags | O_NONBLOCK) == 0;
        }
        return false;
    }

    bool MSocket::setBlocking()
    {
        if (isVaild())
        {
            int flags = fcntl(m_sockfd, F_GETFL,0);
            if (flags == -1)
            {
                return false;
            }
            return fcntl(m_sockfd,F_SETFL,flags & ~O_NONBLOCK) == 0;
        }
        return false;
    }

    int64_t MSocket::getSendTimeout() const
    {
        blue::FdCxt::FdCxtPtr cxt = FdManagerPtr::GetInstance()->get(m_sockfd);
        if (cxt)
        {
            return cxt->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }

    void MSocket::setSendTimeout(int64_t val)
    {
        struct timeval tv;
        tv.tv_sec = val / 1000u;
        tv.tv_usec = (val % 1000u) * 1000u;
        bool res = setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
        if (BLUE_LIKELY(!res))
        {
            BLUE_LOG_ERROR(g_logger) << "setOption(SOL_SOCKET,SO_SNDTIMEO,tv) failed";
        }
        return;
    }

    int64_t MSocket::getRecvTimeout() const
    {
        blue::FdCxt::FdCxtPtr cxt = FdManagerPtr::GetInstance()->get(m_sockfd);
        if (cxt)
        {
            return cxt->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }

    void MSocket::setRecvTimeout(int64_t val)
    {
        struct timeval tv;
        tv.tv_sec = val / 1000u;
        tv.tv_usec = (val % 1000u) * 1000u;
        bool res = setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
        if (BLUE_LIKELY(!res))
        {
            BLUE_LOG_ERROR(g_logger) << "setOption(SOL_SOCKET,SO_RCVTIMEO,tv) failed";
        }
        return;
    }

    Task<std::shared_ptr<MSocket>> MSocket::accept()
    {
        // 复用当前的socket的family,type,protocol
        std::shared_ptr<MSocket> newsockfd =
            std::make_shared<MSocket>(m_family, m_type, m_protocol);

        int connfd = co_await Accept(m_sockfd, nullptr, nullptr);
        if (connfd == -1)
        {
            BLUE_LOG_ERROR(g_logger) << "::accpet(" << m_sockfd
                                     << "),errno : " << errno << ",strerrno : "
                                     << strerror(errno);
            co_return nullptr;
        }
        if (connfd >= 0)
        {
            FdManagerPtr::GetInstance()->get(connfd,true);      // 没有了hook需要我们手动来设置
        }
        if (newsockfd->_init(connfd))
        {
            co_return newsockfd;
        }
        co_return nullptr;
    }

    bool MSocket::bind(const Address::AddressPtr address)
    {
        if (!isVaild())
        {
            _newSocket();
            if (BLUE_UNLIKELY(!isVaild()))
            {
                return false;
            }
        }
        if (BLUE_UNLIKELY(address->getFamily() != m_family))
        {
            BLUE_LOG_ERROR(g_logger) << "bind m_sock.family : ("
                                     << m_family << ")address.family : ("
                                     << address->getFamily() << ")not equal,address : "
                                     << address->toString();
            return false;
        }

        if (::bind(m_sockfd, address->getAddr(), address->getAddrLen()))
        {
            BLUE_LOG_ERROR(g_logger) << "::bind error,addr : "
                                     << address->toString() << "errno : " << errno
                                     << ",strerrno : " << strerror(errno);
            return false;
        }
        getLocalAddress();
        return true;
    }

    bool MSocket::listen(int backlog)
    {
        if (!isVaild())
        {
            BLUE_LOG_ERROR(g_logger) << "listen error sockfd = -1";
            return false;
        }
        if (::listen(m_sockfd, backlog))
        {
            BLUE_LOG_ERROR(g_logger) << "listen error,errno : "
                                     << errno << "strerrno : "
                                     << strerror(errno);
            return false;
        }
        return true;
    }

    Task<bool> MSocket::connect(const Address::AddressPtr address, uint32_t timeout)
    {
        if (!isVaild())
        {
            _newSocket();
            if (BLUE_UNLIKELY(!isVaild()))
            {
                co_return false;
            }
        }
        if (BLUE_UNLIKELY(address->getFamily() != m_family))
        {
            BLUE_LOG_ERROR(g_logger) << "connect m_sock.family : ("
                                     << m_family << ")address.family : ("
                                     << address->getFamily() << ")not equal,address : "
                                     << address->toString();
            co_return false;
        }
        if (timeout == UINT32_MAX)
        {
            ssize_t conn = co_await Connect(m_sockfd, address->getAddr(), address->getAddrLen());
            if (conn)
            {
                BLUE_LOG_ERROR(g_logger) << "::connect error,addr : "
                                         << address->toString() << ",errno : " << errno
                                         << ",strerrno : " << strerror(errno);
                close();
                co_return false;
            }
        }
        else
        {

            ssize_t conn = co_await ConnectT(m_sockfd, address->getAddr(), address->getAddrLen(), timeout);
            if (conn)
            {
                BLUE_LOG_ERROR(g_logger) << "::connect error,addr : "
                                         << address->toString() << "timeout : "
                                         << timeout << ",errno : " << errno
                                         << ",strerrno : " << strerror(errno);
                close();
                co_return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        co_return true;
    }

    bool MSocket::close()
    {
        // 已经关闭或没有正确初始化
        if (!m_isConnected && m_sockfd == -1)
        {
            return true;
        }
        m_isConnected = false;
        if (m_sockfd != -1)
        {
            ::close(m_sockfd);
            m_sockfd = -1;
            return true;
        }
        return false;
    }

    bool MSocket::shutdown(int how)
    {
        if (isVaild())
        {
            int ret = ::shutdown(m_sockfd,how);
            return ret == 0;
        }
        return false;
    }

    bool MSocket::isVaild() const
    {
        return m_sockfd != -1;
    }

    Task<ssize_t> MSocket::send(const iovec *buf, size_t len, int flags)
    {
        if (isConnected())
        {
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buf;
            msg.msg_iovlen = len;
            ssize_t ret = co_await Sendmsg(m_sockfd, &msg, flags);
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::send(const void *buf, size_t len, int flags)
    {
        if (isConnected())
        {
            ssize_t ret = co_await Send(m_sockfd, buf, len, flags);
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::sendTo(const iovec *buf, size_t len,
                            Address::AddressPtr dest_addr, int flags)
    {
        if (isConnected())
        {
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buf;
            msg.msg_iovlen = len;
            msg.msg_name = dest_addr->getAddr();
            msg.msg_namelen = dest_addr->getAddrLen();
            ssize_t ret = co_await Sendmsg(m_sockfd, &msg, flags);
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::sendTo(const void *buf, size_t len,
                            Address::AddressPtr dest_addr, int flags)
    {
        if (isConnected())
        {
            ssize_t ret = co_await Sendto(m_sockfd, buf, len, flags, 
                                dest_addr->getAddr(), dest_addr->getAddrLen());
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::recv(iovec *buf, size_t len, int flags)
    {
        if (isConnected())
        {
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buf;
            msg.msg_iovlen = len;
            ssize_t ret = co_await Recvmsg(m_sockfd, &msg, flags);
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::recv(void *buf, size_t len, int flags)
    {
        if (isConnected())
        {
            ssize_t ret = co_await Recv(m_sockfd, buf, len, flags);
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::recvFrom(iovec *buf, size_t len,
                                    Address::AddressPtr src_addr, int flags)
    {
        if (isConnected())
        {
            struct msghdr msg;
            msg.msg_iov = (iovec *)buf;
            msg.msg_iovlen = len;
            msg.msg_name = src_addr->getAddr();
            msg.msg_namelen = src_addr->getAddrLen();
            ssize_t ret = co_await Recvmsg(m_sockfd, &msg, flags);
            co_return ret;
        }
        co_return -1;
    }

    Task<ssize_t> MSocket::recvFrom(void *buf, size_t len,
                              Address::AddressPtr src_addr, int flags)
    {
        if (isConnected())
        {
            socklen_t length = src_addr->getAddrLen();
            ssize_t ret = co_await Recvfrom(m_sockfd,buf,len,flags,
                                    src_addr->getAddr(),&length);
            co_return ret;
        }
        co_return -1;
    }

    Task<std::shared_ptr<MSocket>> MSocket::acceptT(uint64_t ms)
    {
        if (ms == 0)
        {
            auto res = co_await accept();
            co_return res;
        }
        std::shared_ptr<MSocket> newsockfd =
        std::make_shared<MSocket>(m_family, m_type, m_protocol);

        int connfd = co_await AcceptT(m_sockfd, nullptr, nullptr, ms);
        if (connfd == -1)
        {
            // 超时不显示
            if (errno == ETIMEDOUT)
            {
                co_return nullptr;
            }
            BLUE_LOG_ERROR(g_logger) << "::accpet(" << m_sockfd
                                    << "),errno : " << errno << ",strerrno : "
                                    << strerror(errno);
            co_return nullptr;
        }
        if (connfd >= 0)
        {
            FdManagerPtr::GetInstance()->get(connfd,true);      // 没有了hook需要我们手动来设置
        }
        if (newsockfd->_init(connfd))
        {
            co_return newsockfd;
        }
        co_return nullptr;
    }
    Task<ssize_t> MSocket::sendT(const void *buf, size_t len, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await send(buf, len, flags);
            co_return ret;
        }
        if (isConnected())
        {
            ssize_t ret = co_await SendT(m_sockfd, buf, len, flags, ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::sendT(const iovec *bufs, size_t len, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await send(bufs, len, flags);
            co_return ret;
        }
        if (isConnected())
        {
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)bufs;
            msg.msg_iovlen = len;
            ssize_t ret = co_await SendmsgT(m_sockfd, &msg, flags, ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::sendToT(const void *buf, size_t len, Address::AddressPtr dest_addr, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await sendTo(buf,len,dest_addr,flags);
            co_return ret;
        }
        if (isConnected())
        {
            ssize_t ret = co_await SendtoT(m_sockfd, buf, len, flags, 
                                dest_addr->getAddr(), dest_addr->getAddrLen(),ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::sendToT(const iovec *bufs, size_t len, Address::AddressPtr dest_addr, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await sendTo(bufs,len,dest_addr,flags);
            co_return ret;
        }
        if (isConnected())
        {
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)bufs;
            msg.msg_iovlen = len;
            msg.msg_name = dest_addr->getAddr();
            msg.msg_namelen = dest_addr->getAddrLen();
            ssize_t ret = co_await SendmsgT(m_sockfd, &msg, flags,ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::recvT(void *buf, size_t len, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await recv(buf, len, flags);
            co_return ret;
        }
        if (isConnected())
        {
            ssize_t ret = co_await RecvT(m_sockfd, buf, len, flags, ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::recvT(iovec *buf, size_t len, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await recv(buf,len,flags);
            co_return ret;
        }
        if (isConnected())
        {
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buf;
            msg.msg_iovlen = len;
            ssize_t ret = co_await RecvmsgT(m_sockfd, &msg, flags,ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::recvFromT(void *buf, size_t len, Address::AddressPtr src_addr, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await recvFrom(buf,len,src_addr,flags);
            co_return ret;
        }
        if (isConnected())
        {
            socklen_t length = src_addr->getAddrLen();
            ssize_t ret = co_await RecvfromT(m_sockfd,buf,len,flags,
                                    src_addr->getAddr(),&length, ms);
            co_return ret;
        }
        co_return -1;
    }
    Task<ssize_t> MSocket::recvFromT(iovec *buf, size_t len, Address::AddressPtr src_addr, int flags, uint64_t ms)
    {
        if (ms == 0)
        {
            ssize_t ret = co_await recvFrom(buf,len,src_addr,flags);
            co_return ret;
        }
        if (isConnected())
        {
            struct msghdr msg;
            msg.msg_iov = (iovec *)buf;
            msg.msg_iovlen = len;
            msg.msg_name = src_addr->getAddr();
            msg.msg_namelen = src_addr->getAddrLen();
            ssize_t ret = co_await RecvmsgT(m_sockfd, &msg, flags, ms);
            co_return ret;
        }
        co_return -1;
    }

    std::shared_ptr<Address> MSocket::getRemoteAddress()
    {
        if (m_remoteAddress)
        {
            return m_remoteAddress;
        }
        std::shared_ptr<Address> result;
        switch (m_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknowAddress(m_family));
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        // getpeername会将远端实际的地址存放在addr参数(第二个参数)中,将长度存放在len(第三个参数中)
        if (getpeername(m_sockfd, result->getAddr(), &addrlen))
        {
            BLUE_LOG_ERROR(g_logger) << "getpeername error,sockfd : " << m_sockfd
                                     << ",errno : " << errno << ",strerrno : "
                                     << strerror(errno);
            return std::shared_ptr<Address>(new UnknowAddress(m_family));
        }
        if (m_family == AF_UNIX)
        {
            UnixAddress::UnixAddressPtr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrlen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    std::shared_ptr<Address> MSocket::getLocalAddress()
    {
        if (m_localAddress)
        {
            return m_localAddress;
        }

        std::shared_ptr<Address> result;
        switch (m_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknowAddress(m_family));
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        // getpeername会将远端实际的地址存放在addr参数(第二个参数)中,将长度存放在len(第三个参数中)
        if (getsockname(m_sockfd, result->getAddr(), &addrlen))
        {
            BLUE_LOG_ERROR(g_logger) << "getsockname error,sockfd : " << m_sockfd
                                     << ",errno : " << errno << ",strerrno : "
                                     << strerror(errno);
            return std::shared_ptr<Address>(new UnknowAddress(m_family));
        }
        if (m_family == AF_UNIX)
        {
            UnixAddress::UnixAddressPtr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrlen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    int MSocket::getErrno()
    {
        int error = 0;
        socklen_t optionlen = sizeof(error);
        // 清除错误并将错误值传入error,如果error有值表示有错误
        if (!_getOption(SOL_SOCKET, SO_ERROR, &error, &optionlen))
        {
            // 出错
            return -1;
        }
        // error不为0,表示有错误
        return error;
    }

    std::ostream &MSocket::dump(std::ostream &os) const
    {
        os << "[Socket socketfd : " << m_sockfd
           << ",family : " << m_family
           << ",type : " << m_type
           << ",protocl : " << m_protocol
           << ",isconnected : " << m_isConnected;
        if (m_localAddress)
        {
            os << ",localAddress : " << m_localAddress->toString();
        }
        if (m_remoteAddress)
        {
            os << ",remoteAddress : " << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    std::string MSocket::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    bool MSocket::cancelAccept()
    {
        return blue::IOManager::GetThis()->cancelEvent(m_sockfd, blue::IOManager::Event::READ);
    }

    bool MSocket::cancelRead()
    {
        return blue::IOManager::GetThis()->cancelEvent(m_sockfd, blue::IOManager::Event::READ);
    }

    bool MSocket::cancelWrite()
    {
        return blue::IOManager::GetThis()->cancelEvent(m_sockfd, blue::IOManager::Event::WRITE);
    }

    bool MSocket::cancelAll()
    {
        return blue::IOManager::GetThis()->cancelAll(m_sockfd);
    }

    bool MSocket::setVaildFd()
    {
        if (!isVaild())
        {
            _newSocket();
            if (BLUE_LIKELY(!isVaild()))
            {
                return false;
            }
        }
        return true;
    }

    void MSocket::_initSocket()
    {
        int val = 1; // 1表示启用
        setOption(SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, val);
        if (m_type == SOCK_STREAM)
        {
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }

    void MSocket::_newSocket()
    {
        m_sockfd = socket(m_family, m_type, m_protocol);
        if (BLUE_LIKELY(m_sockfd != -1))
        {
            _initSocket();
        }
        else
        {
            BLUE_LOG_ERROR(g_logger) << "socket(" << m_family << ","
                                     << m_type << "," << m_protocol
                                     << ") error,errno : " << errno
                                     << ",strerrno : " << strerror(errno);
        }
    }

    bool MSocket::_init(int fd)
    {
        blue::FdCxt::FdCxtPtr cxt = FdManagerPtr::GetInstance()->get(fd);
        BLUE_LOG_INFO(g_logger) << "cxt : " << cxt;
        if (cxt && cxt->isSocket() && !cxt->isClosed())
        {
            m_sockfd = fd;
            m_isConnected = true;
            _initSocket();
            getRemoteAddress();
            getLocalAddress();
            return true;
        }
        return false;
    }

}