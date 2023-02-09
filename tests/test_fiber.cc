/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_fiber.cc
 * Author      : muhui
 * Created date: 2022-11-14 07:09:00
 * Description : 
 *
 *******************************************/

#define LOG_TAG "TEST_FIBER"
//#include "muhui.h"
#include "log.h"
#include "fiber.h"
#include "thread.h"

muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

void run_in_fiber()
{
    MUHUI_LOG_INFO(g_logger) << "run_in_fiber begin";
    //将该协程置为后台
    muhui::Fiber::YieldToHold();
    MUHUI_LOG_INFO(g_logger) << "run_in_fiber end";
    muhui::Fiber::YieldToHold();
}

void test_fiber()
{
    MUHUI_LOG_INFO(g_logger) << "main begin -1";
    {
        muhui::Fiber::GetThis();
        MUHUI_LOG_INFO(g_logger) << "main begin";
        //创建协程（主协程默认构造创建）
        muhui::Fiber::ptr fiber(new muhui::Fiber(run_in_fiber));
        //将当前协程置为运行状态
        fiber->swapIn();
        MUHUI_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        MUHUI_LOG_INFO(g_logger) << "main end";
        fiber->swapIn();
    }
    MUHUI_LOG_INFO(g_logger) << "main after end 2";
}
int main(int argc, char *argv[]) 
{
    muhui::Thread::SetName("main");
    std::vector<muhui::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(muhui::Thread::ptr(new muhui::Thread(&test_fiber, "thread_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}

