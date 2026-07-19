/**
 * @file cluster_message.h
 * @brief 集群化节点间消息格式
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.7.16
 * @copyright Copyright (c) 2026年 blue
 */
#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <stdexcept>
#include <arpa/inet.h>

namespace blue
{
    namespace cluster
    {

        enum class MessageType : uint8_t
        {
            // 集群管理
            PING = 0x01,   // 心跳检测
            PONG = 0x02,   // 心跳响应
            MEET = 0x03,   // 节点加入请求
            FAIL = 0x04,   // 节点故障报告
            FORGET = 0x05, // 节点下线
            CONFIG = 0x06, // 配置更新

            // 数据同步
            REPLICATE = 0x10, // 开启复制
            SYNC = 0x11,      // 全量同步请求
            PSYNC = 0x12,     // 增量同步请求
            REPLCONF = 0x13,  // 复制配置
            ACK = 0x14,       // 确认消息

            // 槽位管理
            UPDATE_SLOP = 0x20, // 更新槽位映射
            MIGRATE = 0x21,     // 迁移槽位
            MIGRATE_ACK = 0x22, // 迁移确认
            ASK = 0x23,         // ASK 重定向

            // 集群信息
            CLUSTER_INFO = 0x30, // 集群信息
            NODE_INFO = 0x31,    // 节点信息
            BULK_INFO = 0x32,    // 批量信息

            // 管理命令
            SHUTDOWN = 0x40, // 关闭节点
            RESTART = 0x41,  // 重启节点
            REPLACE = 0x42,  // 替换节点

            // 扩展
            CUSTOM = 0x50, // 自定义消息
        };

        // 消息标志
        enum class MessageFlag : uint16_t
        {
            NONE = 0x0000,
            ENCRYPTED = 0x0001,  // 消息已加密
            COMPRESSED = 0x0002, // 消息已压缩
            URGENT = 0x0004,     // 紧急消息
            RETRY = 0x0008,      // 重试消息
        };

// 消息头(39字节)
#pragma pack(push, 1)
        struct MessageHeader
        {
            uint64_t timestamp;   // 时间戳(ms)
            uint32_t sender_id;   // 发送者节点id
            uint32_t receiver_id; // 接收者节点id
            uint32_t payload_len; // 负载长度(最大64MB)
            uint32_t checksum;    // 校验和(CRC32)
            uint32_t magic;       // 魔数：0x434c5553('CLUS')
            uint32_t version;     // 协议版本: 1
            uint16_t flags;       // 消息标识(MessageFlag)
            uint8_t type;         // 消息类型(MessageType)
            uint8_t reserved[4];  // 预留字段
        };
#pragma pack(pop)

        static_assert(sizeof(MessageHeader) == 39, "MessageHeader size must be 39 bytes");

        // 消息体
        struct Message
        {
            MessageHeader header;
            std::vector<uint8_t> payload;

            Message()
            {
                memset(&header, 0, sizeof(header));
                header.magic = 0x434c5553; // 'CLUS'
                header.version = 1;
            }

            /**
             * @brief 消息体序列化
             */
            std::vector<uint8_t> serialize() const
            {
                std::vector<uint8_t> data;
                data.resize(sizeof(header) + payload.size());
                memcpy(data.data(), &header, sizeof(header));
                if (!payload.empty())
                {
                    memcpy(data.data() + sizeof(header), payload.data(), payload.size());
                }
                return data;
            }

            /**
             * @brief 将data反序列化为消息体
             * @param data 被序列化过的消息体
             */
            static Message deserialize(const std::vector<uint8_t> &data)
            {
                Message msg;
                if (data.size() < sizeof(MessageHeader))
                {
                    throw std::runtime_error("Invalid message: too small");
                }

                memcpy(&msg.header, data.data(), sizeof(MessageHeader));
                if (msg.header.payload_len > 0)
                {
                    msg.payload.resize(msg.header.payload_len);
                    memcpy(msg.payload.data(), data.data() + sizeof(MessageHeader), msg.header.payload_len);
                }
                return msg;
            }

            /**
             * @brief 检查魔数
             */
            bool isValid() const noexcept
            {
                return header.magic == 0x434C5553;
            }

            /**
             * @brief 计算校验和
             */
            uint32_t calculateChecksum() const
            {
                uint32_t crc = 0;
                if (!payload.empty())
                {
                    crc = crc32(0, payload.data(), payload.size());
                }
                return crc;
            }

        private:
            static uint32_t crc32(uint32_t crc, const uint8_t *data, size_t len)
            {
                static const uint32_t crc32_table[256] = {
                    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
                    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
                    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
                    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
                    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
                    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
                    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
                    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
                    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
                    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
                    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
                    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
                    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
                    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
                    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
                    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
                    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
                    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
                    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
                    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
                    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
                    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
                    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
                    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
                    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
                    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
                    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
                    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
                    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
                    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
                    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
                    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
                    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
                    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
                    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
                    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
                    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
                    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
                    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
                    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
                    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
                    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
                    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
                    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
                    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
                    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
                    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
                    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
                    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
                    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
                    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
                    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
                    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
                    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
                    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
                    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
                    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
                    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
                    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
                    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
                    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
                    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
                    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
                    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};
                for (size_t i = 0; i < len; i++)
                {
                    crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
                }
                return crc;
            }
        };

        // 消息工厂
        class MessageFactory
        {
        public:
            /**
             * @brief 创建 PING 消息
             * @param sender_id 发送者id
             * @param receiver_id 接收者id,默认为0表示广播
             */
            static Message createPing(uint32_t sender_id, uint32_t receiver_id = 0)
            {
                Message msg;
                msg.header.type = static_cast<uint8_t>(MessageType::PING);
                msg.header.sender_id = sender_id;
                msg.header.receiver_id = receiver_id;
                msg.header.timestamp = getCurrentTimeStamp();
                return msg;
            }

            /**
             * @brief 创建 PONG 消息
             * @param sender_id 发送者id
             * @param receiver_id 接收者id,默认为0表示广播
             */
            static Message createPong(uint32_t sender_id, uint32_t receiver_id)
            {
                Message msg;
                msg.header.type = static_cast<uint8_t>(MessageType::PONG);
                msg.header.sender_id = sender_id;
                msg.header.receiver_id = receiver_id;
                msg.header.timestamp = getCurrentTimeStamp();
                return msg;
            }

            /**
             * @brief 创建 MEET 消息
             * @param sender_id 发送者id
             * @param receiver_id 接受者id
             * @param ip 发送者ip
             * @param port 发送者端口
             */
            static Message createMeet(uint32_t sender_id, uint32_t receiver_id, const std::string &ip, uint16_t port)
            {
                Message msg;
                msg.header.type = static_cast<uint8_t>(MessageType::MEET);
                msg.header.sender_id = sender_id;
                msg.header.receiver_id = receiver_id;
                msg.header.timestamp = getCurrentTimeStamp();

                // 负载 ip + port
                std::string pl = ip + ":" + std::to_string(port);
                msg.payload.assign(pl.begin(), pl.end());
                msg.header.payload_len = msg.payload.size();

                return msg;
            }

            /**
             * @brief 创建 FATL 消息
             * @param sender_id 发送者id
             * @param failed_node_id 故障节点id
             * @note 广播
             */
            static Message createFatl(uint32_t sender_id, uint32_t failed_node_id)
            {
                Message msg;
                msg.header.type = static_cast<uint8_t>(MessageType::FAIL);
                msg.header.sender_id = sender_id;
                msg.header.receiver_id = 0; // 广播
                msg.header.timestamp = getCurrentTimeStamp();

                // 负载 故障节点id
                msg.payload.resize(sizeof(uint32_t));
                memcpy(msg.payload.data(), &failed_node_id, sizeof(uint32_t));
                msg.header.payload_len = msg.payload.size();

                return msg;
            }

            /**
             * @brief 创建 UPDATE_SLOP 消息
             * @param sender_id 发送者id
             * @param slot_mapping slot->node_id的映射
             * @note 广播
             */
            static Message createUpdateSlop(uint32_t sender_id, const std::map<uint16_t, uint32_t> &slot_mapping)
            {
                Message msg;
                msg.header.type = static_cast<uint8_t>(MessageType::UPDATE_SLOP);
                msg.header.sender_id = sender_id;
                msg.header.receiver_id = 0; // 广播
                msg.header.timestamp = getCurrentTimeStamp();

                // 负载 slot -> node_id
                std::string pl;
                for (const auto &[slot, node_id] : slot_mapping)
                {
                    pl += std::to_string(slot) + "," + std::to_string(node_id) + ";";
                }
                msg.payload.assign(pl.begin(), pl.end());
                msg.header.payload_len = msg.payload.size();

                return msg;
            }

        private:
            static uint64_t getCurrentTimeStamp()
            {
                auto now = std::chrono::steady_clock::now();
                return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            }
        };
    }   // namespace cluster
}