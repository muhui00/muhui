/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : fiber.h
 * Author      : muhui
 * Created date: 2022-11-14 02:12:15
 * Description : 协程封装
 *
 *******************************************/

#ifndef __FIBER_H__
#define __FIBER_H__

#include <memory>
#include <ucontext.h> //用于用户态下的上下文切换
#include <functional>

namespace muhui
{

class Scheduler;
//继承enable_shared_from_this的作用是获取当前类的智能指针
//继承之后不可以在栈上创建对象 
class Fiber : public std::enable_shared_from_this<Fiber>
{
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
    * @brief 协程状态
    */
    enum State {
        ///初始化状态
        INIT,
        ///暂停状态
        HOLD,
        ///执行中状态
        EXEC,
        ///结束状态
        TERM,
        ///可执行状态
        READY,
        ///异常状态
        EXCEPT
    };

private:
    /**
     * @brief 无参构造函数
     * @attention 每个线程第一个协程的构造
     */
    Fiber();
public:
    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() = EXEC
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     */
    void swapOut();

    /**
     * @brief 将当前线程切换到执行状态
     * @pre 执行的为当前线程的主协程
     */
    void call();


    /**
     * @brief 将当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */
    void back();

    /**
     * @brief 返回协程id
     */
    uint64_t getId() const { return m_id; }

    /**
     * @brief 返回协程状态
     */
    State getState() const { return m_state; }

public:
        /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 运行协程
     */
    static void SetThis(Fiber* f);

    /**
     * @brief 返回当前所在的协程
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 将当前协程切换到后台,并设置为READY状态
     * @post getState() = READY
     */
    static void YieldToReady();

    /**
     * @brief 将当前协程切换到后台,并设置为HOLD状态
     * @post getState() = HOLD
     */
    static void YieldToHold();

    /**
     * @brief 返回当前协程的总数量
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程调度协程
     */
    static void CallerMainFunc();

    /**
     * @brief 获取当前协程的id
     */
    static uint64_t GetFiberId();
private:
    ///协程id
    uint64_t m_id = 0;

    ///协程运行栈大小
    uint32_t m_stacksize = 0;

    ///协程状态
    State m_state = INIT;

    /*
    typedef struct ucontext_t
     {
        unsigned long int __ctx(uc_flags);
        struct ucontext_t *uc_link;
        stack_t uc_stack;   //上下文中使用的栈
        mcontext_t uc_mcontext; //上下文的特定寄存器
        sigset_t uc_sigmask;    //上下文中阻塞信号集合
        struct _libc_fpstate __fpregs_mem;
     } ucontext_t;*/
    ///协程上下文(保存程序某一个节点的环境)
    ucontext_t m_ctx;

    ///协程运行栈指针
    void* m_stack = nullptr;

    ///协程运行函数
    std::function<void()> m_cb;

};

} //namespace muhui
#endif //__FIBER_H__

