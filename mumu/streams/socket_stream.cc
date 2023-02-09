#include "socket_stream.h"
#include <vector>

namespace muhui {

/**
    * @brief Construct a new Socket Stream object
    * 
    * @param[in] sock Socket
    * @param[in] owner 是否完全控制
    */
SocketStream::SocketStream(Socket::ptr sock, bool owner) 
    : m_sock(sock)
    , m_owner(owner) {}

/**
    * @brief Destroy the Socket Stream object
    * @pre 如果m_owner == true, close
    */
SocketStream::~SocketStream() {
    if(m_owner && m_sock) {
        m_sock->close();
    }
}

/**
    * @brief 读取数据
    * @param[out] buffer 接收数据的内存
    * @param[in] length 待接收数据的长度
    * @return int 
    *      @retval >0 返回实际接收到的数据长度
    *      @retval =0 socket被远端关闭
    *      @retval <0 socket错误
    */
int SocketStream::read(void *buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_sock->recv(buffer, length);
}
/**
    * @brief 读取数据
    * @param[out] ba 接收数据的ByteArray 
    * @param length 待接收数据的长度
    * @return int 
    *      @retval >0 返回实际接收到的数据长度
    *      @retval =0 socket被远端关闭
    *      @retval <0 socket错误
    */
int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, iovs.size());
    int rt = m_sock->recv(&iovs[0], iovs.size());
    if(rt > 0) {
        // 设置读取数据大小
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

/**
    * @brief 发送数据
    * @param[in] buffer 待发送数据的内存
    * @param length 待发送数据的长度
    * @return int 
    *      @retval >0 返回实际发送的数据长度
    *      @retval =0 socket被远端关闭
    *      @retval <0 socket错误
    */
int SocketStream::write(const void *buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_sock->send(buffer, length);
}

/**
    * @brief 发送数据
    * @param[in] buffer 待发送数据的ByteArray
    * @param length 待发送数据的长度
    * @return int 
    *      @retval >0 返回实际发送的数据长度
    *      @retval =0 socket被远端关闭
    *      @retval <0 socket错误
    */
int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = m_sock->send(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;

}

/**
    * @brief 关闭Socket
    */
void SocketStream::close() {
    if(m_sock) {
        m_sock->close();
    }
}

/**
    * @brief 返回是否连接
    * @return true 
    * @return false 
    */
bool SocketStream::isConnected() const {
    return m_sock && m_sock->isConnected();
}

Address::ptr SocketStream::getRemoteAddress() {
    if(m_sock) {
        return m_sock->getRemoteAddress();
    }
    return nullptr;
}
Address::ptr SocketStream::getLocalAddress() {
    if(m_sock) {
        return m_sock->getLocalAddress();
    }
    return nullptr;
}
std::string SocketStream::getRemoteAddressString() {
    if(m_sock) {
        return m_sock->getRemoteAddress()->toString();
    }
    return "";
}
std::string SocketStream::getLocalAddressString() {
    if(m_sock) {
        return m_sock->getLocalAddress()->toString();
    }
    return "";
}

} // namespace muhui