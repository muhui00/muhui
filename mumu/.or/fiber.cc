/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : fiber.cpp
 * Author      : muhui
 * Created date: 2022-11-14 02:55:55
 * Description : 协程实现
 *
 *******************************************/

#define LOG_TAG "FIBER"

#include <atomic>

#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

namespace muhui 
{
static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

/*std::atomic<type> 原子操作数据类型*/
///协程id
static std::atomic<uint64_t> s_fiber_id {0};
///协程数
static std::atomic<uint64_t> s_fiber_count {0};

//当前协程
static thread_local Fiber* t_fiber = nullptr;
//当前协程（智能指针）
static thread_local Fiber::ptr t_threadFiber = nullptr;

//读取yaml配置
static muhui::ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

//栈内存分配回收类
class MallocStackAllocator
{
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

//每个线程的第一个协程构造（main fiber）
Fiber::Fiber()
{
    //默认执行中状态
    m_state = EXEC;
    //设置当前线程的运行协程
    SetThis(this);
    //获取当前环境（上下文）
    //初始化并保存当前的上下文
    if(getcontext(&m_ctx)) {
        MUHUI_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    MUHUI_LOG_DEBUG(g_logger) << "Fiber::Fiber() main";
}
/**
 * @brief 构造函数
 * @param[in] cb 协程执行的函数
 * @param[in] stacksize 协程栈大小
 * @param[in] use_caller 是否在MainFiber上调度
 */
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id)
    , m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        MUHUI_ASSERT2(false, "getcontext");
    }

    //关联上下文的指针
    m_ctx.uc_link = nullptr;
    //m_ctx的栈指针
    m_ctx.uc_stack.ss_sp = m_stack;
    //m_ctx的栈大小
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    //创建一个新的上下文
    //调用makecontext切换上下文，并指定用户线程中要执行的函数
    //切换到协程执行函数
    if(!use_caller){
        //线程主协程->协程调度(run)协程
        //执行完成之后返回到线程主协程
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        //(执行任务)协程->协程调度(run)协程
        //执行完成之后返回到线程调度协程
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    MUHUI_LOG_DEBUG(g_logger) << "Fiber::Fiber(p) id = " << m_id;
}

Fiber::~Fiber()
{
    --s_fiber_count;
    if(m_stack) {
        MUHUI_ASSERT(m_state == TERM
                     || m_state == EXCEPT
                     || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        //当前回调不存在，且协程状态为正在执行
        MUHUI_ASSERT(!m_cb);
        MUHUI_ASSERT(m_state == EXEC);
        //栈不存在 将当前运行协程指针置空
        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    MUHUI_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id = " << m_id
                            << " total = " << s_fiber_count;
}

/**
 * @brief 重置协程执行函数,并设置状态
 * @pre getState() 为 INIT, TERM, EXCEPT
 * @post getState() = INIT
 */
void Fiber::reset(std::function<void()> cb)
{
    MUHUI_ASSERT( m_state == INIT
                  || m_state == TERM
                  || m_state == EXCEPT);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        MUHUI_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    m_state = INIT;
}


/*
 * swapIn/swapOut 切换的上下文为(run)"调度协程"与调度协程创建的"(任务执行)协程"两者之间的切换
 * */

/**
 * @brief 将当前协程切换到运行状态
 * @pre getState() != EXEC
 * @post getState() = EXEC
 */
void Fiber::swapIn()
{
    //this->Fiber(); 默认构造
    SetThis(this);
    MUHUI_ASSERT( m_state != EXEC);
    m_state = EXEC;
    //m_ctx 主协程 交换上下文（环境）
    //切换到新协程执行任务 调度协程->任务协程
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        MUHUI_ASSERT2(false, "swapcontext");
    }
}

/**
 * @brief 将当前协程切换到后台
 */
void Fiber::swapOut()
{
    //get() 获取当前智能指针对象的指针对象(子协程)
    SetThis(Scheduler::GetMainFiber());
    //任务执行完成，切换为原主协程， 任务协程->调度协程
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        MUHUI_ASSERT2(false, "swapcontext");
    }
}


/*
 * call/back 切换的上下文为当前线程的"主协程""与其创建(run)"调度协程"两者之间的切换
 * */


/**
 * @brief 将当前线程切换到执行状态
 * @pre 执行的为当前线程的主协程
 */
void Fiber::call()
{
    SetThis(this);
    m_state = EXEC;
    //m_ctx 主协程 交换上下文（环境）
    //t_threadFiber 为当前线程（this）的主协程
    //切换到this
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        MUHUI_ASSERT2(false, "swapcontext");
    }
}


/**
 * @brief 将当前线程切换到后台
 * @pre 执行的为该协程
 * @post 返回到线程的主协程
 */
void Fiber::back()
{
    SetThis(t_threadFiber.get());
    //切换到主协程
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        MUHUI_ASSERT2(false, "swapcontext");
    }
}

/**
 * @brief 设置当前线程的运行协程
 * @param[in] f 运行协程
 */
void Fiber::SetThis(Fiber* f)
{
    t_fiber = f;
}

/**
 * @brief 返回当前所在的协程
 */
Fiber::ptr Fiber::GetThis()
{
    //t_fiber存在
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    //t_fiber不存在，说明主协程未创建，创建主协程
    Fiber::ptr main_fiber(new Fiber);
    MUHUI_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    //Fiber() -> SetThis(this);
    return t_fiber->shared_from_this();
}

/**
 * @brief 将当前协程切换到后台,并设置为READY状态
 * @post getState() = READY
 */
void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    MUHUI_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

/**
 * @brief 将当前协程切换到后台,并设置为HOLD状态
 * @post getState() = HOLD
 */
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    MUHUI_ASSERT(cur->m_state == EXEC);
    cur->m_state = HOLD;
    cur->swapOut();
}

/**
 * @brief 返回当前协程的总数量
 */
uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

/**
 * @brief 协程执行函数
 * @post 执行完成返回到线程主协程
 */
void Fiber::MainFunc(){
    //智能指针引用+1
    //当引用不能<1,不释放
    Fiber::ptr cur = GetThis();
    MUHUI_ASSERT(cur);
    try {
        //执行回调函数
        cur->m_cb();
        //执行完置空，防止智能指针释放错误
        cur->m_cb = nullptr;
        //设置该协程执行状态为结束状态
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        MUHUI_LOG_ERROR(g_logger) << "Fiber Except " << ex.what()
            << " fiber_id=" << cur->getId() 
            << std::endl
            << muhui::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        MUHUI_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId() 
            << std::endl
            << muhui::BacktraceToString();

    };
    //获得该智能指针的裸指针
    auto raw_ptr = cur.get();
    //释放该指针
    cur.reset();
    //返回上一层
    //此时当前所在协程为main fiber创建的run函数调度协程
    //也就是m_rootfiber
    //返回上一层为主线程的主协程
    raw_ptr->swapOut();
    MUHUI_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

/**
 * @brief 协程执行函数
 * @post 执行完成返回到线程调度协程
 */
void Fiber::CallerMainFunc()
{
    //智能指针引用+1
    //当引用不能<1,不释放
    Fiber::ptr cur = GetThis();
    MUHUI_ASSERT(cur);
    try {
        //执行回调函数
        cur->m_cb();
        //执行完置空，防止智能指针释放错误
        cur->m_cb = nullptr;
        //设置该协程执行状态为结束状态
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        MUHUI_LOG_ERROR(g_logger) << "Fiber Except " << ex.what()
            << " fiber_id=" << cur->getId() 
            << std::endl
            << muhui::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        MUHUI_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId() 
            << std::endl
            << muhui::BacktraceToString();

    };
    //获得该智能指针的裸指针
    auto raw_ptr = cur.get();
    //释放该指针
    cur.reset();
    //返回线程调度协程
    //当前所在的协程为调度(run)协程创建的(执行任务)协程
    //此时返回协程为调度(run)协程
    raw_ptr->back();
    MUHUI_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

}

/**
 * @brief 获取当前协程的id
 */
uint64_t Fiber::GetFiberId()
{
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

}//namespace muhui
