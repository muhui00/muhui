/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : scheduler.cc
 * Author      : muhui
 * Created date: 2022-11-14 09:48:59
 * Description : 协程调度器实现
 *
 *******************************************/

#define LOG_TAG "SCHEDULER"
#include "scheduler.h"
#include "macro.h"
#include "log.h"
#include "hook.h"

namespace muhui
{
muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

//线程局部静态变量
//当前线程的协程调度器
static thread_local Scheduler* t_scheduler = nullptr;
//当前协程
static thread_local Fiber* t_fiber = nullptr;

//初始化一个协程调度器
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name)
{
    MUHUI_ASSERT(threads > 0);
    
    //是否使用当前调用线程
    if(use_caller) {
        //当前线程的创建主协程
        muhui::Fiber::GetThis();
        --threads;

        //每一个线程内只能有一个协程调度器
        //未分配之前t_scheduler == nullptr
        MUHUI_ASSERT(GetThis() == nullptr);
        //当前线程协程调度器为当前对象
        //t_scheduler = this;
        setThis();

        //创建一个新的协程来执行协程调度函数 m_rootFiber --> new fiber(run)
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        muhui::Thread::SetName(m_name);
        //t_fiber 为当前执行run协程的调度协程
        t_fiber = m_rootFiber.get();
        //当前主线程id
        m_rootThread = muhui::GetThreadId();
        //放入线程池
        m_threadIds.push_back(m_rootThread);
    } else {
        //没有使用当前线程调度
        m_rootThread = -1;
    }
    //线程数量
    m_threadCount = threads;
}
//虚析构
Scheduler::~Scheduler()
{
    //当前线程正在停止
    MUHUI_ASSERT(m_stopping);
    //当前协程调度器为当前对象
    if(GetThis() == this) {
        //置空
        t_scheduler = nullptr;
    }
}

//返回当前协程调度器
Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}
void Scheduler::setThis() 
{
    t_scheduler = this;
}
//返回当前协程调度器的调度协程
Fiber* Scheduler::GetMainFiber()
{
    return t_fiber;
}

//启动协程调度器
void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);
    //m_stopping == false 时，说明正在执行，直接返回 
    if(!m_stopping) {
        return;
    }
    //否则 m_stopping == true;
    m_stopping = false;
    //当前线程未分配
    //线程池为空
    MUHUI_ASSERT(m_threads.empty());
    
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        //创建线程（执行run）及线程名称
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
    /* if(m_rootFiber) { */
    /*     m_rootFiber->call(); */
    /* } */
}

//关闭协程调度器
void Scheduler::stop()
{
    m_autoStop = true;
    //如果当前线程的主协程的执行run的协程存在，
    //且线程数目为0，且该协程的状态为结束或者初始化
    if(m_rootFiber 
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        MUHUI_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;
        //只有一个线程
        if(stopping()) {
            return;
        }
    }

    //当前线程数目不为0
    //在当前协程退出（结束）
    //bool exit_on_this_fiber = false;
    if(m_rootThread != -1) {
        //是use_caller线程
        MUHUI_ASSERT(GetThis() == this);
        //stop一定在scheduler创建线程里执行
    } else {
        MUHUI_ASSERT(GetThis() != this);
        //stop 可以在其他非创建线程里执行
    }
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        //唤醒所有线程，执行结束
        tickle();
    }

    if(m_rootFiber) {
    /* if(m_rootFiber) { */
    /*     m_rootFiber->call(); */
        tickle();
    }

    if(m_rootFiber) {
        if(!stopping()) {
            //此时m_rootFiber的主协程为main fiber
            //调度协程执行
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }

    //if(exit_on_this_fiber) {
    //}

}

void Scheduler::run()
{
    MUHUI_LOG_INFO(g_logger) << m_name << " run";
    muhui::set_hook_enable(true);
    //设置当前协程调度器为this
    setThis();
    //如果没有使用当前线程调度(use_caller == false)（m_rootThread == -1）
    if(muhui::GetThreadId() != m_rootThread) {
        //设置当前协程
        t_fiber = Fiber::GetThis().get();
    }

    //空闲协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    //回调函数协程
    Fiber::ptr cb_fiber;
    
    //协程、函数、线程-组
    FiberAndThread ft;
    while(true) {
        //置空
        ft.reset();
        //信号
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            //遍历当前待执行的协程队列
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //如果当前任务已经指定其他协程（线程）执行
                //it->thread ！= -1 说明是指定了线程
                if(it->thread != -1 && it->thread != muhui::GetThreadId()) {
                    //跳过当前任务
                    ++it;
                    //通知其他协程
                    tickle_me = true;
                    continue;
                }
                //当前任务没有指定其他协程（线程）来执行
                MUHUI_ASSERT(it->fiber || it->cb);
                //如果当前协程正在执行任务(信号不释放，资源占用中)
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    //跳过当前任务
                    ++it;
                    continue;
                }

                //当前协程没有执行任务(给当前任务分配协程去执行任务)
                ft = *it;
                //从待执行队列之中删除该任务
                m_fibers.erase(it);
                ++ m_activeThreadCount;
                is_active = true;
                break;
            }
        }

        if(tickle_me) {
            //通知协程调度器有任务
            tickle();
        }

        //如果当前协程存在不在结束状态
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {
            //工作线程数量
            //切换协程至执行状态
            ft.fiber->swapIn();
            --m_activeThreadCount;
            //如果当前协程状态为可执行状态
            if(ft.fiber->getState() == Fiber::READY) {
                //调度当前协程
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM
                       && ft.fiber->getState() != Fiber::EXCEPT) {
                //当前协程状态为EXEC INIT
                //设置当前协程状态为暂停状态
                ft.fiber->m_state = Fiber::HOLD;
            }
            //重置数据
            ft.reset();
        } else if (ft.cb) {
            //当前执行回调协程存在
            if(cb_fiber) {
                //重置cb_fiber执行函数为当前回调函数
                //reset 为 Fiber 的成员函数
                cb_fiber->reset(ft.cb);
            } else {
                //cb_fiber 的引用为0
                //指向新的对象
                //reset 为 shared_ptr 的成员函数
                //创建协程来执行回调函数
                //此时cb_fiber的主协程调度协程为m_rootFiber
                cb_fiber.reset(new Fiber(ft.cb));
            }
            //重置
            ft.reset();
            //切换cb_fiber的状态为执行状态
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                //cb_fiber 的状态为待执行
                //调用cb_fiber
                schedule(cb_fiber);
                //执行完之后清空
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::TERM 
                       || cb_fiber->getState() == Fiber::EXCEPT) {
                //指定当前协程执行函数为空
                cb_fiber->reset(nullptr);
            } else {
                //cb_fiber 的状态为EXEC INIT
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            //当前没有fiber or cb 存在，即没有执行任务
            if(idle_fiber->getState() == Fiber::TERM) {
                MUHUI_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            //执行空闲任务线程
            ++m_idleThreadcount;
            idle_fiber->swapIn();
            --m_idleThreadcount;
            if(idle_fiber->getState() != Fiber::TERM
               && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

//通知协程调度器有任务
void Scheduler::tickle()
{
    MUHUI_LOG_INFO(g_logger) << "tickle";
}

/**
 * @brief 返回是否可以停止
 */
bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_activeThreadCount == 0 && m_fibers.empty();
}

/**
 * @brief 协程无任务可调度时执行idle协程
 */
void Scheduler::idle()
{
    MUHUI_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        //将当前协程切换到后台,并设置为READY状态
        muhui::Fiber::YieldToReady();
    }
}

}//namespace muhui


