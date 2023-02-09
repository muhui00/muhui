/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : scheduler.h
 * Author      : muhui
 * Created date: 2022-11-14 09:48:52
 * Description : 协程调度器
 *
 *******************************************/

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <memory>
#include <functional>
#include <iostream>
#include <vector>
#include <list>

#include "thread.h"
#include "fiber.h"

namespace muhui
{
/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] user_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "test"); 
    //虚析构
    virtual ~Scheduler();

    //获取协程调度器名称
    const std::string& getName() const { return m_name; }

    //返回当前协程调度器
    static Scheduler* GetThis();

    //返回当前协程调度器的调度协程
    static Fiber* GetMainFiber();

    //启动协程调度器
    void start();

    //关闭协程调度器
    void stop();


    /**
     * @brief 调度协程
     * @param[in] fc 协程或函数
     * @param[in] thread 协程执行的线程id,-1标识任意线程
     */
    template<class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                //向协程队列添加任务
                need_tickle = scheduleNoLock(fc, thread);
            }

            if(need_tickle) {
                tickle();
            }
        }

    /**
     * @brief 批量调度协程
     * @param[in] begin 协程数组的开始
     * @param[in] end 协程数组的结束
     */
    template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end) {
                    //批量添加
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
            }
            if(need_tickle) {
                tickle();
            }
        }
protected:
    //通知协程调度器有任务
    virtual void tickle();

    //协程调度函数
    void run();

    /**
     * @brief 返回是否可以停止
     */
    virtual bool stopping();

    /**
     * @brief 协程无任务可调度时执行idle协程
     */
    virtual void idle();

    //设置当前的协程调度器
    void setThis();

    //是否有空闲线程
    bool hasIdleThreads() { return m_idleThreadcount > 0; }
private:
    /**
     * @brief 协程调度启动(无锁)
     */
    template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread) {
            //协程队列是否为空
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if(ft.fiber || ft.cb) {
                //将任务加入到协程队列中
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }
private:
    /**
     * @brief 协程/函数/线程组
     */
    struct FiberAndThread
    {
        //协程
        Fiber::ptr fiber;
        //协程执行函数
        std::function<void()> cb;
        //线程id
        int thread;

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }
        
        /**
         * @brief 构造函数
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
                //fiber = *f 使用swap可以保证智能指针的引用不增加，防止智能指针释放内存错误
                //返回的*f为空指针
                fiber.swap(*f);
        }
    
        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] *f 协程执行函数
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
                cb.swap(*f);
        }
        
        //默认构造
        FiberAndThread()
            : thread(-1) {
        }
    
        //重置数据
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };


    
private:
    ///Mutex
    MutexType m_mutex;
    ///线程池
    std::vector<Thread::ptr> m_threads;
    ///待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    ///当前线程的主协程执行run的协程（use_caller为true时有效, 调度协程）
    Fiber::ptr m_rootFiber;
    ///协程调度器名称
    std::string m_name;

protected:
    ///协程下的线程id数组
    std::vector<int> m_threadIds;
    ///线程数量
    size_t m_threadCount;
    ///工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    ///空闲线程数量
    std::atomic<size_t> m_idleThreadcount = {0};
    ///是否正在停止
    bool m_stopping = true;
    ///是否自动停止
    bool m_autoStop = false;
    ///主线程id（use_caller）
    int m_rootThread = 0;
};

}//namespace muhui

#endif //__SCHEDULER_H__

