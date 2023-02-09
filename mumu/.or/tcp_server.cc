#include "address.h"
#include "socket.h"
#include "tcp_server.h"
#include "config.h"
#include "log.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <vector>

namespace muhui {
static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");
static muhui::ConfigVar<uint64_t>::ptr g_tcp_server_timeout = 
    muhui::Config::Lookup("tcp.server.timeout"
        , (uint64_t)(60 * 1000 *2), "tcp server timeout");

TcpServer::TcpServer(IOManager* worker, IOManager* io_worker, IOManager* acceptWorker) 
    : m_worker(worker)
    , m_ioWorker(io_worker)
    , m_acceptWorker(acceptWorker)
    , m_recvTimeout(g_tcp_server_timeout->getValue())
    , m_name("muhui/1.0.0")
    , m_isStop(true)
{

}
TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}
bool TcpServer::bind(Address::ptr addr, bool ssl) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}
bool TcpServer::bind(const std::vector<Address::ptr>& addrs,
            std::vector<Address::ptr> fails, bool ssl) {
    m_ssl = ssl;
    for(auto& i : addrs) {
        Socket::ptr sock = Socket::CreateTCP(i);
        if(!sock->bind(i)) {
            MUHUI_LOG_ERROR(g_logger) << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr[" << i->toString() << "]";
                fails.push_back(i); 
                continue;
        } 
        if(!sock->listen()) {
            MUHUI_LOG_ERROR(g_logger) << "listen fail errno=" 
                << errno <<" errstr=" << strerror(errno)
                << " addr[" << i->toString() << "]";
            fails.push_back(i);
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto& i : m_socks) {
        MUHUI_LOG_INFO(g_logger) << "name=" << m_name
                                << ", server bind success: " << *i;
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock) {
    while(!m_isStop) {
        //建立连接
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            // 告诉客户端有连接可用
            m_ioWorker->schedule(std::bind(&TcpServer::handleCilent, shared_from_this(), client));
        } else {
            MUHUI_LOG_ERROR(g_logger) << "accept arrno=" << errno
                                    << " strerr=" << strerror(errno);
        } 
    }
}
bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}
void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self](){  //使用当前类的成员变量及智能指针对象 
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}
void TcpServer::handleCilent(Socket::ptr client) {
    MUHUI_LOG_INFO(g_logger) << "hanleclient: " << *client;
}

std::string TcpServer::toString(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name << " ssl=" << m_ssl
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for(auto& i : m_socks) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}
} //muhui