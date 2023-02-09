#include "http_server.h"
#include "http/http.h"
#include "http/servlet.h"
#include "http_session.h"
#include "log.h"
#include "tcp_server.h"
#include <cstring>
#include <memory>

namespace muhui {
namespace http {
static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

HttpServer::HttpServer(bool isKeepalive
                    , IOManager*  worker
                    , IOManager* io_worker
                    , IOManager* acceptWorker)
    : TcpServer(worker, io_worker, acceptWorker)
    , m_isKeepalive(isKeepalive) {
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::setName(const std::string &v) {
    TcpServer::setName(v);
    m_dispatch->SetDefault(std::make_shared<NotFountServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client) {
    MUHUI_LOG_DEBUG(g_logger) << "HttpServer handleClient: " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        //获取HTTP请求
        auto req = session->recvRequest();
        if(!req) {
            MUHUI_LOG_ERROR(g_logger) << "recv http request fail, errno="
                << errno << ", errstr=" << strerror(errno)
                << ", client:" << *client << ", keep-alive" << m_isKeepalive;
            break;
        }
        //响应请求
        HttpResponce::ptr rsp(new HttpResponce(req->getVersion()
                            , req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        //rsp->setBody("hello muhui");
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    }while (true);
    session->close();
}
}
}
