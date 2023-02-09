#include "address.h"
#include "tcp_server.h"
#include "log.h"
#include "iomanager.h"
muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();
void run(){
    auto addr = muhui::Address::LookupAny("0.0.0.0:8033");
    MUHUI_LOG_INFO(g_logger) << *addr;
    //MUHUI_LOG_INFO(g_logger) << "hello world";
    std::vector<muhui::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    muhui::TcpServer::ptr tcp_server(new muhui::TcpServer);
    std::vector<muhui::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}

int main() {
    muhui::IOManager iom(2);
    iom.schedule(run);
    return 0;
}