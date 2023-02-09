/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_util.cc
 * Author      : muhui
 * Created date: 2022-11-13 15:17:33
 * Description : 
 *
 *******************************************/

#define LOG_TAG "TEST_UTIL"

#include <iostream>
#include <assert.h>
#include "muhui.h"
#include "macro.h"

muhui::Logger::ptr g_logger = MUHUI_LOG_ROOT();

void test_assert() {
    MUHUI_LOG_INFO(g_logger) << muhui::BacktraceToString(10);
    MUHUI_ASSERT2(0 == 1, "abcdef xx");
}

int main(int argc, char *argv[]) {
    test_assert();
    return 0;
}

