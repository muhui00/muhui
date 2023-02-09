/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : endian.hh
 * Author      : muhui
 * Created date: 2022-12-07 19:24:18
 * Description : 字节序操作函数
 *
 *******************************************/

#ifndef __ENDIAN_HH__
#define __ENDIAN_HH__

#define MUHUI_LITTLE_ENDIAN 1 //小端
#define MUHUI_BIG_ENDIAN 2 //大端

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

namespace muhui {

/**
 * @brief 8字节类型的字节序转化
 * 函数模板不能偏特化
 * 使用 std::enable_if 进行条件选择
 * 实现函数多态
 */
template<class T> 
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type //函数类型，当enable_if条件满足时，类型T有效
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 4字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type //函数类型，当enable_if条件满足时，类型T有效
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 2字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type //函数类型，当enable_if条件满足时，类型T有效
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

//内部宏定义
#if BYTE_ORDER == BIG_ENDIAN //如果字节序为大端
#define MUHUI_BYTE_ORDER MUHUI_BIG_ENDIAN //定义宏等于MUHUI_BIG_ENDIAN
#else
#define MUHUI_BYTE_ORDER MUHUI_LITTLE_ENDIAN
#endif

#if MUHUI_BYTE_ORDER == MUHUI_BIG_ENDIAN

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}
#else

/**
 * @brief 只在大端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}

template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}
#endif

}//muhui
#endif //__ENDIAN_H__

