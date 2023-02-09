#include <iostream>
#include "log.h"
#include "util.h"

int main()
{
    muhui::Logger::ptr logger(new muhui::Logger);
    logger->addAppender(muhui::LogAppender::ptr(new muhui::StdoutLogAppender));
    //muhui::LogEvent::ptr event(new muhui::LogEvent(__FILE__, __LINE__, 0, muhui::GetThreadId(), muhui::GetFiberId(), time(0)));
    //logger->log(muhui::LogLevel::DEBUG, event);
    
    muhui::FileLogAppender::ptr file_appender(new muhui::FileLogAppender("./log.txt"));
    muhui::LogFormatter::ptr fmt(new muhui::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(muhui::LogLevel::INFO);
    logger->addAppender(file_appender);

    auto l = muhui::LoggerMgr::GetInstance()->getLogger("xx");
    auto ll = muhui::LoggerMgr::GetInstance()->getRoot();
    for(int i = 0; i < 1000; ++i){
    MUHUI_LOG_INFO(l) << "test info";
    }
    //std::cout << "thread:" << muhui::GetThreadId()<< std::endl; 
    //std::cout << "fiber:" << muhui::GetFiberId()<< std::endl;
    MUHUI_LOG_FMT_DEBUG(logger, "test macro fmt debug %s", "cc");
    MUHUI_LOG_FMT_INFO(logger, "test macro fmt info %s", "bb");
    MUHUI_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    return 0;
}
