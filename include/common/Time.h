#pragma once

// 封装的time函数

#ifndef __TIME_H__
#define __TIME_H__

#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// 毫秒休眠
void mSleep(uint32_t msec)
{
    usleep(msec * 1000);
}

// 获取时间Tick
unsigned long long NowTickCount()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    unsigned long long llret = tv.tv_usec;
    llret *= 1000;
    llret += tv.tv_usec / 1000;
    return llret;
}

// 超时毫秒
bool TimeoutMsec(unsigned long long &dbtime, unsigned long long msec,
                 unsigned long long *escape_ms = nullptr)
{
    bool               retCode = false;
    unsigned long long ullnow  = NowTickCount();
    if (ullnow >= dbtime) {
        if (nullptr != escape_ms) {
            *escape_ms = ullnow - dbtime;
        }

        if (ullnow - dbtime >= msec) {
            retCode = true;
        }
    } else {
        dbtime = ullnow;
    }
    return retCode;
}

// 超时秒
bool TimeoutSec(unsigned long long &dbtime, unsigned long long sec,
                unsigned long long *escape_ms = nullptr)
{
    return TimeoutMsec(dbtime, sec * 1000, escape_ms);
}

// 格式化uinx字符串
std::string ToUnixTimeString(time_t t)
{
    char       buf[32] = {0};
    struct tm *tm_time = localtime(&t);
    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
             tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    return buf;
}

// 获取当前unix时间 "2000-01-01 00:00:00"
std::string NowUnixString()
{
    struct tm tv  = {0};
    time_t    now = time(nullptr);
    return ToUnixTimeString(now);
}

// uinx时间戳
time_t ToUnixTimestamp(const std::string &timeStr)
{
    tm tm_time;
    memset(&tm_time, 0, sizeof(tm_time));

    tm_time.tm_year = atoi(timeStr.substr(0, 4).c_str()) - 1900;
    tm_time.tm_mon  = atoi(timeStr.substr(5, 2).c_str()) - 1;
    tm_time.tm_mday = atoi(timeStr.substr(8, 2).c_str());
    tm_time.tm_hour = atoi(timeStr.substr(11, 2).c_str());
    tm_time.tm_min  = atoi(timeStr.substr(14, 2).c_str());
    tm_time.tm_sec  = atoi(timeStr.substr(17, 2).c_str());

    return mktime(&tm_time);
}

#ifndef _WIN32
int32_t AbsTimespace(struct timespec *ts, int32_t ms)
{
    if (NULL == ts) return EINVAL;

    struct timeval tv;
    int32_t        ret = gettimeofday(&tv, NULL);
    if (0 != ret) return ret;

    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000UL;

    ts->tv_sec += ms / 1000UL;
    ts->tv_nsec += ms % 1000UL * 1000000UL;

    ts->tv_sec += ts->tv_nsec / (1000UL * 1000UL * 1000UL);
    ts->tv_nsec %= (1000UL * 1000UL * 1000UL);

    return 0;
}
#endif

#endif