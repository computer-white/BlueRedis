#include "sslsocket.h"
#include "log.h"

// ssl socket
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    static std::string s_cert_dir;
    void SSLSocket::initSSL(const std::string &cert_dir)
    {
        s_cert_dir = cert_dir;
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        BLUE_LOG_INFO(g_logger) << "OpenSSL initialized, cert dir: " << cert_dir;
    }

    void SSLSocket::cleanupSSL()
    {
        EVP_cleanup();
        ERR_free_strings();
    }

    SSLSocket::SSLSocket(MSocket::MSocketPtr sock, bool owner, bool isServer)
        : SocketStream(sock,owner),
          m_ssl(nullptr),
          m_ctx(nullptr),
          m_isServer(isServer),
          m_handshakedone(false)
    {

        if (m_isServer) 
        {
            m_ctx = SSL_CTX_new(TLS_server_method());
        } 
        else 
        {
            m_ctx = SSL_CTX_new(TLS_client_method());
        }
    
        if (!m_ctx) 
        {
            BLUE_LOG_ERROR(g_logger) << "SSL_CTX_new failed";
            return;
        }
        if (m_isServer)
        {
            // 加载证书
            std::string cert_file = s_cert_dir + "/server.crt";
            std::string key_file = s_cert_dir + "/server.key";
            
            if (SSL_CTX_use_certificate_file(m_ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
                BLUE_LOG_ERROR(g_logger) << "Load cert failed: " << cert_file;
                ERR_print_errors_fp(stderr);
                SSL_CTX_free(m_ctx);
                m_ctx = nullptr;
                return;
            }
            if (SSL_CTX_use_PrivateKey_file(m_ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
                BLUE_LOG_ERROR(g_logger) << "Load key failed: " << key_file;
                ERR_print_errors_fp(stderr);
                SSL_CTX_free(m_ctx);
                m_ctx = nullptr;
                return;
            }
            if (!SSL_CTX_check_private_key(m_ctx)) {
                BLUE_LOG_ERROR(g_logger) << "Private key does not match cert";
                ERR_print_errors_fp(stderr);
                SSL_CTX_free(m_ctx);
                m_ctx = nullptr;
                return;
            }
        }
        
        m_ssl = SSL_new(m_ctx);
        SSL_set_fd(m_ssl, sock->getSocketfd());

        if (m_isServer)
        {
            SSL_set_accept_state(m_ssl);
        }
        else
        {
            SSL_set_connect_state(m_ssl);
        }
    }

    SSLSocket::~SSLSocket()
    {
        if (m_ssl)
        {
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
        }
        if (m_ctx)
        {
            SSL_CTX_free(m_ctx);
        }
    }

    bool SSLSocket::handshake()
    {
        if (m_handshakedone)
        {
            return true;
        }
        int ret;
        if (m_isServer) 
        {
            ret = SSL_accept(m_ssl);
        } 
        else 
        {
            ret = SSL_connect(m_ssl);
        }
        if (ret <= 0) {
            int err = SSL_get_error(m_ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                return false;
            }
            BLUE_LOG_ERROR(g_logger) << "SSL handshake failed, err=" << err;
            if (err == SSL_ERROR_SSL) 
            {
                // 获取更详细的错误
                unsigned long e = ERR_get_error();
                char buf[256];
                ERR_error_string_n(e, buf, sizeof(buf));
                BLUE_LOG_ERROR(g_logger) << "SSL error: " << buf;
            }
            return false;
        }
        m_handshakedone = true;
        // ssl密码
        BLUE_LOG_INFO(g_logger) << "SSL handshake success, cipher : " 
                                << SSL_get_cipher(m_ssl);
        return true;
    }

    ssize_t SSLSocket::read(void *buf, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        BLUE_LOG_WARN(g_logger) << "SSLSocket::read called, len=" << len;  // ← 加这行
        int ret = SSL_read(m_ssl,buf,len);
        if (ret <= 0)
        {
            int err = SSL_get_error(m_ssl,ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {
                errno = EAGAIN;
                return -1;
            }
            if (err == SSL_ERROR_ZERO_RETURN)
            {
                return 0; // 连接关闭
            }
            return -1;
        }
        return ret;
    }

    ssize_t SSLSocket::read(ByteArray::ByteArrayPtr data, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        // 1. 获取 ByteArray 的写缓冲区（分散的 iovec）
        std::vector<iovec> vec;
        data->getWriteBuffers(vec, len);
        
        // 2. 临时申请一块连续内存，用 SSL_read 读入
        std::vector<char> tmp(len);
        int ret = SSL_read(m_ssl, tmp.data(), len);
        
        if (ret <= 0) {
            int err = SSL_get_error(m_ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                errno = EAGAIN;
                return -1;
            }
            if (err == SSL_ERROR_ZERO_RETURN) {
                return 0;
            }
            return -1;
        }
        
        // 3. 把临时缓冲区的数据拷贝到 ByteArray 的分散缓冲区中
        size_t copied = 0;
        size_t remaining = ret;
        for (auto& v : vec) {
            if (remaining == 0) break;
            size_t to_copy = std::min(remaining, v.iov_len);
            memcpy(v.iov_base, tmp.data() + copied, to_copy);
            copied += to_copy;
            remaining -= to_copy;
        }

        // 4. 更新 ByteArray 的写位置
        data->setSize(data->getSize() + ret);
        data->setPosition(data->getPosition() + ret);
        
        return ret;

    }

    ssize_t SSLSocket::write(const void *buf, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        BLUE_LOG_WARN(g_logger) << "SSLSocket::write called, len=" << len;
        int ret = SSL_write(m_ssl, buf, len);
        if (ret <= 0) {
            int err = SSL_get_error(m_ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                errno = EAGAIN;
                return -1;
            }
            return -1;
        }
        return ret;
    }

    ssize_t SSLSocket::write(ByteArray::ByteArrayPtr data, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        // 1. 获取 ByteArray 的读缓冲区（分散的 iovec）
        std::vector<iovec> vec;
        data->getReadBuffers(vec, len);
        
        // 2. 把分散的数据拷贝到连续内存
        std::vector<char> tmp(len);
        size_t copied = 0;
        for (auto& v : vec) {
            if (copied >= len) break;
            size_t to_copy = std::min(len - copied, v.iov_len);
            memcpy(tmp.data() + copied, v.iov_base, to_copy);
            copied += to_copy;
        }
        
        // 3. SSL 写入
        int ret = SSL_write(m_ssl, tmp.data(), copied);
        
        if (ret <= 0) {
            int err = SSL_get_error(m_ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                errno = EAGAIN;
                return -1;
            }
            return -1;
        }
        
        // 4. 更新 ByteArray 的读位置
        data->setPosition(data->getPosition() + ret);
        
        return ret;
    }

    void SSLSocket::close()
    {
        if (m_ssl)
        {
            SSL_shutdown(m_ssl);
        }
        SocketStream::close();
    }

}