/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_hook.cc
 * Author      : muhui
 * Created date: 2022-11-29 22:13:28
 * Description : hook unit test
 *
 *******************************************/

#define LOG_TAG "TEST_HOOK"
#include "hook.h"
#include "muhui.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

void test_sleep() {
    muhui::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        MUHUI_LOG_INFO(g_logger) << "sleep 2";
    });
    iom.schedule([](){
        sleep(3);
        MUHUI_LOG_INFO(g_logger) << "sleep 3";
    });
    MUHUI_LOG_INFO(g_logger) << "sleep test";
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "220.181.38.150", &addr.sin_addr.s_addr);

    MUHUI_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    MUHUI_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    MUHUI_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt < 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    MUHUI_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    buff.resize(rt);
    MUHUI_LOG_INFO(g_logger) << buff;
}
int main(int argc, char *argv[]) {
    //test_sleep();
    //test_sock();
    muhui::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}

