/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file log.h
 * @brief 日志系统
 * @author blue
 * @email homeheyang@outlook.com
 * @date 2026.3.24
 * @copyright Copyright (c) 2026年 blue
 */
#ifndef BLUE_LOG_H
#define BLUE_LOG_H
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <type_traits>
#include <chrono>
#include <map>
#include "util.h"
#include "singleton.h"
#include "mthread.h"
// 宏展开输出
#define BLUE_LOG_LEVEL(logger, level)                                                                                                         \
    if (logger->getlevel() <= level)                                                                                                          \
    blue::LogEventWrap(blue::LogEvent::LogEventPtr(new blue::LogEvent(logger, level, __FILE__,                                                \
                                                                      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()), \
                                                                      __LINE__,                                                               \
                                                                      0, blue::GetThreadId(),                                                 \
                                                                      blue::Mthread::GetName())))                                             \
        .getstringstream()

#define BLUE_LOG_DEBUGE(logger) BLUE_LOG_LEVEL(logger, blue::Level::DEBUG)
#define BLUE_LOG_INFO(logger) BLUE_LOG_LEVEL(logger, blue::Level::INFO)
#define BLUE_LOG_WARN(logger) BLUE_LOG_LEVEL(logger, blue::Level::WARN)
#define BLUE_LOG_ERROR(logger) BLUE_LOG_LEVEL(logger, blue::Level::ERROR)
#define BLUE_LOG_FATAL(logger) BLUE_LOG_LEVEL(logger, blue::Level::FATAL)

#define BLUE_LOG_MASSAGE_ROOT() blue::LoggerMgr::GetInstance()->getRoot()

// 使用名称获取对应的Logger
#define BLUE_LOG_NAME(name) blue::LoggerMgr::GetInstance()->getLogger(name)

// 日志模块
// 名称空间
namespace blue
{

    class Logger;        // 前置声明,日志名称可以由Appender往上传到Formatter
    enum class Level;    // 限定作用域的枚举类的前置声明
    class LoggerManager; // 前置声明Message

    class LogEvent
    {
    public:
        using LogEventPtr = std::shared_ptr<LogEvent>;

        /**
         * @brief logEvent默认构造函数
         */
        LogEvent() = default;

        /**
         * @brief logEvent构造函数
         * @param logger_pre 日志器智能指针
         * @param level      日志级别
         * @param file       文件名称
         * @param time       时间戳
         * @param line       行号
         * @param elapse     系统到现在启动时间
         * @param threadId   线程ID
         * @param name       线程名称
         */
        LogEvent(std::shared_ptr<Logger> logger_ptr, Level level,
                 const char *file, uint64_t time, uint32_t line, uint32_t elapse,
                 uint32_t threadId, const std::string &name);

        /**
         * @brief 获取文件名称
         */
        const char *getFilename() const { return m_file; }

        /**
         * @brief 获取时间戳
         */
        uint64_t getTime() const { return m_time; }

        /**
         * @brief 获取行号
         */
        uint32_t getLines() const { return m_lines; }

        /**
         * @brief 获取系统到现在启动时间
         */
        uint32_t getElapse() const { return m_elapse; }

        /**
         * @brief 获取线程id
         */
        uint32_t getThreadId() const { return m_threadID; }

        /**
         * @brief 获取线程名称
         */
        const std::string &getThreadName() const { return m_threadname; }

        /**
         * @brief 获取消息体
         */
        std::string getContent() const { return m_stringstream.str(); }

        /**
         * @brief 获取日志器智能指针
         */
        std::shared_ptr<Logger> getLoggerptr() const { return m_logger_ptr; }

        /**
         * @brief 获取string流,用于输入内容
         */
        std::stringstream &getstringstream() { return m_stringstream; }

        /**
         * @brief 获取日志级别
         */
        Level getLevel() const { return m_level; }

    private:
        const char *m_file = nullptr;         // 文件名
        uint64_t m_time = 0;                  // 时间戳
        uint32_t m_lines = 0;                 // 行号
        uint32_t m_elapse = 0;                // 程序到现在启动时间(毫秒)
        uint32_t m_threadID = 0;              // 线程id
        std::stringstream m_stringstream;     // string流
        Level m_level;                        // level
        std::shared_ptr<Logger> m_logger_ptr; // loggerptr
        std::string m_threadname;             // 线程名称
    }; // LogEvent

    // 限定作用域枚举支持比较运算符
    enum class Level
    {
        NOKNOW = 0, // noknow
        DEBUG = 1,  // debug
        INFO = 2,   // info
        WARN = 3,   // warn
        ERROR = 4,  // error
        FATAL = 5,  // fatal
    }; // Level

    class LogEventWrap
    {
    public:
        /**
         * @brief 日志事件的包装类构造函数
         * @param e 日志事件类智能指针
         */
        LogEventWrap(LogEvent::LogEventPtr e);
        /**
         * @brief 日志事件的包装类析构函数
         * @param e 日志事件类智能指针
         * @note 当析构函数发生时,才真正执行对日志的写入
         */
        ~LogEventWrap();

        /**
         * @brief 获取event智能指针
         */
        LogEvent::LogEventPtr getEvent() const { return m_event_ptr; }
        /**
         * @brief 获取event上的string流
         */
        std::stringstream &getstringstream();

    private:
        LogEvent::LogEventPtr m_event_ptr;
    };

    // 输出的日志格式
    class LogFormatter
    {
    public:
        using LogFormatterPtr = std::shared_ptr<LogFormatter>;

        /**
         * @brief LogFormatter的构造函数
         * @param pattern 日志的输出格式
         * @note 包含对日志输出格式的解析
         */
        LogFormatter(const std::string &pattern);

        /**
         * @brief 将event上的内容输出为string,写入输出日志的具体格式内容
         * @param logger_ptr 日志器智能指针
         * @param level 日志级别
         * @param event 日志事件指针
         */
        std::string format(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event);

        /**
         * @brief 获取输出格式字符串
         */
        const std::string getPattern() const { return m_pattern; }

        /**
         * @brief 获取模式字符串是否有错误
         * @note 原子操作
         */
        bool getHasError() const { return m_HasError.load(std::memory_order_acquire); }

    public:
        class FormatterItem
        {
        public:
            using FormatterItemPtr = std::shared_ptr<FormatterItem>;
            virtual ~FormatterItem() = default;
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) = 0;
        };

    private:
        /**
         * @brief 做输出格式字符串的解析
         */
        void _init();

    private:
        std::string m_pattern; // 日志输出格式
        std::vector<FormatterItem::FormatterItemPtr> m_items;
        std::atomic<bool> m_HasError = {false};
    };

    // 日志输出目的地
    class LogAppender
    {
        friend class Logger;

    public:
        using LogAppenderPtr = std::shared_ptr<LogAppender>;
        using MutexType = blue::SpinLockMutex;
        LogAppender() = default;
        virtual ~LogAppender() = default;
        virtual std::string toyamlString() = 0;
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) = 0;

        /**
         * @brief 获取formatter指针
         * @note 无锁实现
         */
        LogFormatter::LogFormatterPtr getformatter() const;

        /**
         * @brief 设置formatter指针
         * @param fomatter 新的formatter指针
         * @note 无锁实现
         */
        void setformatter(LogFormatter::LogFormatterPtr formatter);

        /**
         * @brief 获取日志级别
         * @note 原子操作
         */
        Level getLevel() const { return m_level.load(std::memory_order_acquire); }

        /**
         * @brief 设置日志级别
         * @param level 新的日志级别
         * @note 原子操作
         */
        void setLevel(Level level) { m_level.store(level, std::memory_order_release); }

        /**
         * @brief 获取是否有设置输出格式
         * @note 原子操作
         */
        const bool gatHasformatter() const { return m_hasformatter.load(std::memory_order_acquire); }

    protected:
        mutable MutexType m_mutex;
        std::atomic<Level> m_level = Level::DEBUG;
        LogFormatter::LogFormatterPtr m_formatter;
        std::atomic<bool> m_hasformatter = false;
    };

    class StdoutLogAppender : public LogAppender
    {
    public:
        using StdoutLogAppenderPtr = std::shared_ptr<StdoutLogAppender>;
        StdoutLogAppender() = default;
        ~StdoutLogAppender() = default;

        /**
         * @brief 将控制台的信息转为yaml,最后以字符串输出
         * @note 无锁
         */
        virtual std::string toyamlString() override;

        /**
         * @brief 将日志输出到控制台
         * @param logger_ptr 日志器指针
         * @param level 日志级别
         * @param event 日志事件
         * @note 无锁
         */
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override;

    private:
        std::string m_name = "console"; // 输出目的名称(控制台)
    };

    class FileoutLogAppender : public LogAppender
    {
    public:
        using TimePoint = std::chrono::steady_clock::time_point;
        using FileoutLogAppenderPtr = std::shared_ptr<FileoutLogAppender>;
        FileoutLogAppender() = default;

        /**
         * @brief 日志输出到文件类的析构函数,执行对文件的关闭
         */
        ~FileoutLogAppender();

        /**
         * @brief 日志输出到文件类的构造函数
         * @param filename 文件名
         */
        FileoutLogAppender(const std::string &filename);

        /**
         * @brief 输出轮转配置信息
         */
        static std::string RotateConfigToString();

        /**
         * @brief 文件信息转为yamlstring,最后以字符串输出
         * @note 无锁
         */
        virtual std::string toyamlString() override;

        /**
         * @brief 将event事件和日志相关信息写入文件
         * @param logger_ptr 日志器智能指针
         * @param level 日志级别
         * @param event 日志事件
         * @note 无锁
         */
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override;

    private:
        /**
         * @brief 初始化信息
         */
        void init();

        /**
         * @brief 获取文件名称
         */
        std::string getFilename(size_t idx) const;

        /**
         * @brief 清理文件名称
         */
        bool cleanFilename(const std::string &file) const;

        /**
         * @brief 日志文件轮转
         */
        void rotateFile();

        /**
         * @brief 写入文件
         */
        void writeToFile(const std::string &data);

    private:
        std::string m_filename;              // 当前文件名
        std::ofstream m_filestream;          // 文件输出流
        std::string m_name = "file";         // 输出目的名称(文件)
        size_t m_file_idx = 0;               // 当前文件编号
        std::atomic<TimePoint> m_lasttime;   // 文件最新时间
        std::atomic<bool> m_rotating{false}; // 轮转标志

    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        using LoggerPtr = std::shared_ptr<Logger>;
        using MutexType = blue::MRWmutex;

        /**
         * @brief 输出日志
         * @param level 日志级别
         * @param event 日志事件
         * @note 无锁实现
         */
        void Log(Level level, LogEvent::LogEventPtr event);

        /**
         * @brief 日志构造函数
         * @param name 日志的名称,默认为root
         * @note level级别默认为DEBUG,会在构造时设置一个标准的formatter格式
         */
        Logger(const std::string &name = "root");

        /// @brief Debug 日志
        /// @param content 日志内容
        void Debug(const std::string &content);

        /// @brief Info 日志
        /// @param content 日志内容
        void Info(const std::string &content);

        /// @brief Warn 日志
        /// @param content 日志内容
        void Warn(const std::string &content);

        /// @brief Error 日志
        /// @param content 日志内容
        void Error(const std::string &content);

        /// @brief Fatal 日志
        /// @param content 日志内容
        void Fatal(const std::string &content);

        /**
         * @brief 将日志信息转为yamlstring
         * @note 将日志名称，日志级别，日志格式，日志输出器内容以无锁形式写入到yaml节点
         */
        std::string toyamlString();

        /**
         * @brief 添加日志输出目的地
         * @param Appender 需要添加的日志输出目的地智能指针
         */
        void addAppender(LogAppender::LogAppenderPtr Appender);

        /**
         * @brief 删除日志输出目的地
         * @param Appender 需要删除的日志输出目的地智能指针
         */
        void delAppender(LogAppender::LogAppenderPtr Appender);

        /**
         * @brief 清除所有日志输出目的地
         * @note 内部含有写锁
         */
        void clearAppender() noexcept;

        /**
         * @brief 获取日志级别
         */
        Level getlevel() const { return m_level.load(std::memory_order_acquire); }

        /**
         * @brief 设置日志级别
         * @param val 需要设置的日志级别
         */
        void setlevel(Level val) { m_level.store(val, std::memory_order_release); }

        /**
         * @brief 获取日志名称
         */
        const std::string getname() const { return m_name; }

        /**
         * @brief 设置formatter格式,按照string格式字符串
         * @param rhs 日志输出格式字符串
         * @note formatter有错误不给予设置,通过调用setFormatter的无锁版本实现
         */
        void setFormatter(const std::string &rhs);

        /**
         * @brief 获取日志输出格式类智能指针
         * @note 无锁实现
         */
        LogFormatter::LogFormatterPtr getFormatter() const;

    private:
        /**
         * @brief 设置日志输出格式
         * @param rhs 日志输出格式类智能指针
         * @note 无锁实现，有读锁进行对appender的复制
         */
        void setFormatter(LogFormatter::LogFormatterPtr rhs);

    private:
        mutable MutexType m_mutex;                          // 互斥变量
        std::string m_name;                                 // 日志名称
        std::list<LogAppender::LogAppenderPtr> m_Appenders; // Appender列表
        std::atomic<Level> m_level;                         // 日志级别
        LogFormatter::LogFormatterPtr m_formatter;          // 输出日志格式
    };

    class LoggerManager
    {
    public:
        using MutexType = blue::MRWmutex;

        /**
         * @brief 构造函数,管理一个默认的logger智能指针
         * 
         * @note 设置了默认的appender
         */
        LoggerManager();

        /**
         * @brief 按照名称获取logger
         * @param name 日志的名称
         */
        Logger::LoggerPtr getLogger(const std::string &name);

        /**
         * @brief 获取管理的默认的root
         *  日志器的智能指针
         */
        Logger::LoggerPtr getRoot() const { return m_root; }

        /**
         * @brief 将loggerManager管理的日志器信息全部转为yaml,最后以字符串输出
         */
        std::string toyamlString();

    private:
        mutable MutexType m_mutex;
        std::map<std::string, Logger::LoggerPtr> m_logger;
        Logger::LoggerPtr m_root;
    };

    using LoggerMgr = blue::SingleTon<LoggerManager>;

    // 为枚举类型写一个枚举类模板
    template <typename Enum>
    class EnumTraits
    {
        static_assert(std::is_enum_v<Enum>, "必须为枚举类型");
    };
    template <>
    class EnumTraits<Level>
    {
    public:
        /**
         * @brief 从level -> string
         * @param level 日志级别,枚举类型
         */
        static std::string Getlevelstring(const Level &level)
        {
            switch (level)
            {
#define getstring(name) \
    case Level::name:   \
        return #name;
                getstring(DEBUG);
                getstring(INFO);
                getstring(WARN);
                getstring(ERROR);
                getstring(FATAL);
#undef getstring
            }
            return "NOKNOW";
        }

        /**
         * @brief 从string -> level
         * @param val 日志级别的字符串名称
         */
        static blue::Level Getstringlevel(const std::string &val)
        {
#define getlevel(name, str)          \
    if (val == #str || val == #name) \
        return Level::name;
            getlevel(DEBUG, debug);
            getlevel(INFO, info);
            getlevel(WARN, warn);
            getlevel(ERROR, error);
            getlevel(FATAL, fatal);
#undef getlevel
            return Level::NOKNOW;
        }

        // 对<<进行重载,支持直接输出enum class
        friend std::ostream &operator<<(std::ostream &os, Level level);
    };
}

#endif // __BLUE_LOG_H__