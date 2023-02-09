/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : timer.cpp
 * Author      : muhui
 * Created date: 2022-11-25 00:07:08
 * Description : 定时器类实现
 *
 *******************************************/

#define LOG_TAG "TIMER"
#include "timer.h"
#include "util.h"
#include "log.h"

namespace muhui {
static Logger::ptr g_logger = MUHUI_LOG_ROOT();
bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const 
{
    if(!lhs && !rhs) {
        return false;
    }
    if(!lhs) {
        return true;
    }
    if(!rhs) {
        return false;
    }
    //精确执行时间
    if(lhs->m_next < rhs->m_next) {
        return true;
    } 
    if(rhs->m_next < lhs->m_next) {
        return false;
    }
    //裸指针 lhs->m_next == rhs->m_next
    return lhs.get() < rhs.get();
}

bool Timer::cancel()
{
   TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timer.find(shared_from_this());
        //从定时器集合中删除
        m_manager->m_timer.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh()
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timer.find(shared_from_this());
    if(it == m_manager->m_timer.end()) {
        return false;
    }
    m_manager->m_timer.erase(it);
    //刷新定时器精确执行时间，当前时间+执行周期时间
    m_next = muhui::GetCurrentMS() + m_ms;
    m_manager->m_timer.insert(shared_from_this());
    return true;

}

bool Timer::reset(uint64_t ms, bool from_now)
{
    if(ms == m_ms && !from_now) {
        return false;
    }

    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timer.find(shared_from_this());
    if(it == m_manager->m_timer.end()) {
        return false;
    }

    m_manager->m_timer.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = muhui::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    //重新添加定时器，判断执行顺序
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

Timer::Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager)
    : m_recurring(recurring)
    , m_ms(ms)
    , m_cb(cb)
    , m_manager(manager)
{
    m_next = muhui::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next)
    : m_next(next)
{}

TimerManager::TimerManager()
{
    m_previousTime = muhui::GetCurrentMS();
}

TimerManager::~TimerManager()
{

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, 
                    bool recurring)
{
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
    //weak_ptr 但该类型指针通常不单独使用（实际没有用处），只能和shared_ptr类型指针搭配使用。
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                             std::weak_ptr<void> weak_cond,
                             bool recurring)
{
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}
uint64_t TimerManager::getNextTimer()
{
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timer.empty()) {
        //MUHUI_LOG_DEBUG(g_logger) << "m_timer empty";
        return ~0ull;
    }

    //取出集合中第一个计时器
    const Timer::ptr& next = *m_timer.begin();
    uint64_t now_ms = muhui::GetCurrentMS(); //当前时间
    //当前定时器已过时
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        //返回到当前时间最近定时器的时间间隔
        return next->m_next - now_ms;
    }
}

//获取需要执行的定时器的回调函数列表
void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs)
{
    uint64_t now_ms = muhui::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timer.empty()) {
            return;
        }
    }

    RWMutexType::WriteLock lock(m_mutex);
    //检测服务器时间是否被调后了
    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timer.begin())->m_next > now_ms)) {
        //服务器时间未调后，且第一个定时器的精确执行时间大于当前时间
        //则没有要执行的回调函数
        return;
    }
    Timer::ptr now_timer(new Timer(now_ms));
    //返回now_timer在集合中的迭代器（与m_timer集合中的数据比较返回位置）
    //如果服务器时间被调后了，将全部定时器集合内的回调函数获取
    //否则获取已过期的回调函数列表
    auto it = rollover ? m_timer.end() : m_timer.lower_bound(now_timer);
    //同一时间的定时器可能有多个
    while(it != m_timer.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    //将m_timer的前it个数据插入到expired
    expired.insert(expired.begin(), m_timer.begin(), it);
    m_timer.erase(m_timer.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        //如果该定时器需要循环        
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timer.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

bool TimerManager::hasTimer()
{
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timer.empty();
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock)
{
    //set insert 插入返回一个pair，first为指向插入元素的迭代器， second为是否插入成功
    auto it = m_timer.insert(val).first;
    //插入的定时器是否排在最前面
    bool at_front = (it == m_timer.begin()) && !m_tickled;
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();
    if(at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover = false;
    if(now_ms < m_previousTime &&
       now_ms < (m_previousTime - 60 * 60 * 1000)) {
        //服务器时间被调后了1小时
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

}//muhui
