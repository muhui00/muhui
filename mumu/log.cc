/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : log.cpp
 * Author      : muhui
 * Created date: 2022-11-03 20:20:06
 * Description : 
 *
 *******************************************/

#include <functional>
#include <array>
#include <time.h>

#include "config.h"
#include "util.h"
#include "log.h"

namespace muhui
{
LogEventWrap::LogEventWrap(LogEvent::ptr e)
    : m_event(e)
{}
LogEventWrap::~LogEventWrap()
{
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}
std::stringstream& LogEventWrap::getss()
{
    return m_event->getSS();
}

/*=============LogLevel==============*/
const char* LogLevel::ToString(LogLevel::Level level)
{
    switch(level){
#define XX(name) \
        case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }   
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str)
{
#define XX(level, v) \
    if(str == #v){ \
        return LogLevel::level; \
    } 
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);


    return LogLevel::UNKONW;
#undef XX
}
class MessageFormatItem : public LogFormatter::FormatItem
{
public:
    MessageFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem
{
public:
    LevelFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << LogLevel::ToString(level);
    }
};

//日志名
class NameFormatItem : public LogFormatter::FormatItem
{
public:
    NameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getLogger()->getName();
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem
{
public:
    ElapseFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getElapse();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
    ThreadIdFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem
{
public:
    FiberIdFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem
{
public:
    ThreadNameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadName();
    }
};
class DateTimeFormatItem : public LogFormatter::FormatItem
{
public:
    DateTimeFormatItem(const std::string format = "%Y:%m:%d %H:%M:%S")
        : m_format(format)
    {
        if(m_format.empty())
        {
            m_format = "%Y-%m-%d  %H:%M:%S";
        }
    }
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem
{
public:
    FilenameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFilename();
    }
};

class LineFormatItem : public LogFormatter::FormatItem
{
public:
    LineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem
{
public:
    NewLineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem
{
public:
    StringFormatItem(const std::string& str)
        : m_string(str) {} 
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << m_string;
    }
private:
    std::string m_string;
};
class TabFormatItem : public LogFormatter::FormatItem
{
public:
    TabFormatItem(const std::string& str)
        : m_string(str) {} 
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override
    {
        os << "\t";
    }
private:
    std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char* file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                   const std::string& thread_name)
    : m_filename(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(thread_id)
    , m_threadName(thread_name)
    , m_fiberId(fiber_id)
    , m_time(time)
    , m_logger(logger)
    , m_level(level)
{}

//格式化写入日志内容
void LogEvent::format(const char* fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

//格式化写入日志内容
void LogEvent::format(const char* fmt, va_list al)
{
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1)
    {
        m_ss << std::string(buf, len);
        free(buf);
    }
}


/*=============Logger==============*/
Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::DEBUG)
{
    //%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
   m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")); 
}
void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    MutexType::Lock lock(m_mutex);
    if(level >= m_level)
    {
        //返回Logger对象的智能指针
        auto self = shared_from_this();
        //遍历日志输出地集合log
        if(!m_appenders.empty())
        {
            for(auto& i : m_appenders)
            {
                i->log(self, level, event);
            } 
        } else if (m_root){ //如果为空， 使用m_root输出日志
            m_root->log(level, event);
        }
    }
}


//设置日志器
void Logger::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lock(m_mutex);
    m_formatter = val;

    for(auto& i : m_appenders){
        MutexType::Lock _lock(i->m_mutex);
        if(!i->m_hasFormatter){
            i->m_formatter = m_formatter;
        }
    }
}
//获取日志格式器
LogFormatter::ptr Logger::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::setFormatter(const std::string& val)
{
    //std::cout << "---" << val << std::endl;
    muhui::LogFormatter::ptr new_val(new muhui::LogFormatter(val));
    if(new_val->isError()){
        std::cout<< "Logger setFormatter name = " << m_name
            << " value = " << val << "invalid formatter" 
            << std::endl;
        return;
    }
    setFormatter(new_val);
}
std::string Logger::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKONW){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto & i : m_appenders)
    {
        //std::cout << "m_appenders = " << m_appenders.size() << std::endl;
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();

}
void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}
/*===========Appender============*/
//添加删除日志输出地
void Logger::addAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter())
    {
        MutexType::Lock _lock(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}
void Logger::delAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin(); it != m_appenders.end(); it++)
    {
        if(*it == appender)
        {
            m_appenders.erase(it);
            break;
        }
    }
}
//清空日志输出地
void Logger::clearAppender()
{
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

LogFormatter::ptr LogAppender::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}
void LogAppender::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lock(m_mutex);
    m_formatter = val; 
    if(m_formatter){
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}
FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename)
{
    reopen();
}
void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) 
{
    if(level >= m_level) {
    uint64_t now = event->getTime();
    if(now >= (m_lastTime + 3)) {
        reopen();
        m_lastTime = now;
    }
    MutexType::Lock lock(m_mutex);
    if(!m_formatter->format(m_filestream, logger, level, event)) {
        std::cout << "error" << std::endl;
        }
    }
}
std::string FileLogAppender::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKONW){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern(); 
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if(m_filestream)
    {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);
        return !!m_filestream;
}
void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) 
{
    if(level >= m_level)
    {
        MutexType::Lock lock(m_mutex);
        //std::string str = m_formatter->format(logger, level, event);
        m_formatter->format(std::cout, logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    //std::cout << "toYamlString StdoutLogAppender" << std::endl;
    if(m_level != LogLevel::UNKONW){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern(); 
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}
/*=============LogFormatter==============*/
LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern)
{
    init();
}
/* %m   输出代码中指定的消息
*  %p   输出优先级，即DEBUG，INFO，WARN，ERROR，FATAL 
*  %r   输出自应用启动到输出该log信息耗费的毫秒数 
*  %c   输出所属的类目，通常就是所在类的全名 
*  %t   输出产生该日志事件的线程名 
*  %n   输出一个回车换行符，Windows平台为“\\r\\n”，Unix平台为“\\n” 
*  %d   输出日志时间点的日期或时间,默认格式为ISO8601,也可以在其后指定格式，比如：%d{yyy MMM dd HH:mm:ss,SSS}，输出类似：2018年6月15日  22 ： 10 ： 28 ， 921  
*  %l   输出日志事件的发生位置，包括类目名、发生的线程，以及在代码中的行数。
*/
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    //stringstream是C++提供的串流（stream）物件,作用就是从string对象读取字符或字符串
    std::stringstream ss;
    for(auto& i : m_items)
    {
        i->format(ss, logger, level, event);
    }
    //str()清空流的内存缓冲，重复使用内存消耗不再增加
    return ss.str();
}
std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    for(auto& i : m_items)
    {
        i->format(ofs, logger, level, event);
    }
    return ofs;
}
/**
 * 函数功能：对字符串 "%d{ABSOLUTE} %5p %c{1}:%L - %m%n"
 * 进行解析，将模式转换的参数与格式以及类型存入到一个tuple中。
 * type = -1 代表解析失败
 * type = 0  代表只是一个字符串用format来存储，不涉及模式转换
 * type = 1  代表模式转换解析成功
 *
 **/
void LogFormatter::init()
{
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    //"%d{ABSOLUTE}
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        if(m_pattern[i] != '%') {
            //在nstr尾部拼接一个字符
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()) {
            //fmt_statue == 0 m_pattern[n]不为英文字母 
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                //从i+1位置开始，复制n-i-1个字符
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            //str = m_pattern.substr(i + 1, n - i - 1);
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }         
        /*else if(fmt_status == 2){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }*/

    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::unordered_map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
        //{"m", [](const std::string& fmt){ return FormatItem::ptr(new MessageFormatItem(fmt)); }}
        //父类指针调用子类构造
#define XX(str, C) \
        {#str, [](const std::string& fmt){ return FormatItem::ptr(new C(fmt)); }}

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem),
        XX(N, ThreadNameFormatItem),

#undef XX
    };
    //get<0>-->str, get<1>-->fmt get<2>-->type
    //std::vector<std::tuple<std::string, std::string, int> > vec;
    //type = -1 代表解析失败
    //type = 0  代表只是一个字符串用format来存储，不涉及模式转换
    //type = 1  代表模式转换解析成功
    for(auto& i : vec)
    {
        if(std::get<2>(i) == 0) //直接存储字符串
        {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    
        }
        else{
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end())
            {
                //type = -1 解析失败
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            }
            else
            {
                //存储解析成功后的字符串
                //m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i)))); 
                m_items.push_back(it->second(std::get<1>(i))); 
            }
            }
            //std::cout << "str:" << std::get<0>(i) << " - " << "fmt:" << std::get<1>(i) << " - " << "type:" << std::get<2>(i) << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}
/***********LoggerManager************/
LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    //初始化主日志器
    m_loggers[m_root->m_name] = m_root;

    init();
    //std::cout << "**" << m_root->m_name << std::endl;
}
Logger::ptr LoggerManager::getLogger(const std::string& name)
{
    MutexType::Lock lock(m_mutex);
    //保证在未创建新logger的时候将日志写道m_root中
    //当新创建logger后，保证可以将日志写道新创建的logger中
    auto it = m_loggers.find(name);
    if(it != m_loggers.end())
    {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    //将默认配置赋值给新创建的logger
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers)
    {
        //std::cout << "name = " << i.first << std::endl;
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

//日志appender配置格式
struct LogAppenderDefine
{
    //1 File, 2 Stdout
    int type = 0;
    LogLevel::Level level = LogLevel::UNKONW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const
    {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }

};
//日志配置格式
struct LogDefine
{
    std::string name;
    LogLevel::Level level = LogLevel::UNKONW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const
    {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const
    {
        return name < oth.name;
    }
};

#if 1
template<>
class LexcalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if(n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }

        if(n["appenders"].IsDefined()) {
            //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for(size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexcalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKONW) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if(!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }

        for(auto& a : i.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if(a.level != LogLevel::UNKONW) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if(!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};
#endif
muhui::ConfigVar<std::set<LogDefine>>::ptr g_log_defines = 
    muhui::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                    const std::set<LogDefine>& new_value){
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                muhui::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增logger
                    logger = MUHUI_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = MUHUI_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                /*std::cout << "** " << i.name << " level=" << i.level
                    << "  " << logger << std::endl;*/
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppender();
                for(auto& a : i.appenders) {
                    muhui::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2){ 
                            ap.reset(new StdoutLogAppender);
                    } else {
                        continue;
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = MUHUI_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppender();
                }
            }
        });
    }
};
//全局变量，在main函数之前执行
static LogIniter __log_init;

void LoggerManager::init()
{

}
}//namespace muhui


