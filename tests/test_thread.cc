/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_thread.cc
 * Author      : muhui
 * Created date: 2022-11-11 15:40:24
 * Description : 
 *
 *******************************************/

#define LOG_TAG "TEST_THREAD"

#include "muhui.h"
#include <unistd.h>
muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();
int count = 0;
//muhui::RWMutex s_mutex;
muhui::Mutex s_mutex;

void fun1() {
    MUHUI_LOG_INFO(g_logger) << "name:" << muhui::Thread::GetName()
        << " this.name:" << muhui::Thread::GetThis()->getName()
        << " id:" << muhui::GetThreadId()
        << " this.id:" << muhui::Thread::GetThis()->getId();
    for(int i = 0; i < 100; ++i) {
        //模板实例化，调用构造函数，加锁
        //该线程开始执行时加锁，WriteLock 初始化构造
        //该线程结束时， WriteLock 对象销毁，执行析构，解锁
        //muhui::RWMutex::WriteLock lock(s_mutex);
        muhui::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        MUHUI_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        MUHUI_LOG_INFO(g_logger) << "===========================================";
    }
}

int main(int argc, char** argv)
{
    MUHUI_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/muhui/mumu/bin/conf/test2.yml");
    muhui::Config::LoadFromYaml(root);
#if 1
    std::vector<muhui::Thread::ptr> thrs;

    for(int i = 0; i < 2; ++i){
        muhui::Thread::ptr thr1(new muhui::Thread(&fun2, "name_" + std::to_string(i * 2)));
        muhui::Thread::ptr thr2(new muhui::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr1);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i){
        thrs[i]->join();
    }
#endif
    MUHUI_LOG_INFO(g_logger) << " count = " << count;
    MUHUI_LOG_INFO(g_logger) << "thread test end";
    return 0;
}
