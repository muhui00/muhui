/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : iomanager.cc
 * Author      : muhui
 * Created date: 2022-11-24 12:25:48
 * Description : 
 *
 *******************************************/

#define LOG_TAG "IOMANAGER"
#include "iomanager.h"
#include "log.h"
#include "macro.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace muhui {

static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

/********************************************socket上下文函数实现***************************************************/
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event)
{
    switch(event) {
    case IOManager::READ:
        return read;
    case IOManager::WRITE:
        return write;
    default:
        MUHUI_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
    MUHUI_ASSERT(events & event);
    //删除事件
    events = (Event)(events & ~event);
    //触发删除事件的执行
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
}

void IOManager::contextResize(size_t size)
{
    m_fdContext.resize(size);

    for(size_t i = 0; i < size; ++i) {
        //如果 m_fdContext[i] 没有分配内存
        if(!m_fdContext[i]) {
            m_fdContext[i] = new FdContext;
            m_fdContext[i]->fd = i;
        } 
    }
}

/**********************************************公有成员函数实现*****************************************************/
IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name)  //初始化基类有参构造
{
    /**
    * @brief 创建epoll实例
    * @param[in] size> 0
    * @return 添加成功返回创建的文件句柄,失败返回-1
    */
    m_epfd = epoll_create(5000);
    MUHUI_ASSERT(m_epfd > 0);

    /**
    * @brief 创建一个管道
    * @param[in] pipefd[2] 数组
    * @param[optput] 返回两个文件句柄， pipefd[0]->读端， pipefd[1]->写端
    * @return 添加成功返回0,失败返回-1
    */
    int rt = pipe(m_tickleFds);
    MUHUI_ASSERT(!rt);
    
    epoll_event event;
    memset(&event, 0, sizeof(event));
    //设置监听读事件/边沿触发模式
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    MUHUI_ASSERT(!rt);

    /**
    * @brief 向epoll添加监听事件
    * @param[in] epfd epoll实例
    * @param[in] op 操作(EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL)
    * &param[in] fd, 待监听事件的文件句柄
    * &param[in] struct epoll_event *event, 监听事件
    * @return 添加成功返回创建的文件句柄,失败返回-1
    */
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    MUHUI_ASSERT(!rt);
    
    contextResize(32);

    //启动协程调度器
    start();
}

IOManager::~IOManager()
{
    //关闭协程调度器
    stop();
    //关闭文件句柄
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    //释放socket 上下文内存空间
    for(size_t i = 0; i < m_fdContext.size(); ++i) {
        if(m_fdContext[i]) {
            delete m_fdContext[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContext.size() > fd) {
       // 分配的内存足够
       fd_ctx = m_fdContext[fd];
       lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        // 重新分配内存
        contextResize(fd * 1.5);
        fd_ctx = m_fdContext[fd];
    }
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event) {
        //添加的事件与已存在的事件相同
        MUHUI_LOG_ERROR(g_logger) << "addEvent arrert fd=" << fd
            << " event=" << event
            << " fd_ctx.event=" << fd_ctx->events;
        MUHUI_ASSERT(!(fd_ctx->events & event));
    }
    //fd_ctx.event 存在事件说明是修改事件，否则是添加事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    //epoll_data_t(uinon) void* ptr
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd_ctx->fd, &epevent);
    if(rt) {
        MUHUI_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << " ," << fd << " ," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }
    
    //当前等待执行的事件数量加一
    ++ m_pendingEventCount;
    //更新当前socket事件上下文的事件为已注册事件
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    /// 断言当前事件上下文的调度器/协程/回调函数都为空（未设置）
    MUHUI_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    
    //设置当前事件执行的调度器/协程/回调函数
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
       event_ctx.cb.swap(cb); 
    } else {
        event_ctx.fiber = Fiber::GetThis(); //创建协程
        MUHUI_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContext.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContext[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    epoll_event newevent;
    if(!(fd_ctx->events & event)) {
        return false;
    }
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    newevent.events = EPOLLET | new_event;
    newevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd_ctx->fd, &newevent);
    if(rt) {
        MUHUI_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << " ," << fd << " ," << newevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContext.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContext[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    epoll_event epevent;
    if(!(fd_ctx->events & event)) {
        return false;
    }
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    //将该事件从epoll中删除，但是不改变该事件上下文事件
    int rt = epoll_ctl(m_epfd, op, fd_ctx->fd, &epevent);
    if(rt) {
        MUHUI_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << " ," << fd << " ," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;

}

bool IOManager::cancelAll(int fd)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContext.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContext[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    epoll_event epevent;
    if(!fd_ctx->events) {
        return false;
    }
    int op = EPOLL_CTL_DEL;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    //将该事件从epoll中删除，但是不改变该事件上下文事件
    int rt = epoll_ctl(m_epfd, op, fd_ctx->fd, &epevent);
    if(rt) {
        MUHUI_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << " ," << fd << " ," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    return true;
}

IOManager* IOManager::GetThis()
{
    //基类对象指针向下转换为子类对象指针
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


/**********************************************子类重写函数实现**********************************************************/
void IOManager::tickle()
{
    if(!hasIdleThreads()) {
        //没有空闲线程
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    MUHUI_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout)
{
    timeout = getNextTimer();
    //MUHUI_LOG_DEBUG(g_logger) << "timeout = " << (int)timeout;
    return timeout == ~0ull
        && Scheduler::stopping()
        && m_pendingEventCount == 0;
}
bool IOManager::stopping()
{
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle()
{
    //创建监听事件数组
    epoll_event* events = new epoll_event[265]();
    //智能指针不支持数组，定义删除函数在析构时自动释放events
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });
    while(true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            MUHUI_LOG_INFO(g_logger) << "name = " << getName() << " ,idle stopping exit";
            break;
        }
        //MUHUI_LOG_DEBUG(g_logger) << "next_timeout = " << (int)next_timeout;
        int rt = 0;
        do{
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            //成功返回触发I/O事件的文件句柄
            rt = epoll_wait(m_epfd, events, 256, int(next_timeout));
            if(rt < 0 && errno == EINTR) {
                
            } else {
                break;
            }
        }while(true);
        
        //获取定时器触发的回调函数集合
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        //批量添加至调度队列
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        } 

        for(int i = 0; i < rt; i++) {
            //从已注册事件中取出第i个事件
            epoll_event& event = events[i];
            //管道读端
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                //将信号全部读出（可能有多个线程给该线程发送信号"T"）
                while(read(m_tickleFds[0], &dummy, 1) > 0);
                continue;
            }

            FdContext* fd_ctx= (FdContext *)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            //EPOLLERR 读端， EPOLLHUP 写端，异常关闭
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                //事件添加读写监听
                event.events |= (EPOLLIN | EPOLLOUT) | fd_ctx->events;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                //读事件
                real_events |= READ;
            }

            if(event.events & EPOLLOUT) {
                //写事件
                real_events |= WRITE;
            }
            if((fd_ctx->events & real_events) == NONE) {
                //没有监听任务
                continue;
            }

            //剩余事件
            int left_event = (fd_ctx->events & ~real_events);
            int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = left_event & EPOLLET;
            //将剩余监听事件添加到epoll中
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2){
                MUHUI_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << " ," << fd_ctx->fd << " ," << event.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            //执行触发事件
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        //获取当前协程
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        //返回调度协程
        raw_ptr->swapOut();
    } //while

}

/**
 * @brief 当有新的定时器插入到定时器的首部,执行该函数
 */
void IOManager::onTimerInsertedAtFront()
{
   tickle(); 
}

}//muhui
