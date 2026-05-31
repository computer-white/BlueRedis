#include <iostream>
#include <ctype.h>
#include <ctime>
#include <functional>
#include <stdarg.h>
#include <iomanip>
#include "log.h"
#include "config.h"

// 日志模块
namespace blue
{
    // 重载枚举类的 <<
    std::ostream &operator<<(std::ostream &os, Level level)
    {
        os << EnumTraits<blue::Level>::Getlevelstring(level);
        return os;
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger_ptr, Level level,
                       const char *file, uint64_t time, uint32_t line,
                       uint32_t elapse, uint32_t threadId, uint32_t fiberId,
                       const std::string &name)
        : m_file(file), m_time(time), m_lines(line), m_elapse(elapse),
          m_threadID(threadId), m_fiberID(fiberId), m_level(level), m_logger_ptr(logger_ptr),
          m_threadname(name)
    {
    }

    LogEventWrap::LogEventWrap(LogEvent::LogEventPtr e) : m_event_ptr(e)
    {
    }
    LogEventWrap::~LogEventWrap()
    {
        // 把LogEvent自己写进去
        m_event_ptr->getLoggerptr()
            ->Log(m_event_ptr->getLevel(),
                  m_event_ptr);
    }

    std::stringstream &LogEventWrap::getstringstream()
    {
        return m_event_ptr->getstringstream();
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_stringstream << std::string(buf, len);
            free(buf);
        }
    }

    // 消息体
    class MessageFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        MessageFormatterItem(const std::string &str = "") {}
        ~MessageFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getContent();
        }
    };

    // lever
    class LeverFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        LeverFormatterItem(const std::string &str = "") {}
        ~LeverFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << EnumTraits<Level>::Getlevelstring(level);
        }
    };

    // Elapse启动后的时间
    class ElapseFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        ElapseFormatterItem(const std::string &str = "") {}
        ~ElapseFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getElapse();
        }
    };

    // 日志名称
    class LogNameFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        LogNameFormatterItem(const std::string &str = "") {}
        ~LogNameFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getLoggerptr()->getname();
        }
    };

    // 线程id
    class ThreadIdFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        ThreadIdFormatterItem(const std::string &str = "") {}
        ~ThreadIdFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getThreadId();
        }
    };

    // 线程名称
    class ThreadNameFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        ThreadNameFormatterItem(const std::string &str = "") {}
        ~ThreadNameFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getThreadName();
        }
    };

    // 协程id
    class FiberFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        FiberFormatterItem(const std::string &str = "") {}
        ~FiberFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getFiberId();
        }
    };

    // 时间
    class DataTimeFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        DataTimeFormatterItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        ~DataTimeFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            uint64_t e_time = event->getTime();
            time_t m_time = static_cast<time_t>(e_time);
            std::tm time_local;
#ifdef _WIN32
            localtime_s(&time_local, &m_time);
#else
            localtime_r(&m_time, &time_local);
#endif
            // std::put_time转为当地时间(北京时间)
            os << std::put_time(&time_local, m_format.c_str());
        }

    private:
        std::string m_format;
    };

    // 文件名
    class FileNameFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        FileNameFormatterItem(const std::string &str = "") {}
        ~FileNameFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getFilename();
        }
    };

    // 行号
    class LinesFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        LinesFormatterItem(const std::string &str = "") {}
        ~LinesFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << event->getLines();
        }
    };

    // 换行符
    class NewLinesFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        NewLinesFormatterItem(const std::string &str = "") {}
        ~NewLinesFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << std::endl;
        }
    };

    // string,nstr : 普通文本
    class StringFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        StringFormatterItem(const std::string &str) : m_string(str)
        {
        }
        ~StringFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    // Tap
    class TapFormatterItem : public LogFormatter::FormatterItem
    {
    public:
        TapFormatterItem(const std::string &str = "")
        {
        }
        ~TapFormatterItem() = default;
        void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override
        {
            os << "\t";
        }
    };

    Logger::Logger(const std::string &name) : m_name(name), m_level(Level::DEBUG)
    {
        /*
        %d{%Y-%m-%d %H:%M:%S}%T%t%T%m%n
        %m: 消息体 -
        %p: lever -
        %r: 启动后的时间 -
        %c: 日志名称 -
        %t: 线程id -
        %n: 回车换行
        %d: 时间
        %f: 文件名
        %l: 行号
        */
        // 设置解析格式 : 日期(%d)，线程名称(%N)，线程ID(%t)，协程ID(%F)，日志级别(%p)，日志名称(%c)，文件名(%f)，行号(%l)，文本内容(%m)，换行符(%n)
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")); // 调用LogFormatter构造函数，并调用init()
    }

    void Logger::addAppender(LogAppender::LogAppenderPtr Appender)
    {
        // 选择在Appeder没有formatter时使用logger的formatter,若不选择伴随有Appender中的formatter为空的风险
        // MutexType::lockSco lock(m_mutex);
        if (!Appender->getformatter())
        {
            // logger的formatter一开始就创建了，\
            那么在通过LoggerManager新建立一个root时，会加入一个新的Appender\
            如果Appender没有设置formatter这里就可以给Appender设置formatter
            // Appender->m_formatter = this->m_formatter;

            std::atomic_store_explicit(&Appender->m_formatter, this->m_formatter, std::memory_order_release);
        }
        MutexType::WritelockSco lock(m_mutex);
        m_Appenders.push_back(Appender);
    }

    void Logger::delAppender(LogAppender::LogAppenderPtr Appender)
    {
        MutexType::WritelockSco lock(m_mutex);
        for (auto it = m_Appenders.begin(); it != m_Appenders.end(); it++)
        {
            if (*it == Appender)
            {
                // MutexType::lockSco ll(Appender->m_mutex);
                m_Appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppender() noexcept
    {
        MutexType::WritelockSco lock(m_mutex);
        m_Appenders.clear();
    }

    std::string Logger::toyamlString()
    {
        YAML::Node node;
        // 有锁
        // MutexType::WritelockSco lock(m_mutex);
        // node["name"] = m_name;
        // if (m_level != blue::Level::NOKNOW)
        // {
        //     node["level"] = EnumTraits<blue::Level>::Getlevelstring(m_level);
        // }
        // if (m_formatter)
        // {
        //     node["formatter"] = m_formatter->getPattern();
        // }
        // if (!m_Appenders.empty())
        // {
        //     YAML::Node curr_node = YAML::Node(YAML::NodeType::Sequence);
        //     for (auto &appender : m_Appenders)
        //     {
        //         // 解析每个 appender 的 YAML 字符串
        //         YAML::Node appender_node = YAML::Load(appender->toyamlString());
        //         curr_node.push_back(appender_node);
        //     }
        //     node["appenders"] = curr_node;
        // }

        node["name"] = m_name;
        auto level = m_level.load(std::memory_order_acquire);
        auto formatter = std::atomic_load_explicit(&m_formatter, std::memory_order_acquire);
        if (level != blue::Level::NOKNOW)
        {
            node["level"] = EnumTraits<blue::Level>::Getlevelstring(level);
        }
        if (formatter)
        {
            node["formatter"] = formatter->getPattern();
        }
        // 2. 复制 appender 列表（读锁保护）
        std::list<blue::LogAppender::LogAppenderPtr> appenders_copy;
        {
            MutexType::ReadlockSco lock(m_mutex); // 这里后序可以改为sharted_lock<>
            appenders_copy = m_Appenders;         // 复制 shared_ptr，引用计数增加
        }
        if (!appenders_copy.empty())
        {
            YAML::Node curr_node = YAML::Node(YAML::NodeType::Sequence);
            for (auto &appender : appenders_copy)
            {
                // 解析每个 appender 的 YAML 字符串
                YAML::Node appender_node = YAML::Load(appender->toyamlString());
                curr_node.push_back(appender_node);
            }
            node["appenders"] = curr_node;
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void Logger::setFormatter(LogFormatter::LogFormatterPtr rhs)
    {

        // MutexType::lockSco lock(m_mutex);
        // m_formatter = rhs;
        // for (auto &it : m_Appenders)
        // {
        //     if (!it->gatHasformatter())
        //     {
        //         it->setformatter(m_formatter);
        //     }
        // }

        std::atomic_store_explicit(&m_formatter, rhs, std::memory_order_release);

        // 2. 复制 appender 列表（读锁保护）
        std::list<blue::LogAppender::LogAppenderPtr> appenders_copy;
        {
            MutexType::ReadlockSco lock(m_mutex); // 这里后序可以改为sharted_lock<>
            appenders_copy = m_Appenders;         // 复制 shared_ptr，引用计数增加
        }
        // 将那些没有自己的formatter的Appender设置为更改后的,他们依赖于Logger的formatter
        for (auto &it : appenders_copy)
        {
            if (!it->gatHasformatter())
            {
                it->setformatter(rhs);
            }
        }
    }

    void Logger::setFormatter(const std::string &rhs)
    {
        if (m_formatter->getHasError())
        {
            MutexType::WritelockSco lock(m_mutex);
            std::cout << " Logger::setFormatter(const std::string& ) Error, name is "
                      << m_name << " pattern is " << rhs << std::endl;
            return;
        }
        blue::LogFormatter::LogFormatterPtr new_formatter =
            std::make_shared<blue::LogFormatter>(rhs);
        setFormatter(new_formatter);
    }

    LogFormatter::LogFormatterPtr Logger::getFormatter() const
    {
        // 有锁
        // MutexType::WritelockSco lock(m_mutex);
        // return m_formatter;

        // 无锁
        auto formatter = std::atomic_load_explicit(&m_formatter, std::memory_order_acquire);
        return formatter;
    }

    void Logger::Log(Level level, LogEvent::LogEventPtr event)
    {
        auto curr_level = m_level.load(std::memory_order_acquire);
        // 2. 复制 appender 列表（读锁保护）
        std::list<blue::LogAppender::LogAppenderPtr> appenders_copy;
        {
            MutexType::ReadlockSco lock(m_mutex);
            appenders_copy = m_Appenders; // 复制 shared_ptr，引用计数增加
        }
        if (level >= curr_level)
        {
            auto self = shared_from_this();
            if (!appenders_copy.empty())
            {
                for (auto &it : appenders_copy)
                {
                    it->log(self, level, event);
                }
            }
            else if (m_root && this != m_root.get())
            {
                // 如果这个logger没有自己的Appender,那么调用root的Log,也就是调用root的Appender
                m_root->Log(level, event);
            }
            else
            {
                MutexType::WritelockSco lock(m_mutex);
                std::cout << " No appender configured for root logger! " << __FILE__
                          << " " << __LINE__ << std::endl;
            }
        }
    }

    // logger的成员方法
    void Logger::debug(LogEvent::LogEventPtr event)
    {
        Log(Level::DEBUG, event);
    }

    // logger的成员方法
    void Logger::info(LogEvent::LogEventPtr event)
    {
        Log(Level::INFO, event);
    }

    // logger的成员方法
    void Logger::warn(LogEvent::LogEventPtr event)
    {
        Log(Level::WARN, event);
    }

    // logger的成员方法
    void Logger::error(LogEvent::LogEventPtr event)
    {
        Log(Level::ERROR, event);
    }

    // logger的成员方法
    void Logger::fatal(LogEvent::LogEventPtr event)
    {
        Log(Level::FATAL, event);
    }

    void LogAppender::setformatter(LogFormatter::LogFormatterPtr formatter)
    {
        auto old_formatter = std::atomic_exchange_explicit(&m_formatter, formatter, std::memory_order_acq_rel);
        bool tem = (old_formatter != nullptr);
        m_hasformatter.store(tem, std::memory_order_release);

        // 有锁
        // MutexType::lockSco lock(m_mutex);
        // m_formatter = formatter;
        // if (m_formatter)
        // {
        //     m_hasformatter.store(true,std::memory_order_release);
        // }
        // else
        // {
        //     m_hasformatter.store(false,std::memory_order_release);
        // }
    }

    LogFormatter::LogFormatterPtr LogAppender::getformatter() const
    {
        // MutexType::lockSco lock(m_mutex);
        // return m_formatter;
        auto formatter = std::atomic_load_explicit(&m_formatter, std::memory_order_acquire);
        return formatter;
    }

    FileoutLogAppender::FileoutLogAppender(const std::string &filename) : m_filename(filename)
    {
        reopen();
    }

    FileoutLogAppender::~FileoutLogAppender()
    {
        if (m_filestream.is_open())
        {
            m_filestream.close();
        }
    }

    void FileoutLogAppender::log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event)
    {

        // if (level >= m_level)
        // {
        //    uint64_t now = time(0);
        //    if (now != m_lasttime.load(std::memory_order_acquire)) {
        //        reopen();  // 假设 reopen 内部会处理文件操作
        //        m_lasttime.store(now, std::memory_order_release);
        //    }
        //     MutexType::lockSco lock(m_mutex);
        //     std::string formatted = m_formatter->format(logger_ptr, level, event);
        //     std::cout << "写入文件 " << formatted;
        //     // 写入文件
        //     m_filestream << formatted;
        //     m_filestream.flush();
        // }

        auto curr_level = m_level.load(std::memory_order_acquire);
        if (level < curr_level)
        {
            return;
        }

        auto formatter = std::atomic_load_explicit(&m_formatter,
                                                   std::memory_order_acquire);

        // 格式化消息（在锁外执行，减少锁持有时间）
        std::string formatted = formatter->format(logger_ptr, level, event);
        // 加锁保护文件写入
        {
            MutexType::lockSco lock(m_mutex);
            // 检查是否需要重新打开文件
            uint64_t now = time(0);
            if (now != m_lasttime.load(std::memory_order_acquire))
            {
                // reopen();  // 假设 reopen 内部会有锁 ,替换为原地实现，否则可能有死锁风险
                if (m_filestream.is_open())
                {
                    m_filestream.flush();
                    m_filestream.close();
                }
                m_filestream.open(m_filename, std::ios_base::app);
                m_lasttime.store(now, std::memory_order_release);
            }
            // 写入文件
            m_filestream << formatted;
            m_filestream.flush();
        }
    }

    void FileoutLogAppender::clear()
    {
        MutexType::lockSco lock(m_mutex);
        if (m_filestream.is_open())
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios_base::trunc);
        reopen();
    }

    void FileoutLogAppender::reopen()
    {
        MutexType::lockSco lock(m_mutex);
        if (m_filestream.is_open())
        {
            m_filestream.flush();
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios_base::app);
    }

    std::string FileoutLogAppender::toyamlString()
    {
        YAML::Node node;
        // 有锁
        // MutexType::lockSco lock(m_mutex);
        // node["name"] = m_name;
        // node["type"] = "FileoutLogAppender";
        // node["file"] = m_filename;
        // if (m_level != blue::Level::NOKNOW)
        // {
        //     // Getlevelstring是没有加锁
        //     node["level"] = EnumTraits<blue::Level>::Getlevelstring(m_level);
        // }

        // // m_formatter一定有(要么是自己的，要么是父亲的)
        // node["formatter"] = m_hasformatter.load(std::memory_order_acquire) ?
        //  m_formatter->getPattern() :
        //  m_formatter->getPattern() + " (father's)";

        // 无锁
        node["name"] = m_name;
        node["type"] = "FileoutLogAppender";
        node["file"] = m_filename;
        auto level = m_level.load(std::memory_order_acquire);
        auto hasformatter = m_hasformatter.load(std::memory_order_acquire);
        auto formatter = std::atomic_load_explicit(&m_formatter, std::memory_order_acquire);
        if (level != blue::Level::NOKNOW)
        {
            // Getlevelstring是没有加锁
            node["level"] = EnumTraits<blue::Level>::Getlevelstring(level);
        }

        // m_formatter一定有(要么是自己的，要么是父亲的)
        node["formatter"] = hasformatter ? formatter->getPattern() : formatter->getPattern() + " (father's)";
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void StdoutLogAppender::log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event)
    {
        // 有锁
        // if (level >= m_level)
        // {
        //     MutexType::lockSco lock(m_mutex);
        //     std::cout << m_formatter->format(logger_ptr,level,event) << std::endl;
        // }

        // 无锁
        auto curr_level = m_level.load(std::memory_order_acquire);
        if (level < curr_level)
        {
            return;
        }

        auto formatter = std::atomic_load_explicit(&m_formatter,
                                                   std::memory_order_acquire);
        std::string formatted = formatter->format(logger_ptr, level, event);
        {
            MutexType::lockSco lock(m_mutex);
            std::cout << formatted << std::endl;
        }
    }

    std::string StdoutLogAppender::toyamlString()
    {
        YAML::Node node;
        // 有锁实现
        // MutexType::lockSco lock(m_mutex);
        // node["name"] = m_name;
        // node["type"] = "StdoutLogAppender";
        // if (m_level != blue::Level::NOKNOW)
        // {
        //     node["level"] = EnumTraits<blue::Level>::Getlevelstring(m_level);
        // }
        // node["formatter"] = m_hasformatter ? m_formatter->getPattern() : m_formatter->getPattern() + " (father's)";

        // 无锁实现
        node["name"] = m_name;
        node["type"] = "StdoutLogAppender";
        auto level = m_level.load(std::memory_order_acquire);
        auto hasformatter = m_hasformatter.load(std::memory_order_acquire);
        auto formatter = std::atomic_load_explicit(&m_formatter, std::memory_order_acquire);
        if (level != blue::Level::NOKNOW)
        {
            // Getlevelstring是没有加锁
            node["level"] = EnumTraits<blue::Level>::Getlevelstring(level);
        }

        // m_formatter一定有(要么是自己的，要么是父亲的)
        node["formatter"] = hasformatter ? formatter->getPattern() : formatter->getPattern() + " (father's)";
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        _init();
    }
    /*
        %d{%Y-%m-%d %H:%M:%S}%T%t%T%m%n
        %m: 消息体 -
        %p: lever -
        %r: 启动后的时间 -
        %c: 日志名称 -
        %t: 线程id -
        %n: 回车换行
        %d: 时间
        %f: 文件名
        %l: 行号
        %T: 制表符
    */

    void LogFormatter::_init()
    {
        /*
            nstr : 存放普通文本
            vec : 存放一个三元组(str,fmt,type)
            str : 表示%后面的内容
            fmt : 表示{..}括号内的内容
            fmt_status : 表示fmt的状态，处于解析状态 1,解析完毕 2
        */
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); i++)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            // 现在遇到%

            // 处理 %%
            if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '%')
            {
                nstr.append(1, m_pattern[i + 1]);
                ++i;
                continue;
            }

            // %后不是百分号
            std::string str; // 存放%后面的类型
            std::string fmt; // 存放{..}里面的内容

            size_t pre = i + 1;   // 指向% 后的第一个字符
            int fmt_status = 0;   // fmt状态
            size_t fmt_begin = 0; // 表示fmt开始位置
            while (pre < m_pattern.size())
            {
                // 错误类型,没有遇到{ 或 }之前遇到了非字母字符,不再继续下去
                if (!fmt_status && (!isalpha(m_pattern[pre]) && m_pattern[pre] != '{' && m_pattern[pre] != '}'))
                {
                    str = m_pattern.substr(i + 1, pre - i - 1); // 拿到有用的str
                    break;
                }
                if (m_pattern[pre] == '{')
                {
                    str = m_pattern.substr(i + 1, pre - i - 1); // str扫描完毕
                    fmt_status = 1;                             // 标记fmt_status = 1,表示开始解析{..}
                    ++pre;
                    fmt_begin = pre; // 表示fmt开始位置
                    continue;
                }
                else if (m_pattern[pre] == '}')
                {
                    fmt = m_pattern.substr(fmt_begin, pre - fmt_begin); // fmt扫描完毕
                    fmt_status = 2;                                     // 标记fmt_status = 2,表示解析完毕
                    ++pre;
                    break;
                }
                ++pre;
                if (pre == m_pattern.size())
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1); // %后面全是str
                    }
                }
            } // while

            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.emplace_back(nstr, std::string(), 0);
                    nstr.clear();
                }
                vec.emplace_back(str, fmt, 1);
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error: " << m_pattern << " - "
                          << m_pattern.substr(i) << std::endl;
                vec.emplace_back("<<pattern_error>>", fmt, 0);
                m_HasError.store(true, std::memory_order_release);
            }
            else
            {
                if (!nstr.empty())
                {
                    vec.emplace_back(nstr, std::string(), 0);
                    nstr.clear();
                }
                vec.emplace_back(str, fmt, 2);
            }
            i = pre - 1; // while循环终止,pre要么指向 } ,要么指向一个错误类型，都需要写入nstr
        } // for

        if (!nstr.empty())
        {
            vec.emplace_back(nstr, std::string(), 0);
        }
        std::map<std::string, std::function<FormatterItem::FormatterItemPtr(const std::string &str)>> s_format_items =
            {
#define XX(str, Func)                                                                               \
    {                                                                                               \
        #str, [](const std::string &fmt) { return FormatterItem::FormatterItemPtr(new Func(fmt)); } \
    }

                XX(m, MessageFormatterItem),   // m:文本内容
                XX(p, LeverFormatterItem),     // p:日志级别
                XX(r, ElapseFormatterItem),    // r:程序到现在启动时间(ms)
                XX(c, LogNameFormatterItem),   // c:日志名称
                XX(t, ThreadIdFormatterItem),  // t:线程ID
                XX(n, NewLinesFormatterItem),  // n:换行符
                XX(d, DataTimeFormatterItem),  // d:日期
                XX(f, FileNameFormatterItem),  // f:文件名
                XX(l, LinesFormatterItem),     // l:行号
                XX(T, TapFormatterItem),       // T:Tap键
                XX(F, FiberFormatterItem),     // F:协程ID
                XX(N, ThreadNameFormatterItem) // N:线程名称
#undef XX
            };

        for (auto &[str, fmt, type] : vec) // c++17结构化绑定
        {
            if (type == 0)
            {
                m_items.push_back(FormatterItem::FormatterItemPtr(new StringFormatterItem(str)));
            }
            else
            {
                auto it = s_format_items.find(str);
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatterItem::FormatterItemPtr(new StringFormatterItem("<<Error_Fomat %" + str + ">>")));
                    m_HasError.store(true, std::memory_order_release);
                }
                else
                {
                    m_items.push_back(it->second(fmt));
                }
            }

            // std::cout << "(" << str << ") - (" << fmt << ") - (" << type << ")" << std::endl;
        }

        /*
        %m: 消息体 -
        %p: lever -
        %r: 启动后的时间 -
        %c: 日志名称 -
        %t: 线程id -
        %n: 回车换行
        %d: 时间
        %f: 文件名
        %l: 行号
        */
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event)
    {
        std::stringstream ss;
        for (auto &it : m_items)
        {
            it->format(ss, logger_ptr, level, event);
        }
        // std::cout << m_items.size() << std::endl;
        return ss.str();
    }

    LoggerManager::LoggerManager()
    {

        m_root.reset(new Logger); // 默认name为root,并且默认有一个正确的formatter格式
        auto console_appender = std::make_shared<blue::StdoutLogAppender>();
        // addAppener已经加了锁
        m_root->addAppender(console_appender);
        // 把创建的root放入m_logger
        MutexType::WritelockSco lock(m_mutex);
        m_logger[m_root->getname()] = m_root;
        init();
    }

    Logger::LoggerPtr LoggerManager::getLogger(const std::string &name)
    {
        {
            MutexType::ReadlockSco lock(m_mutex);
            auto it = m_logger.find(name);
            if (it != m_logger.end())
            {
                return it->second;
            }
        }
        // 没有找到name对应的Logger,我们创建一个新的,然后让他输出到跟LoggerManager::m_rootr一样,直到它设置了他自己的Appender
        Logger::LoggerPtr new_logger = std::make_shared<blue::Logger>(name);
        {
            MutexType::WritelockSco lock(m_mutex);
            auto it = m_logger.find(name);
            if (it != m_logger.end())
            {
                return it->second;
            }
            new_logger->m_root = m_root;
            // 添加新的new_logger进入m_logger
            m_logger[name] = new_logger;
            return new_logger;
        }
    }

    // 从yaml文件加载出来的LogAppender内容类型
    struct LogAppenderDefine
    {
        int type = 0;                            // type = 1(file),type = 2(std)
        blue::Level level = blue::Level::NOKNOW; // lever
        std::string formatter;                   // formatter
        std::string file;                        // 文件名
        std::string name;                        // 输出目的地名称
        LogAppenderDefine() = default;

        // 重载比较运算符
        bool operator==(const LogAppenderDefine &rhs) const
        {
            return type == rhs.type &&
                   level == rhs.level &&
                   formatter == rhs.formatter &&
                   file == rhs.file &&
                   name == rhs.name;
        }
    };

    // 从yaml文件加载出来的Log内容类型
    struct LogDefine
    {
        std::string name;                         // log的name
        blue::Level level = blue::Level::NOKNOW;  // lever
        std::string formatter;                    // formatter
        std::vector<LogAppenderDefine> appenders; // appenders
        LogDefine() = default;

        // 重载比较运算符
        bool operator==(const LogDefine &rhs) const
        {
            return name == rhs.name &&
                   level == rhs.level &&
                   formatter == rhs.formatter &&
                   appenders == rhs.appenders;
        }

        // 重载比较运算符
        bool operator<(const LogDefine &rhs) const
        {
            return name < rhs.name;
        }
    };

    // 特化string -> LogAppenderDefine
    template <>
    class LexicalCast<std::string, LogAppenderDefine>
    {
    public:
        LogAppenderDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            LogAppenderDefine p;
            if (!node["name"].IsDefined())
            {
                std::cout << "log config error LogAppenderDefine.name is null "
                          << __FILE__ << " " << __LINE__ << "\n"
                          << node << "]" << std::endl;
                return LogAppenderDefine();
            }

            p.name = node["name"].as<std::string>();

            if (!node["type"].IsDefined())
            {
                std::cout << "log config error LogAppenderDefine.type is null"
                          << __FILE__ << " " << __LINE__ << "\n[" << node << "]" << std::endl;
                return LogAppenderDefine();
            }

            std::string type = node["type"].as<std::string>();
            if (type == "FileoutLogAppender")
            {
                p.type = 1;
                if (!node["file"].IsDefined())
                {
                    std::cout << "log config error LogAppenderDefine.file is null "
                              << __FILE__ << " " << __LINE__ << "\n[" << node << "]" << std::endl;
                    return LogAppenderDefine();
                }
                p.file = node["file"].as<std::string>();
            }
            else if (type == "StdoutLogAppender")
            {
                p.type = 2;
            }
            else
            {
                std::cout << "log config error LogAppenderDefine.type is invalid "
                          << __FILE__ << " " << __LINE__ << "\n[" << node << "]" << std::endl;
                return LogAppenderDefine();
            }

            p.level = EnumTraits<blue::Level>::Getstringlevel(
                node["level"].IsDefined() ? node["level"].as<std::string>() : std::string());

            p.formatter = node["formatter"].IsDefined() ? node["formatter"].as<std::string>() : std::string();

            return p;
        }
    };

    // 特化LogAppenderDefine -> string
    template <>
    class LexicalCast<LogAppenderDefine, std::string>
    {
    public:
        std::string operator()(const LogAppenderDefine &val)
        {
            YAML::Node node;
            node["name"] = val.name;

            if (val.type == 1) // file
            {
                node["type"] = "FileoutLogAppender";

                node["file"] = val.file;
            }
            else if (val.type == 2) // std
            {
                node["type"] = "StdoutLogAppender";
            }

            node["level"] = EnumTraits<blue::Level>::Getlevelstring(val.level);

            if (!val.formatter.empty())
            {
                node["formatter"] = val.formatter;
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化string -> LogDefine
    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            LogDefine p;

            if (!node["name"].IsDefined())
            {
                std::cout << " log error val.LogDefine.name is null "
                          << __FILE__ << " " << __LINE__ << "\n[" << node << "]" << std::endl;
                return LogDefine();
            }

            p.name = node["name"].as<std::string>();

            p.level = EnumTraits<blue::Level>::Getstringlevel(
                node["level"].IsDefined() ? node["level"].as<std::string>() : std::string());

            if (node["formatter"].IsDefined())
            {
                p.formatter = node["formatter"].as<std::string>();
            }

            // 从 YAML 节点中读取 appenders
            if (node["appenders"].IsDefined() && node["appenders"].IsSequence())
            {
                for (const auto &appender_node : node["appenders"])
                {
                    LogAppenderDefine appender;
                    appender.name = appender_node["name"].as<std::string>();

                    if (!appender_node["type"].IsDefined())
                    {
                        std::cout << "log config error val.LogAppenderDefine.type is null "
                                  << __FILE__ << " " << __LINE__ << "\n[" << appender_node << std::endl;
                        continue;
                    }

                    std::string type = appender_node["type"].as<std::string>();
                    if (type == "FileoutLogAppender")
                    {
                        appender.type = 1;
                        if (!appender_node["file"].IsDefined())
                        {
                            std::cout << "log config error val.LogAppenderDefine.file is null "
                                      << __FILE__ << " " << __LINE__ << "\n[" << appender_node << std::endl;
                            continue;
                        }
                        appender.file = appender_node["file"].as<std::string>();
                    }
                    else if (type == "StdoutLogAppender")
                    {
                        appender.type = 2;
                    }
                    else
                    {
                        std::cout << "log config error val.LogAppenderDefine.type is invalid "
                                  << __FILE__ << " " << __LINE__ << "\n[" << appender_node << "]" << std::endl;
                        continue;
                    }

                    appender.level = EnumTraits<blue::Level>::Getstringlevel(
                        appender_node["level"].IsDefined() ? appender_node["level"].as<std::string>() : std::string());

                    appender.formatter =
                        appender_node["formatter"].IsDefined() ? appender_node["formatter"].as<std::string>() : std::string();

                    p.appenders.push_back(appender);
                }
            }
            return p;
        }
    };

    // 特化LogDefine -> string
    template <>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &val)
        {
            YAML::Node node;
            node["name"] = val.name;

            node["level"] = EnumTraits<blue::Level>::Getlevelstring(val.level);

            if (!val.formatter.empty())
            {
                node["formatter"] = val.formatter;
            }

            // 序列化 val.appenders
            if (!val.appenders.empty())
            {
                // 用于序列化val.appenders
                YAML::Node appenders_node = YAML::Node(YAML::NodeType::Sequence);
                for (const auto &appender : val.appenders)
                {
                    YAML::Node appender_node; // appenders_node中的节点,也就是序列化LogAppenderDefine的结果

                    appender_node["name"] = appender.name;

                    if (appender.type == 1)
                    {
                        appender_node["type"] = "FileoutLogAppender";

                        appender_node["file"] = appender.file;
                    }
                    else if (appender.type == 2)
                    {
                        appender_node["type"] = "StdoutLogAppender";
                    }

                    appender_node["level"] = EnumTraits<blue::Level>::Getlevelstring(appender.level);

                    if (!appender.formatter.empty())
                    {
                        appender_node["formatter"] = appender.formatter;
                    }

                    appenders_node.push_back(appender_node);
                }
                node["appenders"] = appenders_node;
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    blue::ConfigVar<std::set<LogDefine>>::ConfigVarPtr
        g_logDefine_config_ptr = blue::Config::Lookup<std::set<LogDefine>>("logs",
                                                                           std::set<LogDefine>(),
                                                                           "logs LogDefine");

    struct __LogIniter__
    {
        __LogIniter__()
        {
            // 添加监听器,同时读取内容设置到日志系统里面
            g_logDefine_config_ptr->addListener([](const std::set<LogDefine> &old_val,
                                                   const std::set<LogDefine> &new_val)
                                                {
            BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) 
            << " on_change_cb conf changed! ";
            // 新增
            for (auto& n_val : new_val)
            {
                auto it = old_val.find(n_val);
                blue::Logger::LoggerPtr new_logger;
                if (it == old_val.end())
                {
                    // 新增,利用名字查找,这样就会将新增的放入到\
                    Message管理的logger里面,同时设置了默认的Appender
                    new_logger = BLUE_LOG_NAME(n_val.name);
                }
                else
                {
                    if (*it == n_val)
                    {
                        continue;
                    }
                    // 修改
                    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT())  
                    << " Update logger: [" << n_val.name << "]";
                    new_logger = BLUE_LOG_NAME(n_val.name);
                }
                // 这里也可以不用判断,因为不知道文件的level是什么或者设置的是错误的,那么有必要去提醒？
                if (n_val.level != blue::Level::NOKNOW)
                {
                    new_logger->setlevel(n_val.level);
                }
                if (!n_val.formatter.empty())
                {
                    new_logger->setFormatter(n_val.formatter);
                }
                // 清除默认的new_logger的Appender
                new_logger->clearAppender();
                for (auto& a : n_val.appenders)
                {
                    blue::LogAppender::LogAppenderPtr new_appender;
                    if (a.type == 1) // file
                    {
                        new_appender.reset(new blue::FileoutLogAppender(a.file));
                    }
                    else if (a.type == 2) // std
                    {
                        new_appender.reset(new blue::StdoutLogAppender);
                    }
                    else
                    {
                        BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                        << " LogDefine.type的值未知 type : [" << a.type << "]";
                        continue;
                    }
                    if (a.level != blue::Level::NOKNOW)
                    {
                        new_appender->setLevel(a.level);
                    }
                    if (!a.formatter.empty())
                    {
                        auto a_formatter = 
                        std::make_shared<blue::LogFormatter>(a.formatter);
                        if (!a_formatter->getHasError())
                        {
                            new_appender->setformatter(a_formatter);
                        }
                        else
                        {
                            // 有formatter但是解析出来有错误，我们不添加到new_logger里面
                            std::cout << "log config error n_val.LogAppenderDefine.fomatter is error "
                                      << __FILE__ << " " << __LINE__ << std::endl;
                            continue;
                        }
                    }
                    // 这里addAppender时如果new_appender没有自己的forrmatter,就会被设置为new_logger的formatter
                    new_logger->addAppender(new_appender);
                }
            }
            // 删除
            for (auto& o_val : old_val)
            {
                auto it = new_val.find(o_val);
                if (it == new_val.end())
                {
                    BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT())  
                    << " Remove logger: [" << o_val.name << "]";
                    auto logger = BLUE_LOG_NAME(o_val.name);
                    if (logger)
                    {
                        logger->clearAppender();
                        logger->setlevel(static_cast<blue::Level>(100));
                    }
                }
            } });
        }
    };

    std::string LoggerManager::toyamlString()
    {
        YAML::Node node = YAML::Node(YAML::NodeType::Sequence);
        MutexType::WritelockSco lock(m_mutex);
        for (auto &[key, logger] : m_logger)
        {
            node.push_back(YAML::Load(logger->toyamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    // 全局变量,再main函数之前就会被创建(调用构造函数)
    static __LogIniter__ __S_Log_Initer__;
    void LoggerManager::init() {}

}
