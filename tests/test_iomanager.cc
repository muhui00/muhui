/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_iomanager.cc
 * Author      : muhui
 * Created date: 2022-11-24 22:27:00
 * Description : IOManager单元测试
 *
 *******************************************/

#define LOG_TAG "TEST_IOMANAGER"
#include "muhui.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "hook.h"

muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

int sock = 0;

void test_fiber() {
    MUHUI_LOG_INFO(g_logger) << "test_fiber sock=" << sock;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "220.181.38.150", &addr.sin_addr.s_addr);
    
    if(!(connect_f(sock, (const sockaddr*)&addr, sizeof(addr)))) {
    } else if (errno == EINPROGRESS) {
        //当我们以非阻塞的方式来进行连接的时候，返回的结果如果是 -1,
        //这并不代表这次连接发生了错误，如果它的返回结果是 EINPROGRESS，那么就代表连接还在进行中
        MUHUI_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);

        muhui::IOManager::GetThis()->addEvent(sock, muhui::IOManager::READ, [](){
            MUHUI_LOG_INFO(g_logger) << "read callback";
        });

        muhui::IOManager::GetThis()->addEvent(sock, muhui::IOManager::WRITE, [](){
            MUHUI_LOG_INFO(g_logger) << "write callback";
            muhui::IOManager::GetThis()->cancelEvent(sock, muhui::IOManager::READ);
            MUHUI_LOG_INFO(g_logger) << "cancelEvent";
            close(sock);
        });
    } else {
        MUHUI_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
    
}

void test1() {

    muhui::IOManager iom(2, false);
    //muhui::IOManager iom;
    iom.schedule(&test_fiber);
}

muhui::Timer::ptr s_timer;

void test2() {
    muhui::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        MUHUI_LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 3) {
            //s_timer->cancel();
            s_timer->reset(2000, true);
        }
    }, true); 
}
int main(int argc, char *argv[]) {
    test1();
    //test2();
    return 0;
}

