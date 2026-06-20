#pragma once
#include <fstream>
#include <functional>
#include <string>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include "blue/task.h"
#include "blue/resp_parser.h"
#include "blue/msocket.h"

namespace blue
{
    class AOFModule
    {
    public:
        using TimePoint = std::chrono::steady_clock::time_point;
        using ExecuteFunc = std::function<RespValue(std::vector<RespValue>, MSocket::MSocketPtr, bool)>;
        AOFModule() = default;
        ~AOFModule() = default;
        AOFModule(const AOFModule& ) = delete;
        AOFModule& operator=(const AOFModule& ) = delete;
    private:
        // AOF 配置
        struct AOFConfig
        {
            // AOF
            bool aof_enabled = false;                           // 是否开启aof
            std::string aof_filename = "appendonly.aof";        // 文件模板名
            size_t aof_max_file_size = 1024 * 1024;             // 每个文件最大大小
            size_t aof_max_file_number = 5;                     // 保留5个aof文件
            std::string aof_sync = "everysec";                  // 保存策略,always, everysec, no
        };

        // AOF 异步
        struct AOFBuffer
        {
            std::mutex aof_mutex;                   // 互斥变量
            std::condition_variable aof_cv;         // 条件变量
            std::string aof_buffer;                 // 缓冲区
            std::atomic<size_t> aof_buffer_size{0}; // 缓冲区大小
            bool aof_flush_requested = false;       // 刷新请求
        };
    public:

        /**
         * @brief 设置回调execute
         */
        void setExecutor(ExecuteFunc func) { m_executor = func; }

        /**
         * @brief 供aofFlushThread来检测服务器是否停止
         */
        void stop() { m_stop.store(true, std::memory_order_release); }
        /**
         * @brief 初始化AOF
         */
        void initAOF();

        /**
         * @brief 追加命令到AOF
         * @param cmd 命令
         */
        void appendToAOF(const std::string &cmd);

        /**
         * @brief 加载AOF
         */
        void loadAOF();

        /**
         * @brief 判断是否是写命令
         * @param cmd 命令
         */
        bool isWriteCommand(const std::string &cmd);

        /**
         * @brief 格式化命令为RESP字符转
         * @param args 命令数组
         */
        std::string formatCommand(const std::vector<RespValue> &args);

        /**
         * @brief 按策略循环后台同步AOF文件
         */
        Task<void> aofSyncLoop();

        /**
         * @brief 轮转
         */
        void rotateAOF();

        /**
         * @brief 获取文件名
         */
        std::string getAOFFilename(int index);

        /**
         * @brief 判断文件是否是旧文件并执行删除，若最后文件名可使用返回true
         * @param filename 需要判断的文件名
         * @return true 表示可以使用
         */
        bool cleanupOldAOFs(const std::string &filename);

        /**
         * @brief 开启AOF刷新线程
         */
        void startAOFFlushThread();

        /**
         * @brief 停止刷新线程
         */
        void stopAOFFlushThread();

        /**
         * @brief 刷新线程工作函数
         */
        void aofFlushThread();

        /**
         * @brief 取出缓冲区内容,写入文件
         */
        void flushAOFBuffer();
    public:

        /**
         * @brief 获取和设置aof_config
         */

        // 获取
        bool getConfig_AOFEnabled() const noexcept { return m_aof_config.aof_enabled; }

        const std::string &getConfig_AOFFilename() const noexcept { return m_aof_config.aof_filename; }

        size_t getConfig_AOFMaxFileSize() const noexcept { return m_aof_config.aof_max_file_size; }

        size_t getConfig_AOFMaxFileNumber() const noexcept { return m_aof_config.aof_max_file_number; }

        const std::string &getConfig_AOFSync() const noexcept { return m_aof_config.aof_sync; }

        // 设置
        void setConfig_AOFEnabled(bool val) noexcept { m_aof_config.aof_enabled = val; }

        void setConfig_AOFFilename(const std::string &val) noexcept { m_aof_config.aof_filename = val; }

        void setConfig_AOFMaxFileSize(size_t val) noexcept { m_aof_config.aof_max_file_size = val; }

        void setConfig_AOFMaxFileNumber(size_t val) noexcept { m_aof_config.aof_max_file_number = val; }

        void setConfig_AOFSync(const std::string &val) noexcept { m_aof_config.aof_sync = val; }

        /**
         * @brief 获取max_aof_buffer_size
         */
        size_t getMaxAOFBufferSize() const noexcept { return m_aof_max_buffer_size; }

        /**
         * @brief 设置max_aof_buffer_size
         */
        void setMaxAOFBufferSize(size_t val) noexcept { m_aof_max_buffer_size = val; }
        
        /**
         * @brief 获取last_aof_sync
         */
        TimePoint getLastAOFSync() const noexcept { return m_last_aof_sync; }

        /**
         * @brief 更新last_aof_sync
         */
        void setLastAOFSync(TimePoint now) noexcept { m_last_aof_sync = now; }

        /**
         * @brief 获取aof_rotating
         */
        bool getAOFRotating() const { return m_aof_rotating.load(std::memory_order_acquire); }

        /**
         * @brief 获取当前文件名
         */
        const std::string &getCurrentFileName() const { return m_aof_current_filename; }

        /**
         * @brief 获取当前文件编号
         */
        size_t getCurrentFileIdx() const noexcept { return m_aof_file_idx; }

        /**
         * @brief 获取文件大小
         */
        size_t getCurrentFileSize()
        {
            if (m_aof_file.is_open())
            {
                return (size_t)(m_aof_file.tellp());
            }
            return 0;
        }

        /**
         * @brief 刷新并关闭aof流
         */
        void closeAOFWithFlush() { if (m_aof_file.is_open()) { m_aof_file.flush(); m_aof_file.close(); } }

        /**
         * @brief 关闭aof流
         */
        void closeAOF() { if (m_aof_file.is_open()) { m_aof_file.close(); } }

    private:
        // 配置
        AOFConfig m_aof_config;                         // aof 配置

        // 异步变量
        AOFBuffer m_aof_buffer;                         // 异步aof缓存
        std::thread m_aof_flush_thread;                 // 刷新线程
        std::atomic<bool> m_aof_flush_running{false};   // 刷新线程是否正在运行中
        size_t m_aof_max_buffer_size = 1024 * 1024;     // 最大缓存大小

        // 基础变量
        std::shared_mutex m_aof_mutex;           // 互斥变量
        std::string m_aof_current_filename;      // 当前文件名
        std::ofstream m_aof_file;                // 当前打开的文件流
        size_t m_aof_file_idx = 0;               // 当前文件编号
        TimePoint m_last_aof_sync;               // 最新一次写入时间
        std::atomic<bool> m_aof_rotating{false}; // 轮转标志

        // 回调
        ExecuteFunc m_executor;
        std::atomic<bool> m_stop{false};
    };
}