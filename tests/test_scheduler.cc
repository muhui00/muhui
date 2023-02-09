/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_scheduler.cc
 * Author      : muhui
 * Created date: 2022-11-21 10:16:36
 * Description : Scheduler 测试函数
 *
 *******************************************/

#define LOG_TAG "TEST_SCHEDULER"
#include "muhui.h"
#include "hook.h"
static muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

void test_fiber() {
    static int count = 5;
    MUHUI_LOG_INFO(g_logger) << "test in fiber, count=" << count;
    sleep_f(1);
    if(--count >= 0) {
        muhui::Scheduler::GetThis()->schedule(&test_fiber, muhui::GetThreadId());
    }
}

int main(int argc, char *argv[]) {
    MUHUI_LOG_INFO(g_logger) << "main";
    muhui::Scheduler sc(3, false, "muhui");
    sc.start();
    sleep_f(2);
    MUHUI_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    MUHUI_LOG_INFO(g_logger) << "over";
    return 0;
}

