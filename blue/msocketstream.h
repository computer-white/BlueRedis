#ifndef BLUE_MSOCKETSTREAM_H
#define BLUE_MSOCKETSTREAM_H
#include "msocket.h"
#include "mstream.h"

// socket stream
namespace blue
{
    class SocketStream : public MStream
    {
        public:
            using SocketStreamPtr = std::shared_ptr<SocketStream>;
            /**
             * @brief 构造函数
             * @param sock socket对象
             * @param owner 是否完全交给我管理sock对象
             */
            SocketStream(MSocket::MSocketPtr sock, bool owner = true);

            /**
             * @brief 析构函数
             * @note 若sock对象有效且fd没有关闭并完全由我管理那么关闭sock
             */
            ~SocketStream();

            /**
             * @brief 读取len长度内容到buf中
             * @param buf 存放读取内容的地址
             * @param len 想要读取的长度
             * @return 返回实际读取的大小
             * @note 对socket上recv的抽象
             */
            virtual ssize_t read(void *buf, size_t len) override;

            /**
             * @brief 读取len长度内容到buf中
             * @param buf 存放读取内容的地址
             * @param len 想要读取的长度
             * @return 返回实际读取的大小(出错返回实际读取)
             * @note 对socket上recv的抽象
             */
            virtual ssize_t read(ByteArray::ByteArrayPtr data, size_t len) override;

            /**
             * @brief 将buf中len长度内容发送给对方
             * @param buf 存放输出内容的地址
             * @param len 想要输出的长度
             * @return 返回实际写入的大小
             * @note 对socket上send的抽象(出错返回实际写入)
             */
            virtual ssize_t write(const void *buf, size_t len) override;

            /**
             * @brief 将data中len长度内容发送给对方
             * @param data 存放输出内容的地址
             * @param len 想要输出的长度
             * @return 返回实际写入的大小
             * @note 对socket上send的抽象(出错返回实际写入)
             */
            virtual ssize_t write(ByteArray::ByteArrayPtr data, size_t len) override;

            /**
             * @brief 若sock对象有效且fd没有关闭则关闭
             */
            virtual void close() override;

            /**
             * @brief 当前sock对象是否有效
             * @return 有效返回true
             */
            bool isConnected() const;
            /**
             * @brief 获取sock
             * @return 返回sock 对象
             */
            MSocket::MSocketPtr getSock() const { return m_sock; }
        protected:
            MSocket::MSocketPtr m_sock;
            bool m_owner;
    };
}
#endif