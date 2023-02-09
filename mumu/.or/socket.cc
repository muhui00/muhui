/*****************************************
 * Copyright (C) 2023 * Ltd. All rights reserved.
 *
 * File name   : socket.cpp
 * Author      : muhui
 * Created date: 2023-01-25 10:12:46
 * Description : 
 *
 *******************************************/

#include <sstream>
#define LOG_TAG "SOCKET"
#include "socket.h"
#include "fd_manager.h"
#include "hook.h"
#include "log.h"
#include "macro.h"
#include "iomanager.h"
#include <limits.h>
#include <netinet/tcp.h>

namespace muhui {

static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

/**
 * @brife 创建TCP socket(满足地址类型)
 * @param[in] address 地址
 */
Socket::ptr Socket::CreateTCP(muhui::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

/**
 * @brife 创建UDP socket(满足地址类型)
 * @param[in] address 地址
 */
Socket::ptr Socket::CreateUDP(muhui::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * @brief 创建IPv4的TCP Socket
 */
Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

/**
 * @brief 创建IPv4的UDP Socket
 */
Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * @brief 创建IPv6的TCP Socket
 */
Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

/**
 * @brief 创建IPv6的UDP Socket
 */
Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * @brief 创建Unix的TCP Socket
 */
Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

/**
 * @brief 创建Unix的UDP Socket
 */
Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

/**
 * @brief Socket构造函数
 * @param[in] family 协议簇
 * @param[in] type 类型
 * @param[in] protocol 协议
 */
Socket::Socket(int family, int type, int protocl)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocl)
    , m_isConnected(false)
{

}
Socket::~Socket() {
    close();
}

/**
 * @brief 获取发送超时时间(毫秒)
 */
int64_t Socket::getSendTimeout() {
   FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
   if(ctx) {
       return ctx->getTimeout(SO_SNDTIMEO);
   }
   return -1;
}

/**
 * @brief 设置发送超时时间(毫秒)
 */
void Socket::setSendTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/**
 * @brief 获取接受超时时间(毫秒)
 */
int64_t Socket::getRecvTimeout() {
   FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
   if(ctx) {
       return ctx->getTimeout(SO_RCVTIMEO);
   }
   return -1;
}

/**
 * @brief 设置接受超时时间(毫秒)
 */
void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

/**
 * @brief 获取sockopt @see getsockopt
 */
bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
    int rt = getsockopt(m_sock, level, option, result, len);
    if(rt) {
        MUHUI_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock 
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 设置sockopt @see setsockopt
 */
bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
    int rt= setsockopt(m_sock, level, option, result, len);
    if(rt) {
        MUHUI_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock 
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 接收connect链接
 * @return 成功返回新连接的socket,失败返回nullptr
 * @pre Socket必须 bind , listen  成功
 */
Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    MUHUI_LOG_DEBUG(g_logger) << "newsock=" << newsock; 
    if(newsock == -1) {
        MUHUI_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" 
            << errno << " strerr=" << strerror(errno);
        return nullptr;
    }
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

/**
 * @brief 绑定地址
 * @param[in] addr 地址
 * @return 是否绑定成功
 */
bool Socket::bind(const Address::ptr addr) {
    if(MUHUI_UNLIKELY(!isValid())) {
        newSock();
        if(MUHUI_UNLIKELY(!isValid())) {
            return false;
        }        
    }
    if(MUHUI_UNLIKELY(addr->getFamily() != m_family)) {
        MUHUI_LOG_ERROR(g_logger) << "bind sock.family(" << addr->getFamily()
            << ") not equal_range, addr=" << addr->toString() 
            << " errno=" << errno << " strerr=" << strerror(errno);
        return false;
    }

    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        MUHUI_LOG_ERROR(g_logger) << "bind error errno=" << errno
            << ", strerr=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

/**
 * @brief 连接地址
 * @param[in] addr 目标地址
 * @param[in] timeout_ms 超时时间(毫秒)
 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    if(MUHUI_UNLIKELY(!isValid())) {
        newSock();
        if(MUHUI_UNLIKELY(!isValid())) {
            return false;
        }        
    }
    if(MUHUI_UNLIKELY(addr->getFamily() != m_family)) {
        MUHUI_LOG_ERROR(g_logger) << "connect sock.family(" << addr->getFamily()
            << ") not equal_range, addr=" << addr->toString() 
            << " errno=" << errno << " strerr=" << strerror(errno);
        return false;
    }

    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            MUHUI_LOG_ERROR(g_logger) << "connect error errno=" << errno
                << ", strerr=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            MUHUI_LOG_ERROR(g_logger) << "connect_with_timeout error errno=" << errno
                << ", strerr=" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getLocalAddress();
    getRemoteAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms) {
    return true;
}

/**
 * @brief 监听socket
 * @param[in] backlog 未完成连接队列的最大长度
 * @result 返回监听是否成功
 * @pre 必须先 bind 成功
 */
bool Socket::listen(int backlog) {
    if(MUHUI_UNLIKELY(!isValid())) {
        MUHUI_LOG_ERROR(g_logger) << "listen error, sock=-1";
        return false;
    }
    if(::listen(m_sock, backlog)) {
        MUHUI_LOG_ERROR(g_logger) << "listen error errno=" << errno
            << ", strerr=" << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 关闭socket
 */
bool Socket::close() {
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

/**
 * @brief 发送数据
 * @param[in] buffer 待发送数据的内存
 * @param[in] length 待发送数据的长度
 * @param[in] flags 标志字
 * @return
 *      @retval >0 发送成功对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

/**
 * @brief 发送数据
 * @param[in] buffers 待发送数据的内存(iovec数组)
 * @param[in] length 待发送数据的长度(iovec长度)
 * @param[in] flags 标志字
 * @return
 *      @retval >0 发送成功对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::send(const iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief 发送数据
 * @param[in] buffer 待发送数据的内存
 * @param[in] length 待发送数据的长度
 * @param[in] to 发送的目标地址
 * @param[in] flags 标志字
 * @return
 *      @retval >0 发送成功对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

/**
 * @brief 发送数据
 * @param[in] buffers 待发送数据的内存(iovec数组)
 * @param[in] length 待发送数据的长度(iovec长度)
 * @param[in] to 发送的目标地址
 * @param[in] flags 标志字
 * @return
 *      @retval >0 发送成功对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief 接受数据
 * @param[out] buffer 接收数据的内存
 * @param[in] length 接收数据的内存大小
 * @param[in] flags 标志字
 * @return
 *      @retval >0 接收到对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

/**
 * @brief 接受数据
 * @param[out] buffers 接收数据的内存(iovec数组)
 * @param[in] length 接收数据的内存大小(iovec数组长度)
 * @param[in] flags 标志字
 * @return
 *      @retval >0 接收到对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::recv(iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief 接受数据
 * @param[out] buffer 接收数据的内存
 * @param[in] length 接收数据的内存大小
 * @param[out] from 发送端地址
 * @param[in] flags 标志字
 * @return
 *      @retval >0 接收到对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), & len);
    }
    return -1;
}

/**
 * @brief 接受数据
 * @param[out] buffers 接收数据的内存(iovec数组)
 * @param[in] length 接收数据的内存大小(iovec数组长度)
 * @param[out] from 发送端地址
 * @param[in] flags 标志字
 * @return
 *      @retval >0 接收到对应大小的数据
 *      @retval =0 socket被关闭
 *      @retval <0 socket出错
 */
int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief 获取远端地址
 */
Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }
    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnKnownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    //获取与套接字相连的对端地址
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        MUHUI_LOG_ERROR(g_logger) << "getpeername error errno=" << errno
            << ", strerr=" << strerror(errno);
        return Address::ptr(new UnKnownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return result;
}

/**
 * @brief 获取本地地址
 */
Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }
    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnKnownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    //获取与套接字端地址
    if(getsockname(m_sock, result->getAddr(), &addrlen)) {
        MUHUI_LOG_ERROR(g_logger) << "getsockname error errno=" << errno
            << ", strerr=" << strerror(errno);
        return Address::ptr(new UnKnownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return result;

}

/**
 * @brief 是否有效(m_sock != -1)
 */
bool Socket::isValid() const {
    return m_sock != -1;
}

/**
 * @brief 返回Socket错误
 */
int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return errno;
}

/**
 * @brief 输出信息到流中
 */
std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
        << " isConnected=" << m_isConnected
        << " family=" << m_family
        << " type=" << m_type
        << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

/**
 * @brief 取消读
 */
bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::Event::READ);
}

/**
 * @brief 取消写
 */
bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::Event::WRITE);
}

/**
 * @brief 取消accept
 */
bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::Event::READ);
}

/**
 * @brief 取消所有事件
 */
bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sock);
}

/**
 * @brief 初始化socket
 */
void Socket::initSock() {
    int val = 1;
    //设置套接字复用
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    //TCP
    if(m_type == SOCK_STREAM) {
        //禁止nagle算法，有需要发送的就立即发送
        setOption(IPPROTO_TCP, TCP_NODELAY, val);   
    }
}

/**
 * @brief 创建socket
 */
void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(MUHUI_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        MUHUI_LOG_ERROR(g_logger) << "socket(" << m_family 
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " strerr=" << strerror(errno);
    }
}

/**
 * @brief 初始化sock
 */
bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClosed()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}
std::ostream& operator<<(std::ostream& os, const Socket& sock) {
    return sock.dump(os);
}
}//muhui
