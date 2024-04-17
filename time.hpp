// 封装的time函数
#pragma once

#include <string>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <sys/time.h>

namespace libtime {

// 格式化时间
inline std::string Format(unsigned int _t = 0) {
  time_t _tmp = _t;
  if (_tmp == 0) {
    _tmp = time(NULL);
  }
  struct tm Stm = *localtime(&_tmp);
  char szText[24] = {0};
  sprintf(szText, "%04d-%02d-%02d %02d:%02d:%02d", Stm.tm_year + 1900,
          Stm.tm_mon + 1, Stm.tm_mday, Stm.tm_hour, Stm.tm_min, Stm.tm_sec);

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
} // namespace libtime
