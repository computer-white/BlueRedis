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
#include <map>
#include "util.h"
#include "singleton.h"
#include "mthread.h"
// 宏展开输出
#define BLUE_LOG_LEVER(logger, level)                                                             \
    if (logger->getlevel() <= level)                                                              \
    blue::LogEventWrap(blue::LogEvent::LogEventPtr(new blue::LogEvent(logger, level, __FILE__,    \
                                                                      time(0), __LINE__,          \
                                                                      0, blue::GetThreadId(),     \
                                                                      blue::GetFiberID(),         \
                                                                      blue::Mthread::GetName()))) \
        .getstringstream()

#define BLUE_LOG_DEBUGE(logger) BLUE_LOG_LEVER(logger, blue::Level::DEBUG)
#define BLUE_LOG_INFO(logger) BLUE_LOG_LEVER(logger, blue::Level::INFO)
#define BLUE_LOG_WARN(logger) BLUE_LOG_LEVER(logger, blue::Level::WARN)
#define BLUE_LOG_ERROR(logger) BLUE_LOG_LEVER(logger, blue::Level::ERROR)
#define BLUE_LOG_FATAL(logger) BLUE_LOG_LEVER(logger, blue::Level::FATAL)

// format输出
#define BLUE_LOG_FORMAT_LEVEL(logger, fmt, level, ...)                                            \
    if (logger->getlevel() <= level)                                                              \
    blue::LogEventWrap(blue::LogEvent::LogEventPtr(new blue::LogEvent(logger, level, __FILE__,    \
                                                                      time(0), __LINE__, 0,       \
                                                                      blue::GetThreadId(),        \
                                                                      blue::GetFiberID(),         \
                                                                      blue::Mthread::GetName()))) \
        .getEvent()                                                                               \
        ->format(fmt, ##__VA_ARGS__)

#define BLUE_LOG_FORMAT_DEBUGE(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::DEBUG, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_INFO(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::INFO, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_WARN(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::WARN, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_ERROR(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::ERROR, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_FATAL(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::FATAL, ##__VA_ARGS__)

// 输出LogMessageRoot,默认输出到控制台
#define BLUE_LOG_MASSAGE_ROOT() blue::LoggerMgr::GetInstance()->getRoot()

// 用name获取Logger,如果不存在那么输出的位置跟BLUE_LOG_MASSAGE_ROOT() 得到的Logger输出位置一样
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
         * @return
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
         * @param fiberId    协程ID
         * @param name       线程名称
         * @return
         */
        LogEvent(std::shared_ptr<Logger> logger_ptr, Level level,
                 const char *file, uint64_t time, uint32_t line, uint32_t elapse,
                 uint32_t threadId, uint32_t fiberId, const std::string &name);

        /**
         * @brief 获取文件名称
         * @return 文件名称
         */
        const char *getFilename() const { return m_file; }

        /**
         * @brief 获取时间戳
         * @return 时间戳
         */
        uint64_t getTime() const { return m_time; }

        /**
         * @brief 获取行号
         * @return 行号
         */
        uint32_t getLines() const { return m_lines; }

        /**
         * @brief 获取系统到现在启动时间
         * @return 系统到现在启动时间
         */
        uint32_t getElapse() const { return m_elapse; }

        /**
         * @brief 获取线程id
         * @return 线程id
         */
        uint32_t getThreadId() const { return m_threadID; }

        /**
         * @brief 获取协程id
         * @return 协程id
         */
        uint32_t getFiberId() const { return m_fiberID; }

        /**
         * @brief 获取线程名称
         * @return 线程名称
         */
        const std::string &getThreadName() const { return m_threadname; }

        /**
         * @brief 获取消息体
         * @return 内容
         */
        std::string getContent() const { return m_stringstream.str(); }

        /**
         * @brief 获取日志器智能指针
         * @return 日志器智能指针
         */
        std::shared_ptr<Logger> getLoggerptr() const { return m_logger_ptr; }

        /**
         * @brief 获取string流,用于输入内容
         * @return std::stringstream
         */
        std::stringstream &getstringstream() { return m_stringstream; }

        /**
         * @brief 获取日志级别
         * @return level
         */
        Level getLevel() const { return m_level; }

        /**
         * @brief 利用fmt输出的辅助函数
         * @return
         */
        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        // 32个字节:内存对齐
        const char *m_file = nullptr;         // 文件名
        uint64_t m_time = 0;                  // 时间戳
        uint32_t m_lines = 0;                 // 行号
        uint32_t m_elapse = 0;                // 程序到现在启动时间(毫秒)
        uint32_t m_threadID = 0;              // 线程id
        uint32_t m_fiberID = 0;               // 协程id
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
    }; // Lervel

    class LogEventWrap
    {
    public:
        /**
         * @brief 日志事件的包装类构造函数
         * @param e 日志事件类智能指针
         * @return
         */
        LogEventWrap(LogEvent::LogEventPtr e);
        /**
         * @brief 日志事件的包装类析构函数
         * @param e 日志事件类智能指针
         * @return
         * @note 当析构函数发生时,才真正执行对日志的写入
         */
        ~LogEventWrap();

        /**
         * @brief 获取event智能指针
         * @return event智能指针
         */
        LogEvent::LogEventPtr getEvent() const { return m_event_ptr; }
        /**
         * @brief 获取event上的string流
         * @return string 流
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
         * @return
         * @note 包含对日志输出格式的解析
         */
        LogFormatter(const std::string &pattern);

        /**
         * @brief 将event上的内容输出为string,写入输出日志的具体格式内容
         * @param logger_ptr 日志器智能指针
         * @param level 日志级别
         * @param event 日志事件指针
         * @return string,日志信息
         */
        std::string format(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event);

        /**
         * @brief 获取输出格式字符串
         * @return 输出格式字符串
         */
        const std::string getPattern() const { return m_pattern; }

        /**
         * @brief 获取模式字符串是否有错误
         * @return 有错误返回true 否则 false
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
         * @return
         */
        void _init();

    private:
        std::string m_pattern; // 一旦从构造函数加载好不会再去做修改
        std::vector<FormatterItem::FormatterItemPtr> m_items;
        std::atomic<bool> m_HasError = {false};
    }; // LogFormatter

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
         * @return formatter指针
         * @note 无锁实现
         */
        LogFormatter::LogFormatterPtr getformatter() const;

        /**
         * @brief 设置formatter指针
         * @param fomatter 新的formatter指针
         * @return
         * @note 无锁实现
         */
        void setformatter(LogFormatter::LogFormatterPtr formatter);

        /**
         * @brief 获取日志级别
         * @return 日志级别
         * @note 原子操作
         */
        Level getLevel() const { return m_level.load(std::memory_order_acquire); }

        /**
         * @brief 设置日志级别
         * @param level 新的日志级别
         * @return
         * @note 原子操作
         */
        void setLevel(Level level) { m_level.store(level, std::memory_order_release); }

        /**
         * @brief 获取是否有设置输出格式
         * @return 有就返回true 否则false
         * @note 原子操作
         */
        const bool gatHasformatter() const { return m_hasformatter.load(std::memory_order_acquire); }

    protected:
        mutable MutexType m_mutex;
        std::atomic<Level> m_level = Level::DEBUG;
        LogFormatter::LogFormatterPtr m_formatter;
        std::atomic<bool> m_hasformatter = false;
    }; // LogAppender

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
         * @return
         * @note 无锁实现
         */
        void Log(Level level, LogEvent::LogEventPtr event);

        /**
         * @brief 日志构造函数
         * @param name 日志的名称,默认为root
         * @return
         * @note level级别默认为DEBUG,会在构造时设置一个标准的formatter格式
         */
        Logger(const std::string &name = "root");

        void debug(LogEvent::LogEventPtr event);
        void info(LogEvent::LogEventPtr event);
        void warn(LogEvent::LogEventPtr event);
        void error(LogEvent::LogEventPtr event);
        void fatal(LogEvent::LogEventPtr event);

        /**
         * @brief 将日志信息转为yamlstring
         * @return yamlstring
         * @note 将日志名称，日志级别，日志格式，日志输出器内容以无锁形式写入到yaml节点
         */
        std::string toyamlString();

        /**
         * @brief 添加日志输出目的地
         * @param Appender 需要添加的日志输出目的地智能指针
         * @return 内部含有写锁
         */
        void addAppender(LogAppender::LogAppenderPtr Appender);

        /**
         * @brief 删除日志输出目的地
         * @param Appender 需要删除的日志输出目的地智能指针
         * @return 内部含有写锁
         */
        void delAppender(LogAppender::LogAppenderPtr Appender);

        /**
         * @brief 清除所有日志输出目的地
         * @return
         * @note 内部含有写锁
         */
        void clearAppender() noexcept;

        /**
         * @brief 获取日志级别
         * @return
         */
        Level getlevel() const { return m_level.load(std::memory_order_acquire); }

        /**
         * @brief 设置日志级别
         * @param val 需要设置的日志级别
         * @return
         */
        void setlevel(Level val) { m_level.store(val, std::memory_order_release); }

        /**
         * @brief 获取日志名称
         * @return
         */
        const std::string getname() const { return m_name; }

        /**
         * @brief 设置日志输出格式
         * @param rhs 日志输出格式类智能指针
         * @return
         * @note 无锁实现，有读锁进行对appender的复制
         */
        void setFormatter(LogFormatter::LogFormatterPtr rhs);

        /**
         * @brief 设置formatter格式,按照string格式字符串
         * @param rhs 日志输出格式字符串
         * @return
         * @note formatter有错误不给予设置,通过调用setFormatter的无锁版本实现
         */
        void setFormatter(const std::string &rhs);

        /**
         * @brief 获取日志输出格式类智能指针
         * @return 日志输出格式类智能指针
         * @note 无锁实现
         */
        LogFormatter::LogFormatterPtr getFormatter() const;

    private:
        mutable MutexType m_mutex;                          // 互斥变量
        std::string m_name;                                 // 日志名称
        std::list<LogAppender::LogAppenderPtr> m_Appenders; // Appender列表
        std::atomic<Level> m_level;                         // 日志级别
        LogFormatter::LogFormatterPtr m_formatter;          // 输出日志格式
        Logger::LoggerPtr m_root;                           // root
    }; // Logger

    // 输出到控制台的appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        using StdoutLogAppenderPtr = std::shared_ptr<StdoutLogAppender>;
        StdoutLogAppender() = default;
        ~StdoutLogAppender() = default;

        /**
         * @brief 将控制台的信息转为yaml,最后以字符串输出
         * @return yamlstring
         * @note 无锁
         */
        virtual std::string toyamlString() override;

        /**
         * @brief 将日志输出到控制台
         * @param logger_ptr 日志器指针
         * @param level 日志级别
         * @param event 日志事件
         * @return
         * @note 无锁
         */
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override;

    private:
        std::string m_name = "console"; // 输出目的名称(控制台)
    }; // StdoutLogAppender

    // 输出到文件的appender
    class FileoutLogAppender : public LogAppender
    {
    public:
        ;
        using FileoutLogAppenderPtr = std::shared_ptr<FileoutLogAppender>;
        FileoutLogAppender() = default;

        /**
         * @brief 日志输出到文件类的析构函数,执行对文件的关闭
         * @return
         */
        ~FileoutLogAppender();

        /**
         * @brief 日志输出到文件类的构造函数
         * @param filename 文件名
         * @return
         */
        FileoutLogAppender(const std::string &filename);

        /**
         * @brief 重新打开文件
         * @return
         * @note 有锁
         */
        void reopen();

        /**
         * @brief 清空文件
         * @return
         * @note 有锁
         */
        void clear();

        /**
         * @brief 文件信息转为yamlstring,最后以字符串输出
         * @return yamlstring
         * @note 无锁
         */
        virtual std::string toyamlString() override;

        /**
         * @brief 将event事件和日志相关信息写入文件
         * @param logger_ptr 日志器智能指针
         * @param level 日志级别
         * @param event 日志事件
         * @return
         * @note 无锁
         */
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override;

    private:
        std::string m_filename;               // 文件名
        std::ofstream m_filestream;           // 文件输出流
        std::string m_name = "file";          // 输出目的名称(文件)
        std::atomic<uint64_t> m_lasttime = 0; // 文件最新时间
    }; // FileoutLogAppender

    // logger管理类,生成root(默认name = "root",具有formatter格式,\
通过logger的Appender方法来给没有formatter的Appender设置formatter)
    class LoggerManager
    {
    public:
        using MutexType = blue::MRWmutex;

        /**
         * @brief 构造函数,管理一个默认的logger智能指针
         * @return
         * @note 设置了默认的appender
         */
        LoggerManager();

        /**
         * @brief 按照名称获取logger
         * @param name 日志的名称
         * @return
         * @note 没有找到name对应的Logger,我们创建一个新的,然后让他输出到跟LoggerManager::m_rootr一样,直到它设置了他自己的Appender
         */
        Logger::LoggerPtr getLogger(const std::string &name);

        /**
         * @brief 获取管理的默认的root
         * @return 日志器的智能指针
         */
        Logger::LoggerPtr getRoot() const { return m_root; }

        /**
         * @brief 初始化logManager
         * @return
         */
        void init();

        /**
         * @brief 将loggerManager管理的日志器信息全部转为yaml,最后以字符串输出
         * @return yamlstring
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
        // 两种静态方法,支持序列化和反序列化,在读取yaml文件时配置起来方便

        /**
         * @brief 从level -> string
         * @param level 日志级别,枚举类型
         * @return string
         */
        static std::string Getlevelstring(const Level& level)
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
         * @return Level,枚举类型
         */
        static blue::Level Getstringlevel(const std::string &val)
        {
#define getlever(name, str)          \
    if (val == #str || val == #name) \
        return Level::name;
            getlever(DEBUG, debug);
            getlever(INFO, info);
            getlever(WARN, warn);
            getlever(ERROR, error);
            getlever(FATAL, fatal);
#undef getlever
            return Level::NOKNOW;
        }

        // 对<<进行重载,支持直接输出enum class
        friend std::ostream &operator<<(std::ostream &os, Level level);
    };
}

#endif // __BLUE_LOG_H__