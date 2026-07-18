#include "blue/bytearray.h"
#include "blue/blue.h"
#include <random>
#include <memory>
#include <cstring>

static blue::Logger::LoggerPtr g_Logger = BLUE_LOG_MASSAGE_ROOT();

void test_getReadBuffers()
{
    BLUE_LOG_INFO(g_Logger) << "========== test getReadBuffers ==========";
    
#define TEST_READ_BUFFERS(base_len, write_size, read_req) { \
    /* 1. 写入测试数据 */ \
    blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(base_len)); \
    std::string original; \
    for (size_t i = 0; i < write_size; i++) { \
        char ch = 'A' + (rand() % 26); \
        byte->writeFint8(ch); \
        original += ch; \
    } \
    BLUE_LOG_INFO(g_Logger) << "write " << write_size << " bytes, size=" << byte->getSize(); \
    \
    /* 2. 获取可读缓冲区 */ \
    byte->setPosition(0); \
    std::vector<iovec> vec; \
    uint64_t ret = byte->getReadBuffers(vec, read_req); \
    BLUE_LOG_INFO(g_Logger) << "getReadBuffers(request=" << read_req \
                            << ") return=" << ret \
                            << " iov_cnt=" << vec.size(); \
    \
    /* 3. 从 iovec 中读取数据并验证 */ \
    std::string read_data; \
    size_t total_read = 0; \
    for (auto& iov : vec) { \
        BLUE_LOG_INFO(g_Logger) << "  iov[" << (&iov - &vec[0]) << "] base=" << iov.iov_base \
                                << " len=" << iov.iov_len; \
        read_data.append((char*)iov.iov_base, iov.iov_len); \
        total_read += iov.iov_len; \
    } \
    BLUE_ASSERT(total_read == ret); \
    BLUE_ASSERT(read_data == original.substr(0, ret)); \
    BLUE_LOG_INFO(g_Logger) << "getReadBuffers PASS: read_data matches original"; \
}

    // 测试1: 小 base_len，正好一个 block
    TEST_READ_BUFFERS(4, 4, 10);
    
    // 测试2: 跨多个 block
    TEST_READ_BUFFERS(4, 13, 100);
    
    // 测试3: 请求量小于可读量
    TEST_READ_BUFFERS(8, 20, 5);
    
    // 测试4: 请求量等于可读量
    TEST_READ_BUFFERS(8, 16, 16);
    
    // 测试5: 大量数据
    TEST_READ_BUFFERS(16, 100, 1000);
    
    // 测试6: 从中间位置读取
    {
        blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(4));
        std::string original;
        for (int i = 0; i < 20; i++) {
            char ch = 'a' + i;
            byte->writeFint8(ch);
            original += ch;
        }
        
        // 设置读取位置到第5个字节
        byte->setPosition(5);
        std::vector<iovec> vec;
        uint64_t ret = byte->getReadBuffers(vec, 100);
        
        BLUE_LOG_INFO(g_Logger) << "position=5, getReadBuffers return=" << ret 
                                << " iov_cnt=" << vec.size();
        
        std::string read_data;
        for (auto& iov : vec) {
            read_data.append((char*)iov.iov_base, iov.iov_len);
        }
        BLUE_ASSERT(read_data == original.substr(5));
        BLUE_LOG_INFO(g_Logger) << "offset read PASS";
    }
    
    // 测试7: 空 ByteArray
    {
        blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(4));
        std::vector<iovec> vec;
        uint64_t ret = byte->getReadBuffers(vec, 100);
        BLUE_ASSERT(ret == 0);
        BLUE_ASSERT(vec.empty());
        BLUE_LOG_INFO(g_Logger) << "empty ByteArray PASS";
    }
    
#undef TEST_READ_BUFFERS
}

void test_getWriteBuffers()
{
    BLUE_LOG_INFO(g_Logger) << "========== test getWriteBuffers ==========";
    
#define TEST_WRITE_BUFFERS(base_len, init_size, req_size) { \
    blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(base_len)); \
    /* 先写入一些数据占位 */ \
    for (size_t i = 0; i < init_size; i++) { \
        byte->writeFint8('X'); \
    } \
    BLUE_LOG_INFO(g_Logger) << "init: base_len=" << base_len \
                            << " init_size=" << init_size \
                            << " size=" << byte->getSize(); \
    \
    /* 获取可写缓冲区 */ \
    std::vector<iovec> vec; \
    uint64_t ret = byte->getWriteBuffers(vec, req_size); \
    BLUE_LOG_INFO(g_Logger) << "getWriteBuffers(request=" << req_size \
                            << ") return=" << ret \
                            << " iov_cnt=" << vec.size(); \
    \
    /* 验证 iovec 总大小 */ \
    size_t total_cap = 0; \
    for (auto& iov : vec) { \
        BLUE_LOG_INFO(g_Logger) << "  iov[" << (&iov - &vec[0]) << "] base=" << iov.iov_base \
                                << " len=" << iov.iov_len; \
        total_cap += iov.iov_len; \
    } \
    BLUE_ASSERT(total_cap == ret); \
    \
    /* 向可写缓冲区写入数据 */ \
    char fill_char = 'A'; \
    size_t written = 0; \
    for (auto& iov : vec) { \
        memset(iov.iov_base, fill_char, iov.iov_len); \
        written += iov.iov_len; \
        fill_char++; \
    } \
    BLUE_LOG_INFO(g_Logger) << "write to buffers: " << written << " bytes"; \
    BLUE_LOG_INFO(g_Logger) << "getWriteBuffers PASS"; \
}

    // 测试1: 刚好一个 block
    TEST_WRITE_BUFFERS(8, 0, 8);
    
    // 测试2: 超过一个 block
    TEST_WRITE_BUFFERS(4, 0, 20);
    
    // 测试3: 已有数据，追加写入
    TEST_WRITE_BUFFERS(4, 3, 10);
    
    // 测试4: 大量写入
    TEST_WRITE_BUFFERS(16, 0, 100);
    
    // 测试5: 已有数据跨多个 block
    TEST_WRITE_BUFFERS(4, 13, 50);
    
    // 测试6: 请求 0 字节
    {
        blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(4));
        byte->writeFint8('X');
        std::vector<iovec> vec;
        uint64_t ret = byte->getWriteBuffers(vec, 0);
        BLUE_ASSERT(ret == 0);
        BLUE_LOG_INFO(g_Logger) << "request 0 bytes PASS";
    }
    
#undef TEST_WRITE_BUFFERS
}

void test_read_write_buffers_roundtrip()
{
    BLUE_LOG_INFO(g_Logger) << "========== test roundtrip ==========";
    
    // 模拟 SocketStream 的读写流程
    blue::ByteArray::ByteArrayPtr read_ba(new blue::ByteArray(4));
    blue::ByteArray::ByteArrayPtr write_ba(new blue::ByteArray(4));
    
    // 1. 准备发送数据
    std::string send_data = "Hello, this is a test message for roundtrip!";
    for (char c : send_data) {
        write_ba->writeFint8(c);
    }
    write_ba->setPosition(0);
    
    // 2. 模拟 write：从 write_ba 获取读缓冲区
    std::vector<iovec> read_vec;
    uint64_t read_ret = write_ba->getReadBuffers(read_vec, send_data.size());
    BLUE_LOG_INFO(g_Logger) << "write_ba getReadBuffers: " << read_ret << " bytes"
                            << " iov_cnt=" << read_vec.size();
    
    // 3. 模拟 read：获取写缓冲区
    std::vector<iovec> write_vec;
    uint64_t write_ret = read_ba->getWriteBuffers(write_vec, read_ret);
    BLUE_LOG_INFO(g_Logger) << "read_ba getWriteBuffers: " << write_ret << " bytes"
                            << " iov_cnt=" << write_vec.size();
    
    BLUE_ASSERT(write_ret >= read_ret);
    
    // 4. 拷贝数据（把分散的读缓冲区拷贝到分散的写缓冲区）
    size_t copy_offset = 0;  // 当前拷贝到原始数据的哪个位置
    for (auto& w_iov : write_vec) {
        if (copy_offset >= read_ret) break;
        
        size_t w_offset = 0;  // 当前写缓冲区的偏移
        while (w_offset < w_iov.iov_len && copy_offset < read_ret) {
            // 找到 copy_offset 对应的读缓冲区
            size_t r_offset = copy_offset;
            iovec* r_iov = nullptr;
            size_t r_iov_offset = 0;
            
            for (auto& r : read_vec) {
                if (r_offset < r.iov_len) {
                    r_iov = &r;
                    r_iov_offset = r_offset;
                    break;
                }
                r_offset -= r.iov_len;
            }
            
            BLUE_ASSERT(r_iov != nullptr);
            
            // 计算本次拷贝大小
            size_t r_avail = r_iov->iov_len - r_iov_offset;
            size_t w_avail = w_iov.iov_len - w_offset;
            size_t to_copy = std::min({r_avail, w_avail, read_ret - copy_offset});
            
            // 拷贝
            memcpy((char*)w_iov.iov_base + w_offset,
                   (char*)r_iov->iov_base + r_iov_offset,
                   to_copy);
            
            w_offset += to_copy;
            copy_offset += to_copy;
        }
    }
    
    BLUE_ASSERT(copy_offset == read_ret);
    
    // 5. 更新位置（模拟 SocketStream）
    write_ba->setPosition(write_ba->getPosition() + read_ret);
    read_ba->setSize(read_ba->getSize() + read_ret);
    read_ba->setPosition(read_ba->getPosition() + read_ret);
    
    // 6. 验证：从 read_ba 读回数据
    read_ba->setPosition(0);
    std::string received;
    for (size_t i = 0; i < send_data.size(); i++) {
        received += read_ba->readFint8();
    }
    
    BLUE_LOG_INFO(g_Logger) << "received: " << received;
    BLUE_LOG_INFO(g_Logger) << "expected: " << send_data;
    
    BLUE_ASSERT(received == send_data);
    BLUE_LOG_INFO(g_Logger) << "roundtrip PASS";
}

int main(int argc, char* argv[])
{
    srand(time(nullptr));
    
    test_getReadBuffers();
    test_getWriteBuffers();
    test_read_write_buffers_roundtrip();
    
    BLUE_LOG_INFO(g_Logger) << "All tests passed!";
    return 0;
}