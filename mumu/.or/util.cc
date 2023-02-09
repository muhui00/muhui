/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : util.cc
 * Author      : muhui
 * Created date: 2022-11-04 21:40:00
 * Description : 工具函数实现
 *
 *******************************************/
#include <unistd.h>
#include <sys/syscall.h>
#include <execinfo.h>
#include <sstream>
#include <sys/time.h>

#include "util.h"
#include "log.h"
#include "fiber.h"
namespace  muhui
{

static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

pid_t GetThreadId()
{
    return syscall(SYS_gettid);
}

uint32_t GetFiberId()
{
    return Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip)
{
    //为指针数组array开辟空间（一次性开辟）
    //void* 是从堆栈中获取的返回地址
    void** array = (void**)malloc((sizeof(void*) * size));
    
    //array 为传入传出参数
    size_t s = ::backtrace(array, size);

    //该函数的参数为backtrace的返回值s及传出参数array
    //s 为实际返回的void*个数
    //函数返回值是一个指向字符串数组的指针，它包含char*元素个数为size。
    //每个字符串包含了一个相对于buffer中对应元素的可打印信息，包括函数名、函数偏移地址和实际返回地址。
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        MUHUI_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }
    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }
    
    free(array);
    free(strings);

}

std::string BacktraceToString(int size, int skip, const std::string& prefix)
{
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++ i) {
        ss << prefix << bt[i] << std::endl;
    }

    return ss.str();
}

// 获取当前时间毫秒
uint64_t GetCurrentMS()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

//获取当前时间微秒
uint64_t GetCurrentUS()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}


} //muhui
