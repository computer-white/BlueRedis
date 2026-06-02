#include "msocketstream.h"

// socket stream
namespace blue
{
    SocketStream::SocketStream(MSocket::MSocketPtr sock, bool owner)
    :m_sock(sock),
    m_owner(owner)
    {

    }

    SocketStream::~SocketStream()
    {
        if (m_owner && m_sock)
        {
            m_sock->close();
        }
    }

    bool SocketStream::isConnected() const
    {
        return m_sock && m_sock->isConnected();
    }

    Task<ssize_t> SocketStream::read(void *buf, size_t len)
    {
        if (!isConnected())
        {
            co_return -1;
        }
        auto res = co_await m_sock->recv(buf,len);
        co_return res;
    }

    Task<ssize_t> SocketStream::read(ByteArray::ByteArrayPtr data, size_t len)
    {
        if (!isConnected())
        {
            co_return -1;
        }
        std::vector<iovec> vec;
        data->getWriteBuffers(vec,len);
        ssize_t ret = co_await m_sock->recv(&vec[0],vec.size());
        if (ret > 0)
        {
            data->setSize(data->getSize() + ret);
            data->setPosition(data->getPosition() + ret);
        }
        co_return ret;
    }

    Task<ssize_t> SocketStream::write(const void *buf, size_t len)
    {
        if (!isConnected())
        {
            co_return -1;
        }
        auto res = co_await m_sock->send(buf,len);
        co_return res;
    }

    Task<ssize_t> SocketStream::write(ByteArray::ByteArrayPtr data, size_t len)
    {
        if (!isConnected())
        {
            co_return -1;
        }
        std::vector<iovec> vec;
        data->getReadBuffers(vec,len);
        ssize_t ret = co_await m_sock->send(&vec[0],vec.size());
        if (ret > 0)
        {
            // data->setSize(ret);
            data->setPosition(data->getPosition() + ret);
        }
        co_return ret;
    }

    void SocketStream::close()
    {
        if (!isConnected())
        {
            return;
        }
        m_sock->close();
    }
}