#include <exception>
#include <fstream>
#include <iomanip>
#include <string.h>
#include "bytearray.h"
#include "log.h"
#include "macro.h"
#include "mendian.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
// 字节流输出
namespace blue
{
    ByteArray::Node::Node(size_t s)
        : ptr(new char[s]),
          next(nullptr),
          pre(nullptr),
          size(s)
    {
    }

    ByteArray::Node::Node(size_t s, Node *next, Node *pre)
        : ptr(new char[s]),
          next(next),
          pre(pre),
          size(s)
    {
    }

    ByteArray::Node::~Node()
    {
        if (ptr)
        {
            delete[] ptr;
        }
    }

    // 32位 Varint + Zigzag编码
    static uint32_t EncodeZigzag(int32_t val)
    {
        /*
            Varint + Zigzag编码,主要是为了使负数写入流的字节变小
            正数 -> 2*val
            负数 -> 2*(-val) - 1
            -1 = 111...111 -> (-2 = 111...110 ^ 111...111) = 000...001 = 1
        */
        // 如果是负数那么异或一个全1
        return (val << 1) ^ (val >> 31);
    }

    // 64位 Varint + Zigzag编码
    static uint64_t EncodeZigzag64(int64_t val)
    {
        return (val << 1) ^ (val >> 63);
    }

    // 32位 Varint + ZigZag 解码
    static int32_t DecodeZigzag(uint32_t val)
    {
        /*
            首先除以二,然后根据编码后的值我们发现
            val & 1 == 1，那么原来的值为负数
            val & 1 == 0，那么原来的值为正数
            x ^ 0 = x (x > 0)
            x ^ -1 = -x-1 (x > 0)
            那么根据这个恒等式，我们可以正确解码
        */
        return (val >> 1) ^ (-static_cast<int32_t>(val & 1));
    }

    // 64位 Varint + ZigZag 解码
    static int64_t DecodeZigzag64(uint64_t val)
    {
        return (val >> 1) ^ (-static_cast<int64_t>(val & 1));
    }

    ByteArray::ByteArray(size_t base_size)
        : m_root(new Node(base_size)),
          m_curr(m_root),
          m_baseSize(base_size),
          m_capacity(base_size),
          m_position(0),
          m_size(0),
          m_endian(BLUE_BIG_ENDIAN)
    {
    }

    ByteArray::~ByteArray()
    {
        Node *tem = m_root;
        while (tem)
        {
            m_curr = tem;
            tem = tem->next;
            delete m_curr;
        }
    }

    bool ByteArray::isLittleEndian() const
    {
        return m_endian == BLUE_LITTLE_ENDIAN;
    }

    void ByteArray::setLittleEndian(bool val)
    {
        m_endian = val ? BLUE_LITTLE_ENDIAN : BLUE_BIG_ENDIAN;
    }

    void ByteArray::writeFint8(int8_t val)
    {
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFuint8(uint8_t val)
    {
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFint16(int16_t val)
    {
        if (m_endian != BLUE_BYTE_ORDER)
        {
            val = bitswap(val);
        }
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFuint16(uint16_t val)
    {
        if (m_endian != BLUE_BYTE_ORDER)
        {
            val = bitswap(val);
        }
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFint32(int32_t val)
    {
        if (m_endian != BLUE_BYTE_ORDER)
        {
            val = bitswap(val);
        }
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFuint32(uint32_t val)
    {
        if (m_endian != BLUE_BYTE_ORDER)
        {
            val = bitswap(val);
        }
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFint64(int64_t val)
    {
        if (m_endian != BLUE_BYTE_ORDER)
        {
            val = bitswap(val);
        }
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeFuint64(uint64_t val)
    {
        if (m_endian != BLUE_BYTE_ORDER)
        {
            val = bitswap(val);
        }
        write(reinterpret_cast<const char *>(&val), sizeof(val));
    }

    void ByteArray::writeInt32(int32_t val)
    {
        writeUint32(EncodeZigzag(val));
    }

    void ByteArray::writeUint32(uint32_t val)
    {
        uint8_t tem[5];
        uint8_t i = 0;
        while (val >= 0x80)
        {
            tem[i++] = (val & 0x7F) | 0x80; // | 0x80加上标志位，方便反编码回来
            val >>= 7;
        }
        tem[i++] = val;
        write(reinterpret_cast<const char *>(tem), i);
    }

    void ByteArray::writeInt64(int64_t val)
    {
        writeUint64(EncodeZigzag64(val));
    }

    void ByteArray::writeUint64(uint64_t val)
    {
        uint8_t tem[10];
        uint8_t i = 0;
        while (val >= 0x80)
        {
            tem[i++] = (val & 0x7F) | 0x80; // | 0x80加上标志位，方便反编码回来
            val >>= 7;
        }
        tem[i++] = val;
        write(reinterpret_cast<const char *>(tem), i);
    }

    void ByteArray::writeFloat(float val)
    {
        uint32_t tem;
        memcpy(&tem, &val, sizeof(tem));
        writeUint32(tem);
    }

    void ByteArray::writeDouble(double val)
    {
        uint64_t tem;
        memcpy(&tem, &val, sizeof(tem));
        writeUint64(tem);
    }

    void ByteArray::writeString16(const std::string &val)
    {
        writeFuint16(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeString32(const std::string &val)
    {
        writeFuint32(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeString64(const std::string &val)
    {
        writeFuint64(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeStringVint(const std::string &val)
    {
        writeUint64(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeStringLength(const std::string &val)
    {
        write(val.c_str(), val.size());
    }

    int8_t ByteArray::readFint8()
    {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint8_t ByteArray::readFuint8()
    {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }
#define XX(m_endian, type)           \
    type v;                          \
    read(&v, sizeof(v));             \
    if (m_endian == BLUE_BYTE_ORDER) \
    {                                \
        return v;                    \
    }                                \
    return bitswap(v);

    int16_t ByteArray::readFint16()
    {
        XX(m_endian, int16_t);
    }

    uint16_t ByteArray::readFuint16()
    {
        XX(m_endian, uint16_t);
    }

    int32_t ByteArray::readFint32()
    {
        XX(m_endian, int32_t);
    }

    uint32_t ByteArray::readFuint32()
    {
        XX(m_endian, uint32_t);
    }

    int64_t ByteArray::readFint64()
    {
        XX(m_endian, int64_t);
    }

    uint64_t ByteArray::readFuint64()
    {
        XX(m_endian, uint64_t);
    }
#undef XX

    int32_t ByteArray::readInt32()
    {
        uint32_t uv = readUint32();
        return DecodeZigzag(uv);
    }

    uint32_t ByteArray::readUint32()
    {
        uint32_t v = 0;
        for (uint8_t i = 0;; i++)
        {
            uint8_t tem = readFuint8();
            v |= static_cast<uint32_t>((tem & 0x7f)) << (7 * i);
            if (!(tem & 0x80))
                break;
        }
        return v;
    }

    int64_t ByteArray::readInt64()
    {
        uint64_t uv = readUint64();
        return DecodeZigzag64(uv);
    }

    uint64_t ByteArray::readUint64()
    {
        uint64_t v = 0;
        for (uint8_t i = 0;; i++)
        {
            uint8_t tem = readFuint8();
            v |= static_cast<uint64_t>((tem & 0x7f)) << (7 * i);
            if (!(tem & 0x80))
                break;
        }
        return v;
    }

    float ByteArray::readFloat()
    {
        uint32_t v = readUint32();
        float val;
        memcpy(&val, &v, sizeof(val));
        return val;
    }

    double ByteArray::readDouble()
    {
        uint64_t v = readUint64();
        double val;
        memcpy(&val, &v, sizeof(val));
        return val;
    }

    std::string ByteArray::readString16()
    {
        uint16_t len = readFuint16();
        std::string v;
        v.resize(len);
        read(&v[0], len);
        return v;
    }

    std::string ByteArray::readString32()
    {
        uint32_t len = readFuint32();
        std::string v;
        v.resize(len);
        read(&v[0], len);
        return v;
    }

    std::string ByteArray::readString64()
    {
        uint64_t len = readFuint64();
        std::string v;
        v.resize(len);
        read(&v[0], len);
        return v;
    }

    std::string ByteArray::readStringVint()
    {
        uint64_t len = readFuint64();
        std::string v;
        v.resize(len);
        read(&v[0], len);
        return v;
    }

    void ByteArray::clear()
    {
        m_position = m_size = 0;
        m_capacity = m_baseSize;
        Node *tem = m_root->next;
        while (tem)
        {
            m_curr = tem;
            tem = tem->next;
            delete m_curr;
        }
        m_curr = m_root;
        m_curr->next = nullptr;
    }

    void ByteArray::write(const char *buf, size_t size)
    {
        if (size == 0)
        {
            return;
        }
        // 先增加便利签的容量
        addCapacity(size);
        // 在当前页的哪一行
        size_t npos = m_position % m_baseSize;
        // 这一页还可以写入多少行
        size_t ncap = m_curr->size - npos;
        // buf已经写入多少
        size_t bpos = 0;

        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy(m_curr->ptr + npos, buf + bpos, size);
                if (m_curr->size == npos + size)
                {
                    m_curr = m_curr->next; // 换到下一页,可能为空
                }
                m_position += size;
                size = 0;
            }
            else
            {
                memcpy(m_curr->ptr + npos, buf + bpos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_curr = m_curr->next; // 换到下一页
                ncap = m_curr->size;   // 新的一页可以写入的大小
                npos = 0;              // 新的一页从头开始
            }
        }
        if (m_position > m_size)
        {
            m_size = m_position;
        }
    }

    void ByteArray::read(void *buf, size_t size)
    {
        if (size > getReadSize())
        {
            std::__throw_out_of_range("not enough len");
        }
        // 在当前页的哪一行
        size_t npos = m_position % m_baseSize;
        // 这一页还可以读多少行
        size_t ncap = m_curr->size - npos;
        // 已经向buf写入多少
        size_t bpos = 0;
        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy((char *)buf + bpos, m_curr->ptr + npos, size);
                if (npos + size == m_curr->size)
                {
                    m_curr = m_curr->next; // 这一页读完了
                }
                m_position += size;
                size = 0;
            }
            else
            {
                memcpy((char *)buf + bpos, m_curr->ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_curr = m_curr->next;
                ncap = m_curr->size;
                npos = 0;
            }
        }
    }

    void ByteArray::read(void *buf, size_t size, size_t position) const
    {
        if (size > m_size - position)
        {
            std::__throw_out_of_range("not enough len");
        }
        Node *curr = m_root;
        size_t tem = position;
        while (curr && tem >= curr->size)
        {
            tem -= curr->size;
            curr = curr->next;
        }
        if (BLUE_UNLIKELY(!curr))
        {
            BLUE_LOG_ERROR(g_logger) << "没有内容可以读, position : " << position;
            return;
        }
        // 在当前页的哪一行
        size_t npos = position % m_baseSize;
        // 这一页还可以读多少行
        size_t ncap = curr->size - npos;
        // 已经向buf写入多少
        size_t bpos = 0;
        while (size > 0)
        {
            if (ncap >= size)
            {
                memcpy((char *)buf + bpos, curr->ptr + npos, size);
                if ((position + size) % m_size == 0)
                {
                    curr = curr->next; // 这一页读完了
                }
                position += size;
                size = 0;
            }
            else
            {
                memcpy((char *)buf + bpos, curr->ptr + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                if (!curr->next)
                {
                    BLUE_LOG_ERROR(g_logger) << "no data";
                    return;
                }
                curr = curr->next;
                ncap = curr->size;
                npos = 0;
            }
        }
    }

    void ByteArray::setPosition(size_t v)
    {
        if (v > m_size)
        {
            std::__throw_out_of_range("setPosition out of range");
        }
        // v <= m_size 所以最后除了特殊情况m_curr一定指向正确位置
        m_position = v;
        m_curr = m_root;
        // 需要判断相等,
        // 因为当v等于m_curr->size整数倍时我们希望m_curr可以指向m_position所在的下一页
        // 这样在上面去读写的时候即使m_position % m_baseSize == 0也会从正确页数开始读或写
        // 而不是在m_position所在的页上面重新覆盖
        size_t tem = v;
        while (m_curr && tem >= m_curr->size)
        {
            tem -= m_curr->size;
            m_curr = m_curr->next;
        }
        if (tem > 0) // v不是m_curr->size 或 m_BaseSize的整数倍
        {
        }
        else if (tem == 0) // v 是m_curr->size的整数倍
        {
            if (v == m_size) // v == m_size 说明需要开辟新的一页
            {
                if (m_curr && !m_curr->next)
                {
                    m_curr->next = new Node(m_baseSize, nullptr, m_curr);
                    m_capacity += m_baseSize;
                    m_curr = m_curr->next;
                }
            }
        }
        // if (v % m_baseSize == 0)
        // {
        //     if (m_curr && !m_curr->next)
        //     {
        //         m_curr->next = new Node(m_baseSize);
        //         m_capacity += m_baseSize;
        //         m_curr = m_curr->next;
        //     }
        // }
    }

    bool ByteArray::writeToFile(const std::string &name) const
    {
        std::ofstream ofs;
        ofs.open(name,std::ios_base::trunc | std::ios_base::binary);
        if (BLUE_UNLIKELY(!ofs))
        {
            BLUE_LOG_ERROR(g_logger) << "writeToFile, file name : " << name
                                     << " errno : " << errno << "strerror : "
                                     << strerror(errno);
            return false;
        }

        size_t readsize = getReadSize();
        size_t position = m_position;
        Node *tem = m_curr;
        while (tem && readsize > 0)
        {
            size_t npos = position % m_baseSize;
            size_t ncap = tem->size - npos;
            size_t len = readsize < ncap ? readsize : ncap;
            ofs.write(tem->ptr + npos, len);
            tem = tem->next;
            readsize -= len;
            position += len;
        }
        return readsize <= 0;
    }

    bool ByteArray::readFromFile(const std::string &name)
    {
        std::ifstream ifs;
        ifs.open(name, std::ios_base::binary);
        if (BLUE_UNLIKELY(!ifs))
        {
            BLUE_LOG_ERROR(g_logger) << "readFromFile, file name : " << name
                                     << " errno : " << errno << "strerror : "
                                     << strerror(errno);
            return false;
        }

        std::shared_ptr<char> buf(new char[m_baseSize], [](char *ptr)
                                  { delete[] ptr; });
        while (!ifs.eof())
        {
            ifs.read(buf.get(), m_baseSize);
            write((const char *)buf.get(), ifs.gcount());
        }
        return true;
    }

    void ByteArray::addCapacity(size_t size)
    {
        int remain = getCapacity();
        if (size == 0 || remain >= size)
        {
            return;
        }
        size -= remain;
        // 还需要加上多少个节点
        size_t count = (size + m_baseSize - 1) / m_baseSize;

        // 找链表末尾，同时记录最后一个有效节点
        Node *tail = m_curr ? m_curr : m_root; // 需保证m_root一定不能为空
        while (tail && tail->next)
        {
            tail = tail->next;
        }

        Node *back = tail; // 记录最后一个有效节点

        m_capacity += count * m_baseSize;
        while (count--)
        {
            if (tail)
            {
                Node *newNode = new Node(m_baseSize, nullptr, tail);
                tail->next = newNode; // 链接到尾部
                tail = tail->next;    // 更新尾指针
            }
        }
        if (!m_curr)
        {
            m_curr = back->next;
        }
    }

    void ByteArray::setSize(size_t v)
    {
        if (m_size < v)
        {
            m_size = v;
        }
    }

    std::string ByteArray::toString() const
    {
        std::string str;
        str.resize(getReadSize());
        if (str.size() == 0)
        {
            return str;
        }
        read(&str[0], str.size(), m_position);
        return str;
    }

    std::string ByteArray::toHexString() const
    {
        std::string str = toString();
        std::stringstream ss;
        for (size_t i = 0; i < str.size(); i++)
        {
            if (i > 0 && (i & 31) == 0)
            {
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex
               << (int)(uint8_t)str[i] << " ";
        }
        return ss.str();
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec> &vect, uint64_t size) const
    {
        // 获取可读大小
        size_t readsize = getReadSize();
        size = size > readsize ? readsize : size;
        if (size == 0)
        {
            return 0;
        }
        Node *tem = m_curr;
        size_t realreadsize = size;
        size_t npos = m_position % m_baseSize;
        size_t ncap = tem->size - npos;
        struct iovec iov;
        while (size)
        {
            if (ncap >= size)
            {
                iov.iov_base = tem->ptr + npos;
                iov.iov_len = size;
                size = 0;
            }
            else
            {
                iov.iov_base = tem->ptr + npos;
                iov.iov_len = ncap;
                size -= ncap;
                if (!tem->next)
                {
                    BLUE_LOG_ERROR(g_logger) << "no date,only readsize = " << (realreadsize - size);
                    return realreadsize - size;
                }
                tem = tem->next;
                ncap = tem->size;
                npos = 0;
            }
            vect.push_back(iov);
        }
        return realreadsize;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec> &vect, uint64_t size, size_t position) const
    {
        size_t readsize = m_size - position;
        size = size > readsize ? readsize : size;
        if (size == 0)
        {
            return 0;
        }
        Node *tem = m_root;
        size_t tem_p = position;
        while (tem && tem_p >= tem->size)
        {
            tem_p -= tem->size;
            tem = tem->next;
        }
        if (BLUE_UNLIKELY(!tem))
        {
            BLUE_LOG_ERROR(g_logger) << "没有内容可以读, position : " << position;
            return 0;
        }
        size_t realreadsize = size;
        size_t npos = position % m_baseSize;
        size_t ncap = tem->size - npos;
        struct iovec iov;
        while (size)
        {
            if (ncap >= size)
            {
                iov.iov_base = tem->ptr + npos;
                iov.iov_len = size;
                size = 0;
            }
            else
            {
                iov.iov_base = tem->ptr + npos;
                iov.iov_len = ncap;
                size -= ncap;
                if (!tem->next)
                {
                    BLUE_LOG_ERROR(g_logger) << "no date,only readsize = " << (realreadsize - size);
                    return realreadsize - size;
                }
                tem = tem->next;
                ncap = tem->size;
                npos = 0;
            }
            vect.push_back(iov);
        }
        return realreadsize;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &vect, uint64_t size)
    {
        if (size == 0)
        {
            return 0;
        }
        addCapacity(size);
        Node *tem = m_curr;
        size_t realwritesize = size;
        size_t npos = m_position % m_baseSize;
        size_t ncap = tem->size - npos;
        struct iovec iov;
        while (size)
        {
            if (ncap >= size)
            {
                iov.iov_base = tem->ptr + npos;
                iov.iov_len = size;
                size = 0;
            }
            else
            {
                iov.iov_base = tem->ptr + npos;
                iov.iov_len = ncap;
                size -= ncap;
                if (!tem->next)
                {
                    tem->next = new Node(m_baseSize);
                    m_capacity += m_baseSize;
                }
                tem = tem->next;
                ncap = tem->size;
                npos = 0;
            }
            vect.push_back(iov);
        }
        return realwritesize;
    }

}