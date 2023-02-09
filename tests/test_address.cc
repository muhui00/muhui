#include "mumu/address.h"
#include "mumu/log.h"
#include <map>
#include <sys/socket.h>

muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

void test() {
    std::vector<muhui::Address::ptr> addrs;
    bool v = muhui::Address::Lookup(addrs, "www.baidu.com:ftp");
    if(!v) {
        MUHUI_LOG_ERROR(g_logger) << "Address::Lookup filed";
    }    

    for (size_t i = 0; i < addrs.size(); i++)
    {   
        MUHUI_LOG_DEBUG(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<muhui::Address::ptr, uint32_t>> results;
    bool v = muhui::Address::GetInterfaceAddresses(results, AF_INET6);
    if(!v) {
        MUHUI_LOG_ERROR(g_logger) << "GetInterfaceAddresses filed";
        return;
    }

    for(auto &i : results) {
        MUHUI_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
    }
}

void test_ipv4() {
    auto addr = muhui::IPAddress::Create("127.0.0.8");
    if(addr) {
        MUHUI_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    //test_iface();
    test_ipv4();    
    return 0;
}