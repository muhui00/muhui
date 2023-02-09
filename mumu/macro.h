/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : macro.h
 * Author      : muhui
 * Created date: 2022-11-13 15:15:27
 * Description : 常用宏的封装
 *
 *******************************************/

#ifndef __MACRO_H__
#define __MACRO_H__

#include <assert.h>
#include <string.h>
#include "util.h"
#include "log.h"

//编译器优化
#if defined __GNUC__ || defined  __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define MUHUI_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define MUHUI_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define MUHUI_LIKELY(x)      (x)
#   define MUHUI_UNLIKELY(x)      (x)
#endif


//断言
#define MUHUI_ASSERT(x) \
    if(MUHUI_UNLIKELY(!(x))) { \
        MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace\n" \
            << muhui::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define MUHUI_ASSERT2(x, w) \
    if(MUHUI_UNLIKELY(!(x))) { \
        MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace\n" \
            << muhui::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif //__MACRO_H__

