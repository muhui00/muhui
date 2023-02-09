#include "socket.h"
#include "iomanager.h"
#include "log.h"
static muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

void test_sock() {
    muhui::IPAddress::ptr addr = muhui::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        MUHUI_LOG_INFO(g_logger) << "get address: " << addr->toString();        
    } else {
        MUHUI_LOG_ERROR(g_logger) << "get address filed";
        return;
    }

    muhui::Socket::ptr sock = muhui::Socket::CreateTCP(addr);
    addr->setPort(80);
    MUHUI_LOG_INFO(g_logger) << "addr=" << addr->toString();

    //连接
    if(!sock->connect(addr)) {
        MUHUI_LOG_ERROR(g_logger) << "connect " << addr->toString() << "fail";
        return;
    } else {
        MUHUI_LOG_INFO(g_logger) << "connect " << addr->toString() << "succ";
    }

    //发送请求数据
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        MUHUI_LOG_ERROR(g_logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if(rt <= 0) {
        MUHUI_LOG_ERROR(g_logger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    MUHUI_LOG_INFO(g_logger) << buffs;
}

int main(int argc, char** argv) {
    muhui::IOManager iom;
    iom.schedule(&test_sock);
    return 0;
}
