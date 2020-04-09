#ifndef __LOG_H__
#define __LOG_H__

#pragma once

#include "Time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DBG_FG_BLACK "\033[30m"
#define DBG_FG_RED "\033[31m"
#define DBG_FG_GREEN "\033[32m"
#define DBG_FG_YELLOW "\033[33m"
#define DBG_FG_BLUE "\033[34m"
#define DBG_FG_VIOLET "\033[35m"
#define DBG_FG_VIRDIAN "\033[36m"
#define DBG_FG_WHITE "\033[37m"

#define DBG_BG_BLACK "\033[40m"
#define DBG_BG_RED "\033[41m"
#define DBG_BG_GREEN "\033[42m"
#define DBG_BG_YELLOW "\033[43m"
#define DBG_BG_BLUE "\033[44m"
#define DBG_BG_VIOLET "\033[45m"
#define DBG_BG_VIRDIAN "\033[46m"
#define DBG_BG_WHITE "\033[47m"

#define DBG_END "\033[0m"

#define LOG(format, ...)                                                                          \
    {                                                                                             \
        printf("%s | %s:%d " format, NowUnixString().c_str(), __FILE__, __LINE__, ##__VA_ARGS__); \
    }
#define LOG_ERROR(format, ...)                                                                                        \
    {                                                                                                                 \
        printf(DBG_FG_RED "%s | %s: %d " format DBG_END, NowUnixString().c_str(), __FILE__, __LINE__, ##__VA_ARGS__); \
    }
#define LOG_WARN(format, ...)                                                                            \
    {                                                                                                    \
        printf(DBG_FG_YELLOW "%s | %s: %d " format DBG_END, NowUnixString().c_str(), __FILE__, __LINE__, \
               ##__VA_ARGS__);                                                                           \
    }

#endif