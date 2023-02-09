/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : iomanager.h
 * Author      : muhui
 * Created date: 2022-11-24 12:25:36
 * Description : 基于Epoll的IO协程调度器
 *
 *******************************************/

#ifndef __IOMANAGER_H__
#define __IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace muhui {

class IOManager : public Scheduler, public TimerManager
{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    //IO事件
    enum Event {
        /// 无事件
        NONE  = 0x0,
        /// 读事件        
        READ  = 0x1, //EPOLLIN
        /// 写事件
        WRITE = 0x4  //EPOLLOUT
    };
private:
    /*
     * socket事件上下文类
     * */
    struct FdContext {
        typedef Mutex MutexType;
        /*
         * 事件上下文类
         */
        struct EventContext {
            /// 事件执行的调度器
            Scheduler* scheduler = nullptr;
            /// 事件协程
            Fiber::ptr fiber;
            /// 事件的回调函数
            std::function<void()> cb;
        };

        /**
         * @brief 获取事件上下文类
         * @param[in] event 事件类型
         * @return 返回对应事件的上下文
         */
        EventContext& getContext(Event event);

        /**
         * @brief 重置事件上下文
         * @param[in, out] ctx 待重置的上下文类
         */
        void resetContext(EventContext& ctx);

        /**
         * @brief 触发事件
         * @param[in] event 事件类型
         */
        void triggerEvent(Event event);

        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联的文件句柄、
        int fd = 0;
        /// 已经注册的事件
        Event events = NONE;
        MutexType mutex;
    };

public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否将调用线程包含进去
     * @param[in] name 调度器的名称
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "test");

    ///析构函数
    //
    ~IOManager();
    /**
     * @brief 添加事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @param[in] cb 事件回调函数
     * @return 添加成功返回0,失败返回-1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 不会触发事件
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 如果事件存在则触发事件
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief 取消所有事件
     * @param[in] fd socket句柄
     */
    bool cancelAll(int fd);

    /**
     * @brief 返回当前的IOManager
     */
    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    /**
     * @brief 当有新的定时器插入到定时器的首部,执行该函数
     */
    void onTimerInsertedAtFront() override;

    /**
     * @brief 判断是否可以停止
     * @param[out] timeout 最近要出发的定时器事件间隔
     * @return 返回是否可以停止
     */
    bool stopping(uint64_t& timeout);

    /**
     * @brief 重置socket句柄上下文的容器大小
     * @param[in] size 容量大小
     */
    void contextResize(size_t size);
private:
    /// epoll事件文件句柄
    int m_epfd;
    /// pipe 文件句柄
    int m_tickleFds[2];

    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutex m_mutex;
    /// socket 上下文的容器
    std::vector<FdContext*> m_fdContext;
};

} //muhui
#endif //__IOMANAGER_H__
