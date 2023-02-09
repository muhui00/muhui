/**
 * @file http_server.h
 * @author muhui (2571579302@qq.com)
 * @brief HTTP服务器封装
 * @version 0.1
 * @date 2023-02-06
 */
#ifndef __MUHUI_HTTP_HTTP_SERVER_H
#define __MUHUI_HTTP_HTTP_SERVER_H
#include "iomanager.h"
#include "tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace muhui {
namespace http {

/**
 * @brief HttpServer 服务器类
 */
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief Construct a new Http Server object
     * 
     * @param[in] isKeepalive 是否长连接
     * @param[in] worker 
     * @param[in] io_worker 
     * @param[in] acceptWorker 
     */
    HttpServer(bool isKeepalive = false
            , IOManager*  worker = IOManager::GetThis()
            , IOManager* io_worker = IOManager::GetThis()
            , IOManager* acceptWorker = IOManager::GetThis());
    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}
    void setServletDispatch(ServletDispatch::ptr v) {m_dispatch = v;}
    virtual void setName(const std::string &v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// servlet分发器
    ServletDispatch::ptr m_dispatch;
};
} // namespace http
} // namespace muhui
#endif
