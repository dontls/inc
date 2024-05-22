// 封装的time函数
#pragma once

#include <string>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <sys/time.h>
#if __linux__
#include <unistd.h>
#endif

namespace libtime {

// 格式化时间
inline std::string Format(unsigned int _t = 0) {
  time_t _tmp = _t;
  if (_tmp == 0) {
    _tmp = time(NULL);
  }
  struct tm timeinfo;
#ifdef _WIN32
  localtime_s(&timeinfo, &timeStamp);
#else
  localtime_r(&_tmp, &timeinfo);
#endif
  char szText[24] = {0};
  strftime(szText, sizeof(szText), "%Y-%m-%d %H:%M:%S", &timeinfo);

  return std::string(szText);
}

inline std::time_t Format2Unix(const char *str) {
  std::tm tm = {};
  std::istringstream ss(str);
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  return std::mktime(&tm);
}

inline long long UnixMilli() {
#if defined(WIN32) || defined(_WINDOWS)
  return ::GetTickCount();
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  long long llRet = tv.tv_sec;
  llRet *= 1000;
  llRet += tv.tv_usec / 1000;
  return llRet;
#endif
}

// 超时判断
inline bool Since(long long tb, long long mills) {
  return (tb + mills) < UnixMilli();
}

inline void Sleep(unsigned int ms) {
#if defined(WIN32) || defined(_WINDOWS)
  return ::Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}
} // namespace libtime
