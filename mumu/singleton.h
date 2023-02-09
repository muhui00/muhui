/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : singleton.h
 * Author      : muhui
 * Created date: 2022-11-05 22:06:47
 * Description : 单例模式封装
 *
 *******************************************/

#ifndef __SINGLETON_H__
#define __SINGLETON_H__
#include <memory>
namespace muhui
{
/**
 *  单例模式封装类
 *  T 类型
 *  X 为了创造多个实例对应的Tag
 *  N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class Singleton
{
public:
    //返回单例裸指针
    static T* GetInstance()
    {
        static T v;
        return &v;
    }
};

/**
 * 单例模式智能指针封装类
 * T 类型
 * X 为了创造多个实例对应的Tag
 * N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class SingletonPtr
{
public:
    static std::shared_ptr<T> GetInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};
}
#endif //__SINGLETON_H__

