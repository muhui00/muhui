#include "tcp_server.h"
#include "bytearray.h"
#include "iomanager.h"
#include "log.h"
#include "address.h"

static muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

/**
 * @brief TCP server
 * 
 */
class EchoServer : public muhui::TcpServer {

public:
    EchoServer(int type);
    void handleClient(muhui::Socket::ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handleClient(muhui::Socket::ptr client) {
    MUHUI_LOG_INFO(g_logger) << "handleCilent: cilent = " << *client;
    muhui::ByteArray::ptr ba(new muhui::ByteArray);
    while(true) {
        std::vector<iovec> iov;
        ba->clear();
        ba->getWriteBuffers(iov, 1024);
        int rt = client->recv(&iov[0], iov.size());
        if(rt == 0) {
            MUHUI_LOG_INFO(g_logger) << "client close:" << *client;
            break;
        } else if(rt < 0) {
            MUHUI_LOG_ERROR(g_logger) << "client error rt=" << rt
                << ", errno=" << errno <<", errstr=" << strerror(errno);
            break;
        }
        //MUHUI_LOG_DEBUG(g_logger) << "rt=" << rt;
        //MUHUI_LOG_DEBUG(g_logger) << "set_position = " << ba->getPosition() + rt;
        //设置可读数据大小
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1) {
            std::cout << ba->toString();
        } else {
            std::cout << ba->toHexString();
        }
        std::cout.flush();
    }
}
int type = 2;
void run() {
    MUHUI_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = muhui::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv) {
#if 0
    if(argc < 2) {
        MUHUI_LOG_INFO(g_logger) << "used as[ " << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    if(!strcmp(argv[0], "-b")) {
        type = 2;
    }
#endif
    muhui::IOManager iom(2);
    iom.schedule(run);
    return 0;
}