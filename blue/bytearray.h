#ifndef BLUE_BYTEARRAY_H
#define BLUE_BYTEARRAY_H
#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <vector>

// 字节流输出
namespace blue
{
    class ByteArray
    {
    public:
        using ByteArrayPtr = std::shared_ptr<ByteArray>;

    public:
        struct Node
        {
            char *ptr = nullptr;
            Node *next = nullptr;
            Node *pre = nullptr;
            size_t size = 0;

            Node(size_t s);
            Node(size_t s, Node *next, Node *pre);
            Node() = default;
            ~Node();
        };

        /**
         * @brief 构造出一个便利签,每页大小默认4096
         * @param base_size 便利签每页大小
         */
        ByteArray(size_t base_size = 4096);

        /**
         * @brief 析构掉便利签,一页一页烧掉
         */
        ~ByteArray();

        /**
         * @brief 写入固定int8_t类型的内容
         * @param val 需要写入的内容
         */
        void writeFint8(int8_t val);

        /**
         * @brief 写入固定uint8_t类型的内容
         * @param val 需要写入的内容
         */
        void writeFuint8(uint8_t val);

        /**
         * @brief 写入固定int16_t类型的内容
         * @param val 需要写入的内容
         * @note 考虑字节序
         */
        void writeFint16(int16_t val);

        /**
         * @brief 写入固定uint16_t类型的内容
         * @param val 需要写入的内容
         * @note 考虑字节序
         */
        void writeFuint16(uint16_t val);

        /**
         * @brief 写入固定int32_t类型的内容
         * @param val 需要写入的内容
         * @note 考虑字节序
         */
        void writeFint32(int32_t val);

        /**
         * @brief 写入固定uint32_t类型的内容
         * @param val 需要写入的内容
         * @note 考虑字节序
         */
        void writeFuint32(uint32_t val);

        /**
         * @brief 写入固定int64_t类型的内容
         * @param val 需要写入的内容
         * @note 考虑字节序
         */
        void writeFint64(int64_t val);

        /**
         * @brief 写入固定uint64_t类型的内容
         * @param val 需要写入的内容
         * @note 考虑字节序
         */
        void writeFuint64(uint64_t val);

        /**
         * @brief 写入不固定int32_t类型的内容
         * @param val 需要写入的内容
         * @note 每次写入8字节,对于有符号进行向无符号的Varint + Zigzag编码
         */
        void writeInt32(int32_t val);

        /**
         * @brief 写入不固定uint32_t类型的内容
         * @param val 需要写入的内容
         * @note 每次写入8字节
         */
        void writeUint32(uint32_t val);

        /**
         * @brief 写入不固定int64_t类型的内容
         * @param val 需要写入的内容
         * @note 每次写入8字节,对于有符号进行向无符号的Varint + Zigzag编码
         */
        void writeInt64(int64_t val);

        /**
         * @brief 写入不固定uint64_t类型的内容
         * @param val 需要写入的内容
         * @note 每次写入8字节
         */
        void writeUint64(uint64_t val);

        /**
         * @brief 写入float内容
         * @param val 需要写入的内容
         * @note 先copy进入uint32_t然后调用uint32_t的函数写入
         */
        void writeFloat(float val);

        /**
         * @brief 写入double内容
         * @param val 需要写入的内容
         * @note 先copy进入uint64_t然后调用uint64_t的函数写入
         */
        void writeDouble(double val);

        /**
         * @brief 写入长度为16位的string
         * @param val 需要写入的内容
         * @note 先写入uint16_t的字符串长度,再写入字符串
         */
        void writeString16(const std::string &val);

        /**
         * @brief 写入长度为32位的string
         * @param val 需要写入的内容
         * @note 先写入uint32_t的字符串长度,再写入字符串
         */
        void writeString32(const std::string &val);

        /**
         * @brief 写入长度为64位的string
         * @param val 需要写入的内容
         * @note 先写入uint64_t的字符串长度,再写入字符串
         */
        void writeString64(const std::string &val);

        /**
         * @brief 写入长度为64位的string
         * @param val 需要写入的内容
         * @note 先写入uint64_t的字符串长度,再写入字符串
         */
        void writeStringVint(const std::string &val);

        /**
         * @brief 写入长度不固定string
         * @param val 需要写入的内容
         * @note 直接写入字符串
         */
        void writeStringLength(const std::string &val);

        /**
         * @brief 读出固定int8_t类型的内容
         * @return 返回读到的内容
         */
        int8_t readFint8();

        /**
         * @brief 读出固定uint8_t类型的内容
         * @return 返回读到的内容
         */
        uint8_t readFuint8();

        /**
         * @brief 读出固定int16_t类型的内容
         * @return 返回读到的内容
         * @note 返回答案时需要注意字节序
         */
        int16_t readFint16();

        /**
         * @brief 读出固定uint16_t类型的内容
         * @return 返回读到的内容
         * @note 返回答案时需要注意字节序
         */
        uint16_t readFuint16();

        /**
         * @brief 读出固定int32_t类型的内容
         * @return 返回读到的内容
         * @note 返回答案时需要注意字节序
         */
        int32_t readFint32();

        /**
         * @brief 读出固定uint32_t类型的内容
         * @return 返回读到的内容
         * @note 返回答案时需要注意字节序
         */
        uint32_t readFuint32();

        /**
         * @brief 读出固定int64_t类型的内容
         * @return 返回读到的内容
         * @note 返回答案时需要注意字节序
         */
        int64_t readFint64();

        /**
         * @brief 读出固定uint64_t类型的内容
         * @return 返回读到的内容
         * @note 返回答案时需要注意字节序
         */
        uint64_t readFuint64();

        /**
         * @brief 读出int32_t类型的内容
         * @return 返回读到的内容
         * @note 对于有符号32位整数需要先考虑读出无符号然后进行Varint + ZigZag 解码
         */
        int32_t readInt32();

        /**
         * @brief 读出uint32_t类型的内容
         * @return 返回读到的内容
         * @note 每7位读出来然后判断标志位,加入到uint32_t答案中，最后返回答案
         */
        uint32_t readUint32();

        /**
         * @brief 读出int64_t类型的内容
         * @return 返回读到的内容
         * @note 对于有符号64位整数需要先考虑读出无符号然后进行Varint + ZigZag 解码
         */
        int64_t readInt64();

        /**
         * @brief 读出uint64_t类型的内容
         * @return 返回读到的内容
         * @note 每7位读出来然后判断标志位,加入到uint64_t答案中，最后返回答案
         */
        uint64_t readUint64();

        /**
         * @brief 读出float类型的内容
         * @return 返回读到的内容
         * @note 先按照uint32_t读出，再copy进入float
         */
        float readFloat();

        /**
         * @brief 读出double类型的内容
         * @return 返回读到的内容
         * @note 先按照uint64_t读出，再copy进入double
         */
        double readDouble();

        /**
         * @brief 读出长度为16位的字符串
         * @return 返回读到的内容
         * @note 先读出长度，再读字符串
         */
        std::string readString16();

        /**
         * @brief 读出长度为32位的字符串
         * @return 返回读到的内容
         * @note 先读出长度，再读字符串
         */
        std::string readString32();

        /**
         * @brief 读出长度为64位的字符串
         * @return 返回读到的内容
         * @note 先读出长度，再读字符串
         */
        std::string readString64();

        /**
         * @brief 读出长度为64位的字符串
         * @return 返回读到的内容
         * @note 先读出长度，再读字符串
         */
        std::string readStringVint();

        /**
         * @brief 清除便利签
         * @return
         */
        void clear();

        /**
         * @brief 往便利签写内容
         * @param buf 要写入的内容
         * @param size 写入内容的大小
         * @return
         */
        void write(const char *buf, size_t size);

        /**
         * @brief 从便利签读内容
         * @param buf 把读出的内容放到buf里面
         * @param size 读出内容的大小
         * @return
         */
        void read(void *buf, size_t size);

        /**
         * @brief 从便利签读内容
         * @param buf 把读出的内容放到buf里面
         * @param size 读出内容的大小
         * @param position 从指定位置读
         * @return
         */
        void read(void *buf, size_t size, size_t position) const;

        /**
         * @brief 获取当前便利签位置，类似鼠标光标位置
         * @return 当前便利签位置
         */
        size_t getPosition() const { return m_position; }

        /**
         * @brief 设置当前便利签位置，类似鼠标光标位置
         * @param v 便利签的位置
         * @return
         */
        void setPosition(size_t v);

        /**
         * @brief 设置当前内容大小
         */
        void setSize(size_t v);

        /**
         * @brief 把便利签内容写到文件
         * @param name 文件名称
         * @return 成功返回true
         */
        bool writeToFile(const std::string &name) const;

        /**
         * @brief 从文件读取便利签内容
         * @param name 文件名称
         * @return 成功返回true
         */
        bool readFromFile(const std::string &name);

        /**
         * @brief 判断当前便利签是否是小端类型
         * @return 是返回true
         */
        bool isLittleEndian() const;

        /**
         * @brief 设置当前便利签类型
         * @param val 便利签的类型
         * @return
         */
        void setLittleEndian(bool val);

        /**
         * @brief 获取当前便利签每一页大小
         * @param
         * @return 当前便利签每一页大小
         */
        size_t getBaseSize() const { return m_baseSize; }

        /**
         * @brief 获取当前便利签可读的大小
         * @param
         * @return 当前便利签可读的大小
         */
        size_t getReadSize() const { return m_size - m_position; }

        /**
         * @brief 转为string输出
         * @param
         * @return string
         */
        std::string toString() const;

        /**
         * @brief 转为16进制string
         * @param
         * @return string
         */
        std::string toHexString() const;

        /**
         * @brief 只获取内容
         * @param vect 存放struct iovec的容器
         * @param size 指定获取内容的大小
         * @return 实际读到的内容
         */
        uint64_t getReadBuffers(std::vector<struct iovec> &vect, uint64_t size = ~0ull) const;

        /**
         * @brief 只获取内容(从指定位置开始)
         * @param vect 存放struct iovec的容器
         * @param size 指定获取内容的大小
         * @param position 指定开始获取的位置
         * @return 实际读到的内容
         * @note 并没有真正读出数据,是指将数据指针绑定到vect上,后序对vect的读取比如socket的send实现将byteArray上数据读出来
         */
        uint64_t getReadBuffers(std::vector<struct iovec> &vect, uint64_t size, size_t position) const;

        /**
         * @brief 添加内容，会修改m_capacity
         * @param vect 需要添加的内容
         * @param size 添加内容的大小
         * @return 返回实际写入的大小
         * @note 并不是真正写入,只是将从m_curr当前页开始往后size大小的每一页绑定到vect上,后序通过对vect的写入(比如sock的recv)实现对byteArray的写入
         */
        uint64_t getWriteBuffers(std::vector<struct iovec> &vect, uint64_t size);

        /**
         * @brief 获取当前写入内容的大小
         * @return 当前写入内容的大小
         */
        size_t getSize() const { return m_size; }

    private:
        /**
         * @brief 增加容量
         * @param capacity 增加的容量大小
         */
        void addCapacity(size_t capacity);

        /**
         * @brief 获取还剩多少容量
         * @return 还剩多少容量
         */
        size_t getCapacity() const { return m_capacity - m_size; }

    private:
        // 想象成一本便利签,必须保证m_baseSize == Node中的size
        Node *m_root;      // 第一页
        Node *m_curr;      // 哪一页
        size_t m_baseSize; // 每页的大小
        size_t m_capacity; // 容量
        size_t m_position; // 当前在哪一行(不分具体在哪一页),不能等于m_baseSize的整数倍
        size_t m_size;     // 写入多少数据了
        uint8_t m_endian;  // 机器是大端还是小端
    };
}

#endif