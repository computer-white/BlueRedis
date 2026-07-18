#include "blue/bytearray.h"
#include "blue/blue.h"
#include <random>
#include <memory>
static blue::Logger::LoggerPtr g_Logger = BLUE_LOG_MASSAGE_ROOT();

void test_bytearray()
{
#define XX(type,len,read_func,write_func,base_len) {\
    std::vector<type> vec; \
    for (int i = 0;i<len;i++) \
    { \
        type x = rand(); \
        vec.push_back(x); \
    } \
    blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(base_len)); \
    for (auto& x : vec) \
    { \
        byte->write_func(x); \
    } \
    byte->setPosition(0); \
    for (size_t i = 0;i < vec.size();i++) \
    { \
        type v = byte->read_func();\
        BLUE_ASSERT(v == vec[i]); \
    } \
    BLUE_ASSERT(byte->getReadSize() == 0); \
    BLUE_LOG_INFO(g_Logger) << #write_func "/" #read_func " (" #type ") len : " \
                            << len << " base_len << " << base_len   \
                            << " size : " << byte->getSize(); \
}

    XX(int8_t,100,readFint8,writeFint8,1);
    XX(uint8_t,100,readFuint8,writeFuint8,1);
    XX(int32_t,100,readFint32,writeFint32,1);
    XX(uint32_t,100,readFuint32,writeFuint32,1);
    XX(int64_t,100,readFint64,writeFint64,1);
    XX(uint64_t,100,readFuint64,writeFuint64,1);

    XX(int32_t,100,readInt32,writeInt32,1);
    XX(uint32_t,100,readUint32,writeUint32,1);
    XX(int64_t,100,readInt64,writeInt64,1);
    XX(uint64_t,100,readUint64,writeUint64,1);


    XX(float,100,readFloat,writeFloat,1);
    XX(double,100,readDouble,writeDouble,1);
#undef XX

#define XX(type,len,read_func,write_func,base_len) {\
    std::vector<type> vec; \
    for (int i = 0;i<len;i++) \
    { \
        type x = rand(); \
        vec.push_back(x); \
    } \
    blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(base_len)); \
    for (auto& x : vec) \
    { \
        byte->write_func(x); \
    } \
    byte->setPosition(0); \
    for (size_t i = 0;i < vec.size();i++) \
    { \
        type v = byte->read_func();\
        BLUE_ASSERT(v == vec[i]); \
    } \
    BLUE_ASSERT(byte->getReadSize() == 0); \
    BLUE_LOG_INFO(g_Logger) << "ToFile : " #write_func "/" #read_func " (" #type ") len : " \
                            << len << " base_len << " << base_len   \
                            << " size : " << byte->getSize(); \
    byte->setPosition(0);   \
    BLUE_ASSERT(byte->writeToFile("./tem/" #type "_" #len "_" #read_func ".dat"));  \
    blue::ByteArray::ByteArrayPtr byte2(new blue::ByteArray(base_len * 2)); \
    BLUE_ASSERT(byte2->readFromFile("./tem/" #type "_" #len "_" #read_func ".dat")); \
    byte2->setPosition(0);  \
    BLUE_ASSERT(byte->toString() == byte2->toString()); \
    BLUE_ASSERT(byte->getPosition() == 0);  \
    BLUE_ASSERT(byte2->getPosition() == 0); \
}

    XX(int8_t,100,readFint8,writeFint8,1);
    XX(uint8_t,100,readFuint8,writeFuint8,1);
    XX(int32_t,100,readFint32,writeFint32,1);
    XX(uint32_t,100,readFuint32,writeFuint32,1);
    XX(int64_t,100,readFint64,writeFint64,1);
    XX(uint64_t,100,readFuint64,writeFuint64,1);

    XX(int32_t,100,readInt32,writeInt32,1);
    XX(uint32_t,100,readUint32,writeUint32,1);
    XX(int64_t,100,readInt64,writeInt64,1);
    XX(uint64_t,100,readUint64,writeUint64,1);


    XX(float,100,readFloat,writeFloat,1);
    XX(double,100,readDouble,writeDouble,1);

#undef XX


#define XX(val,len,read_func,write_func,base_len) {\
    blue::ByteArray::ByteArrayPtr byte(new blue::ByteArray(base_len)); \
    byte->write_func(val); \
    byte->setPosition(0); \
    BLUE_ASSERT(byte->read_func() == val);  \
    BLUE_ASSERT(byte->getReadSize() == 0); \
    BLUE_LOG_INFO(g_Logger) << "ToFile : " #write_func "/" #read_func " (" #val ") len : " \
                            << len << " base_len << " << base_len   \
                            << " size : " << byte->getSize(); \
    byte->setPosition(0);   \
    BLUE_ASSERT(byte->writeToFile("./tem/string" "_" #len "_" #read_func ".dat"));  \
    blue::ByteArray::ByteArrayPtr byte2(new blue::ByteArray(base_len * 2)); \
    BLUE_ASSERT(byte2->readFromFile("./tem/string" "_" #len "_" #read_func ".dat")); \
    byte2->setPosition(0);  \
    BLUE_ASSERT(byte->toString() == byte2->toString()); \
    BLUE_ASSERT(byte->getPosition() == 0);  \
    BLUE_ASSERT(byte2->getPosition() == 0); \
}
    XX("hello world",12,readString16,writeString16,1);
    XX("hello world,hello blue",23,readString32,writeString32,1);
    XX("hello world,hello blue,hello,hello,xxx",39,readString64,writeString64,1);
#undef XX

}

int main(int argc,char* argv[])
{
    test_bytearray();
    return 0;
}