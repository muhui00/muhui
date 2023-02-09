/**
 * @file http_session.h
 * @author muhui (2571579302@qq.com)
 * @brief http session类 server.accept socket -> session
 * @version 0.1
 * @date 2023-02-05
 */
#ifndef __MUHUI_HTTP_SESSION_H
#define __MUHUI_HTTP_SESSION_H
#include "http/http.h"
#include "socket.h"
#include "streams/socket_stream.h"
namespace muhui {
namespace http {
/**
 * @brief HttpSession 类 封装
 * 
 */
class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief Construct a new Http Session object
     * @param[in] socket 类
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::ptr socket, bool owner = true);

    /**
     * @brief 接收HTTP请求
     * @return HttpRequest::ptr 
     */
    HttpRequest::ptr recvRequest();
    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponce::ptr rsp);
};

} // http
} // namespace muhui
#endif // !__MUHUI_HTTP_SESSION_H