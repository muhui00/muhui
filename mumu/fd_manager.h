/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : fd_manager.h
 * Author      : muhui
 * Created date: 2022-12-04 22:50:55
 * Description : 文件句柄管理类
 *
 *******************************************/

#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <vector>
#include <memory>
#include "mutex.h"
#include "singleton.h"

namespace muhui {

class FdCtx : public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx> ptr;
    /**
     * @brief 通过文件句柄构造FdCtx
     */
    FdCtx(int fd);
    /**
     * @brief 析构函数
     */
    ~FdCtx();

    /**
     * @brief 是否初始化完成
     */
    bool isInit() const { return m_isInit; }

    /**
     * @brief 是否socket
     */    
    bool isSocket() const { return m_isSocket; }

    /**
     * @brief 是否已关闭
     */
    bool isClosed() const { return m_isClosed;}

    /**
     * @brief 获取系统非阻塞
     */    
    bool getSysNonblock() const { return m_sysNonblock; }

    /**
     * @brief 设置系统非阻塞
     * @param[in] v 是否阻塞
     */
    void setSysNonblock(bool v) { m_sysNonblock = v; }

    /**
     * @brief 获取是否用户主动设置的非阻塞
     */    
    bool getUserNonblock() const { return m_userNonblock; }

    /**
     * @brief 设置用户主动设置非阻塞
     * @param[in] v 是否阻塞
     */    
    void setUserNonblock(bool v) { m_userNonblock = v; }

    /**
     * @brief 设置超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @param[in] v 时间毫秒
     */
    void setTimeout(int type, uint64_t v);

    /**
     * @brief 获取超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @return 超时时间毫秒
     */
    uint64_t getTimeout(int type);

private:

    /**
     * @brief 初始化
     */
    bool init();

private:
    /// 文件句柄
    int m_fd;
    /// 是否初始化
    bool m_isInit : 1;
    /// 是否socket
    bool m_isSocket : 1;
    /// 是否hook非阻塞
    bool m_sysNonblock : 1;
    /// 是否用户主动设置非阻塞
    bool m_userNonblock : 1;
    /// 是否关闭
    bool m_isClosed : 1;
    /// 读超时时间毫秒
    uint64_t m_recvTimeout;
    /// 写超时时间毫秒
    uint64_t m_sendTimeout;
};

/**
 * @brief 文件句柄管理类
 */
class FdManager {
public:
    typedef std::shared_ptr<FdManager> ptr;
    typedef RWMutex RWMutexType;

    FdManager();

    /**
     * @brief 获取/创建文件句柄类FdCtx
     * @param[in] fd 文件句柄
     * @param[in] auto_create 是否自动创建
     * @return 返回对应文件句柄类FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /**
     * @brief 删除文件句柄类
     * @param[in] fd 文件句柄
     */
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;    
};

//文件句柄单例
typedef Singleton<FdManager> FdMgr;

}//muhui

#endif //__FD_MANAGER_H__

