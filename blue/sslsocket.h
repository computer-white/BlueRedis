#ifndef BLUE_SSLSOCKET_H
#define BLUE_SSLSOCKET_H
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>
#include "msocket.h"
#include "msocketstream.h"

// ssl socket
namespace blue
{
    class SSLSocket : public SocketStream
    {
        public:
            using SSLSocketPtr = std::shared_ptr<SSLSocket>;
        public:

            SSLSocket(MSocket::MSocketPtr sock, bool owner = true, bool isServer = true);
            ~SSLSocket();
            
            /**
             * @brief ssl握手
             */
            bool handshake();
            bool isHandshakeDone() const { return m_handshakedone; }

            /**
             * @brief 读取len长度内容到buf中
             * @param buf 存放读取内容的地址
             * @param len 想要读取的长度
             * @return 返回实际读取的大小
             * @note 对socket上recv的抽象(出错返回实际读取)
             */
            ssize_t read(void *buf, size_t len) override;

            /**
             * @brief 读取len长度内容到字节链表(byteArray)
             * @param data 字节链表指针
             * @param len 想要读取的长度
             * @return 返回实际读取的大小
             * @note 对socket上recv的抽象(出错返回实际读取)
             */
            ssize_t read(ByteArray::ByteArrayPtr data, size_t len) override;

            /**
             * @brief 将buf中len长度内容发送给对方
             * @param buf 存放输出内容的地址
             * @param len 想要输出的长度
             * @return 返回实际写入的大小
             * @note 对socket上send的抽象(出错返回实际写入)
             */
            ssize_t write(const void *buf, size_t len) override;

            /**
             * @brief 将data中len长度内容发送给对方
             * @param data 存放输出内容的地址
             * @param len 想要输出的长度
             * @return 返回实际写入的大小
             * @note 对socket上send的抽象(出错返回实际写入)
             */
            ssize_t write(ByteArray::ByteArrayPtr data, size_t len) override;

            /**
             * @brief 关闭流
             */
            void close() override;

            /**
             * @brief 获取原始 socket
             */
            MSocket::MSocketPtr getSocket() const { return m_sock; }

            /**
             * @brief 获取ssl
             */
            SSL* getSSL() const { return m_ssl; }

            bool isValid() const { return m_ssl != nullptr; }
            // 静态初始化（全局调用一次）
            static void initSSL(const std::string &);
            static void cleanupSSL();
        private:
            SSL* m_ssl;
            SSL_CTX* m_ctx;
            bool m_isServer;
            bool m_handshakedone;
        
    };
}
#endif