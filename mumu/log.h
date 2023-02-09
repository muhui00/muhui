/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : log.h
 * Author      : muhui
 * Created date: 2022-11-03 19:48:58
 * Description : 日志模块
 *
 *******************************************/

#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <map>
#include <unordered_map>

#include "singleton.h"
#include "thread.h"
#include "util.h"
/**
 *使用流的方式将日志级别写入到日志器
 */
#define MUHUI_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        muhui::LogEventWrap(muhui::LogEvent::ptr(new muhui::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, muhui::GetThreadId(), \
                    muhui::GetFiberId(), time(0), muhui::Thread::GetName()))).getss()

#define MUHUI_LOG_DEBUG(logger) MUHUI_LOG_LEVEL(logger, muhui::LogLevel::DEBUG)
#define MUHUI_LOG_INFO(logger) MUHUI_LOG_LEVEL(logger, muhui::LogLevel::INFO)
#define MUHUI_LOG_WARN(logger) MUHUI_LOG_LEVEL(logger, muhui::LogLevel::WARN)
#define MUHUI_LOG_ERROR(logger) MUHUI_LOG_LEVEL(logger, muhui::LogLevel::ERROR)
#define MUHUI_LOG_FATAL(logger) MUHUI_LOG_LEVEL(logger, muhui::LogLevel::FATAL)

#define MUHUI_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        muhui::LogEventWrap(muhui::LogEvent::ptr(new muhui::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, muhui::GetThreadId(), \
                    muhui::GetFiberId(), time(0), muhui::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define MUHUI_LOG_FMT_DEBUG(logger, fmt, ...) MUHUI_LOG_FMT_LEVEL(logger, muhui::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define MUHUI_LOG_FMT_INFO(logger, fmt, ...) MUHUI_LOG_FMT_LEVEL(logger, muhui::LogLevel::INFO, fmt, __VA_ARGS__)
#define MUHUI_LOG_FMT_WARN(logger, fmt, ...) MUHUI_LOG_FMT_LEVEL(logger, muhui::LogLevel::WARN, fmt, __VA_ARGS__)
#define MUHUI_LOG_FMT_ERROR(logger, fmt, ...) MUHUI_LOG_FMT_LEVEL(logger, muhui::LogLevel::ERROR, fmt, __VA_ARGS__)
#define MUHUI_LOG_FMT_FATAL(logger, fmt, ...) MUHUI_LOG_FMT_LEVEL(logger, muhui::LogLevel::FATAL, fmt, __VA_ARGS__)

#define MUHUI_LOG_ROOT() muhui::LoggerMgr::GetInstance()->getRoot()
#define MUHUI_LOG_NAME(name) muhui::LoggerMgr::GetInstance()->getLogger(name)
namespace muhui
{
class Logger;
class LoggerManager;
//日志级别
class LogLevel
{
public:
    enum Level{
        UNKONW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    //将日志级别转成文本输出
    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};


//日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, 
             const char* file, int32_t line, uint32_t elapse,
            uint32_t thread_id, uint32_t fiber_id, uint64_t time,
            const std::string& thread_name);

    const char* getFilename() const { return m_filename; }
    
    int32_t getLine() const { return m_line; }

    int32_t getElapse() const { return m_elapse; }
    
    int32_t getThreadId() const { return m_threadId; }

    std::string getThreadName() { return m_threadName; }

    int32_t getFiberId() const { return m_fiberId; }

    uint64_t getTime() const { return m_time; }

    std::string getContent() const { return m_ss.str(); }

    std::stringstream& getSS() { return m_ss; }

    std::shared_ptr<Logger> getLogger() const { return m_logger; }

    LogLevel::Level getLevel() const { return m_level; }

    //格式化写入日志内容
    void format(const char* fmt, ...);

    //格式化写入日志内容
    void format(const char* fmt, va_list al);
private:
    const char* m_filename = nullptr; //文件名
    int32_t m_line = 0;           //行号
    int32_t m_elapse = 0;         //程序启动开始到现在的毫秒数
    int32_t m_threadId = 0;       //线程id
    std::string m_threadName;    //线程名
    int32_t m_fiberId = 0;        //协程id
    int64_t m_time = 0;           //时间戳
    std::stringstream m_ss;       //日志内容流
    std::shared_ptr<Logger> m_logger;  //日志器
    LogLevel::Level m_level;

};
//日志包装器
class LogEventWrap
{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event; }
    std::stringstream& getss();
private:
    LogEvent::ptr m_event;
};
//日志格式器
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    void init();
    const std::string getPattern() const { return m_pattern; }
public:
    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        //防止对子类的析构
        virtual ~FormatItem(){}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    bool isError() { return m_error; }

private:
    //日志格式模板
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;    
    bool m_error = false;
};

//日志输出地
class LogAppender
{
friend class Logger;
public:
    typedef Spinlock MutexType;
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {};
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; } 
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
    MutexType m_mutex;
    //是否有自己的日志格式器
    bool m_hasFormatter = false;
};
//日志器
class Logger : public std::enable_shared_from_this<Logger>
{
friend class LoggerManager;
public:
    typedef Spinlock MutexType;
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string& name = "root");
    /**
    * @brief 写日志
    * @param[in] level 日志级别
    * @param[in] event 日志事件
    */
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);                   
    void info(LogEvent::ptr event);                   
    void warn(LogEvent::ptr event);                   
    void error(LogEvent::ptr event);                   
    void fatal(LogEvent::ptr event);                   

    //添加删除日志输出地
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    //清空日志输出地
    void clearAppender();
    //获取日志级别
    LogLevel::Level getLevel() const {return m_level;}
    //设置日志级别
    void setLevel(LogLevel::Level level) {m_level = level;}

    //获取日志名称
    const std::string& getName() const { return m_name; }

    //设置日志器
    void setFormatter(LogFormatter::ptr val);

    //设置日志格式模板
    void setFormatter(const std::string& val);

    //获取日志格式器
    LogFormatter::ptr getFormatter();

    std::string toYamlString();
private:
    std::string m_name;                      //日志名称
    LogLevel::Level m_level;                 //日记级别
    std::list<LogAppender::ptr> m_appenders; //Append集合
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
    MutexType m_mutex;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

//定义输出到文件的Appender
class FileLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
    //重新打开文件
    bool reopen();
private:
    std::string m_filename; //文件名
    std::ofstream m_filestream; //文件输出流
    uint64_t m_lastTime = 0;
};

//日志管理
class LoggerManager
{
public:
    typedef Spinlock MutexType;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);
    void init();
    Logger::ptr getRoot() const { return m_root; }
    //将所有的日志器配置转成YAML String
    std::string toYamlString();
private:
    //日志器集合， key-->name 保证唯一, value-->logger::ptr
    std::map<std::string, Logger::ptr> m_loggers;
    MutexType m_mutex;
    Logger::ptr m_root;
};

typedef muhui::Singleton<LoggerManager> LoggerMgr;

}

#endif //__LOG_H__

