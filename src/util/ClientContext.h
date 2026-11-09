#include <functional>
#include <string>
#include <vector>
#include <memory>
#include "src/util/resp_parser.h"
#include "src/util/scan_cursor.h"

namespace blue
{
    class ClientContext
    {
    private:
        struct WatchedKey;
    public:
        using ClientContextPtr = std::shared_ptr<ClientContext>;
        ClientContext() = default;
        ~ClientContext() = default;
    public:
        using VersionChecker = std::function<uint64_t(const std::string &)>;
        void setVersionChecker(VersionChecker checker)
        {
            m_version_checker = checker;
        }
    public:
        /**
         * @brief 获取客户端密码
         */
        const std::string &getClientPassword() const noexcept { return m_client_password; }

        /**
         * @brief 设置客户端密码
         * @param val 客户端密码
         */
        void setClientPassword(const std::string &val) { m_client_password = val; }

        /**
         * @brief 获取客户端级别
         */
        const int getClientlevel() const noexcept { return m_auth_level; }

        /**
         * @brief 设置客户端级别
         * @param val 级别
         */
        void setClientlevel(int val) noexcept { m_auth_level = val; }

        /**
         * @brief 获取客户端名称
         */
        const std::string &getClientName() const noexcept { return m_client_name; }

        /**
         * @brief 获取客户端数据库索引id
         */
        const int getClientId() const noexcept { return m_client_db_id; }

        /**
         * @brief 设置客户端名称
         * @param 客户端名称
         */
        void setClientName(const std::string &val) { m_client_name = val; }

        /**
         * @brief 设置客户端数据库索引id
         * @param val 客户端数据库索引id
         */
        void setClientId(int val) noexcept { m_client_db_id = val; }

        /**
         * @brief 开始redis事务
         * @note 如果在订阅模式，那么不会进入事务模式
         */
        bool beginTransaction() noexcept { if (m_in_subScription) { return false; } m_in_transaction = true; return true; }

        /**
         * @brief 是否在事务中
         * @return true 表示在事务中
         */
        bool inTransaction() const noexcept { return m_in_transaction; }

        /**
         * @brief 获取所有事务
         * @return 事务
         */
        const std::vector<std::vector<blue::RespValue>> &getTransaction() const { return m_transaction_cmds; }

        /**
         * @brief 添加一组事务
         * @param transaction 事务数组
         */
        void addTransaction(std::vector<blue::RespValue> transaction) { m_transaction_cmds.push_back(std::move(transaction)); }

        /**
         * @brief 清理事务
         */
        void clearTransaction()
        {
            m_transaction_cmds.clear();
            m_in_transaction = false;
        }

        /**
         * @brief 添加监视key,version
         * @param key 需要监视的key
         * @param version 需要监视的key当前的version
         */
        void addWatchKey(const std::string &key, uint64_t version)
        {
            m_watchedKeys.emplace_back(key, version);
        }

        /**
         * @brief 清空所有被监视的key
         */
        void clearWatchedKey() { m_watchedKeys.clear(); }

        /**
         * @brief key的版本是否被改变
         * @param key 需要检测的key
         * @param curr_version key当前的版本
         * @return true表示有修改,false 也可能表示没有监视这个key
         */
        bool isKeyModified(const std::string &key, uint64_t curr_version) const
        {
            bool modified = false;
            for (const auto &watchedkey : m_watchedKeys)
            {
                if (watchedkey.key == key && watchedkey.version != curr_version)
                {
                    modified = true;
                    break;
                }
            }
            return modified;
        }

        /**
         * @brief 是否有key的版本被改变
         * @param getversoin 获取version的函数
         * @return true表示有修改,false 也可能表示没有监视这个key
         */
        bool hasKeyModified() const
        {
            bool modified = false;
            for (const auto &watchedkey : m_watchedKeys)
            {
                if (watchedkey.version != m_version_checker(watchedkey.key))
                {
                    modified = true;
                    break;
                }
            }
            return modified;
        }

        /**
         * @brief 获取watchedKey
         */
        const std::vector<WatchedKey> &getWatchedKey() const noexcept { return m_watchedKeys; }

        /**
         * @brief 开始订阅模式
         * @note 如果在事务模式，那么不会进入订阅模式
         */
        bool beginSubScription() { if (m_in_transaction) { return false; } m_in_subScription = true; return true; }

        /**
         * @brief 退出订阅模式
         */
        bool endSubScription() { m_in_subScription = false; return true; }

        /**
         * @brief 是否在订阅模式
         */
        bool inSubScription() const noexcept { return m_in_subScription; }

        /**
         * @brief 添加订阅channel
         * @param channel 需要添加的channel
         */
        void addSubScriptionChannel(const std::string &channel) { m_subScription_channels.insert(channel); }

        /**
         * @brief 删除订阅channel
         * @param channel 需要删除的channel
         */
        void removeSubScriptionChannel(const std::string &channel) { m_subScription_channels.erase(channel); }

        /**
         * @brief 获取订阅channel
         */
        const std::unordered_set<std::string> &getSubScriptionChannels() const noexcept { return m_subScription_channels; }

        /**
         * @brief 添加订阅pattern
         * @param pattern 需要添加的pattern
         */
        void addSubScriptionPattern(const std::string &pattern) { m_subScription_patterns.insert(pattern); }

        /**
         * @brief 删除订阅channel
         * @param channel 需要删除的channel
         */
        void removeSubScriptionPattern(const std::string &pattern) { m_subScription_patterns.erase(pattern); }

        /**
         * @brief 获取订阅pattern
         */
        const std::unordered_set<std::string> &getSubScriptionPatterns() const noexcept { return m_subScription_patterns; }

        /**
         * @brief 清空订阅
         */
        void clearSubScription() { m_subScription_channels.clear(); m_subScription_patterns.clear(); m_in_subScription = false; }

        /**
         * @brief 设置监控模式
         */
        void setMonitorMode(bool enabled) noexcept { m_in_monitor = enabled; }

        /**
         * @brief 是否在监控模式
         */
        bool inMonitorMode() const noexcept { return m_in_monitor; }

        /**
         * @brief 是否具有游标
         */
        bool hasScanCursor() const { return m_hasScanCursor; }

        /**
         * @brief 获取游标
         */
        const ScanCursor& getScanCursor() const { return m_scanCursor; }

        /**
         * @brief 设置游标
         */
        void setScanCursor(const ScanCursor& cursor) 
        {
            m_scanCursor = cursor;
            m_hasScanCursor = true;
        }

        /**
         * @brief 清理游标
         */
        void clearScanCursor() 
        {
            m_hasScanCursor = false;
            m_scanCursor = ScanCursor{};
        }
    private:
        struct WatchedKey
        {
            std::string key;  // 被监视的key
            uint64_t version; // 被监视的key的值的版本
        };

        std::vector<WatchedKey> m_watchedKeys;
    private:
        // commandHandler使用
        std::string m_client_password = "client123";                  // 客户端密码 = "client123", 供commandHandler使用
        std::string m_client_name = "";                               // 客户端名称
        int m_auth_level = 0;                                         // 客户端级别, 供commandHandler使用, 0普通用户(没有密码不可以访问),1客户端(除了个别危险命令不可访问),2管理员(最高权,主要负责shutdown)
        int m_client_db_id = 0;                                       // 客户端数据库的索引

        // 事务模式
        bool m_in_transaction = false;                                // 事务是否进行中
        std::vector<std::vector<blue::RespValue>> m_transaction_cmds; // 事务命令数组
        VersionChecker m_version_checker;                             // 版本检查回调函数

        // 订阅模式
        bool m_in_subScription = false;                               // 是否处在订阅模式
        std::unordered_set<std::string> m_subScription_channels;      // 每个连接订阅的频道
        std::unordered_set<std::string> m_subScription_patterns;      // 每个连接订阅的模式订阅

        // 监控模式
        bool m_in_monitor = false;

        // 游标
        ScanCursor m_scanCursor;  // SCAN 游标状态
        bool m_hasScanCursor = false;
    };
}