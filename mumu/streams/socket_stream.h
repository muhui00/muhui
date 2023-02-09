/**
 * @file stream_socket.h
 * @author muhui (2571579302@qq.com)
 * @brief Socket流式接口封装
 * @version 0.1
 * @date 2023-02-05
 */

#ifndef __MUHUI_STREAM_SOCKET_H__
#define __MUHUI_STREAM_SOCKET_H__
#include "address.h"
#include "stream.h"
#include "socket.h"
#include "mutex.h"
#include "iomanager.h"
#include <memory>

namespace muhui {
/**
 * @brief Socket流
 */
class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;

    /**
     * @brief Construct a new Socket Stream object
     * 
     * @param[in] sock Socket
     * @param[in] owner 是否完全控制
     */
    SocketStream(Socket::ptr sock, bool owner = true);

    /**
     * @brief Destroy the Socket Stream object
     * @pre 如果m_owner == true, close
     */
    ~SocketStream();

    /**
     * @brief 读取数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 待接收数据的长度
     * @return int 
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int read(void *buffer, size_t length) override;
    /**
     * @brief 读取数据
     * @param[out] ba 接收数据的ByteArray 
     * @param length 待接收数据的长度
     * @return int 
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int read(ByteArray::ptr ba, size_t length) override;

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的内存
     * @param length 待发送数据的长度
     * @return int 
     *      @retval >0 返回实际发送的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int write(const void *buffer, size_t length) override;

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的ByteArray
     * @param length 待发送数据的长度
     * @return int 
     *      @retval >0 返回实际发送的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    virtual int write(ByteArray::ptr ba, size_t length) override;

    /**
     * @brief 关闭Socket
     */
    virtual void close() override;

    Socket::ptr getSocket() const {return m_sock;}

    /**
     * @brief 返回是否连接
     * @return true 
     * @return false 
     */
    bool isConnected() const;

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();
    std::string getRemoteAddressString();
    std::string getLocalAddressString();
protected:
    /// Socket 句柄
    Socket::ptr m_sock;
    /// 是否主控
    bool m_owner;
};
} // namespace muhui
#endif // !__MUHUI_STREAM_SOCKET_H__