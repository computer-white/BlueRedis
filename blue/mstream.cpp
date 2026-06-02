#include "mstream.h"
#include "blue/log.h"

// mstream
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    Task<ssize_t> MStream::readFixSize(void* buf, size_t len)
    {
        size_t offset = 0,l = len;
        while (l > 0)
        {
            ssize_t ret = co_await read((char*)(buf) + offset,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            offset += ret;
            l -= ret;
        }
        co_return len;
    }

    Task<ssize_t> MStream::readFixSize(ByteArray::ByteArrayPtr data, size_t len)
    {
        size_t l = len;
        while (l > 0)
        {
            ssize_t ret = co_await read(data,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            l -= ret;
        }
        co_return len;
    }

    Task<ssize_t> MStream::writeFixSize(const void* buf, size_t len)
    {
        size_t offset = 0,l = len;
        while (l > 0)
        {
            ssize_t ret = co_await write((const char*)(buf) + offset,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            offset += ret;
            l -= ret;
        }
        co_return len;

    }

    Task<ssize_t> MStream::writeFixSize(ByteArray::ByteArrayPtr data, size_t len)
    {
        size_t l = len;
        while (l > 0)
        {
            ssize_t ret = co_await write(data,l);
            if (ret <= 0)
            {
                co_return ret;
            }
            l -= ret;
        }
        co_return len;
    }
}
