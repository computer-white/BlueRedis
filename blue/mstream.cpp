#include "mstream.h"
#include "blue/log.h"

// mstream
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    ssize_t MStream::readFixSize(void* buf, size_t len)
    {
        size_t offset = 0,l = len;
        while (l > 0)
        {
            ssize_t ret = read((char*)(buf) + offset,l);
            if (ret <= 0)
            {
                return ret;
            }
            offset += ret;
            l -= ret;
        }
        return len;
    }

    ssize_t MStream::readFixSize(ByteArray::ByteArrayPtr data, size_t len)
    {
        size_t l = len;
        while (l > 0)
        {
            ssize_t ret = read(data,l);
            if (ret <= 0)
            {
                return ret;
            }
            l -= ret;
        }
        return len;
    }

    ssize_t MStream::writeFixSize(const void* buf, size_t len)
    {
        size_t offset = 0,l = len;
        while (l > 0)
        {
            ssize_t ret = write((const char*)(buf) + offset,l);
            if (ret <= 0)
            {
                return ret;
            }
            offset += ret;
            l -= ret;
        }
        return len;

    }

    ssize_t MStream::writeFixSize(ByteArray::ByteArrayPtr data, size_t len)
    {
        size_t l = len;
        while (l > 0)
        {
            ssize_t ret = write(data,l);
            if (ret <= 0)
            {
                return ret;
            }
            l -= ret;
        }
        return len;
    }
}
