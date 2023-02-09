/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : fd_manager.cpp
 * Author      : muhui
 * Created date: 2022-12-04 22:51:00
 * Description : 文件句柄管理类实现
 *
 *******************************************/

#define LOG_TAG "FD_MANAGER"
#include "fd_manager.h"
#include "hook.h"

#include <sys/stat.h>
#include <sys/socket.h>

namespace muhui{
  
FdCtx::FdCtx(int fd)
    : m_fd(fd)
    , m_isInit(false)
    , m_isSocket(false)
    , m_sysNonblock(false)
    , m_userNonblock(false)
    , m_isClosed(false)
    , m_recvTimeout(-1)
    , m_sendTimeout(-1)
{
    init();
}

FdCtx::~FdCtx()
{

}

bool FdCtx::init() 
{
    if(m_isInit) {
        return true;
    }

    m_recvTimeout = -1;
    m_sendTimeout = -1;

    // fdsate返回文件句柄类型
    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)) {
        //错误
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        //宏 是否是socket
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    if(m_isSocket) {
        //F_GETFD 获取文件句柄flag(状态)
        int flag = fcntl_f(m_fd, F_GETFL, 0);
        //设置非阻塞
        if(!(flag & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flag | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v)
{
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}
uint64_t FdCtx::getTimeout(int type)
{
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager()
{
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_datas.size() < fd) {
        if(auto_create == false) {
            return nullptr;
        } 
    } else {
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();

    RWMutex::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd)
{
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() < fd) {
        return;
    }
    m_datas[fd].reset();
}

}//muhui

