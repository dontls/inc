// 封装的time函数
#pragma once

#include <string>
#include <iomanip>
#include <ctime>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <chrono>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

namespace libtime {

// 格式化时间
inline std::string Format(std::time_t t = 0) {
  if (t == 0) {
    t = time(NULL);
  }
  struct tm timeinfo = {0};
#ifdef _WIN32
  localtime_s(&timeinfo, &t);
#else
  localtime_r(&t, &timeinfo);
#endif
  char szText[24] = {0};
  strftime(szText, sizeof(szText), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return std::string(szText);
}

inline std::time_t Format2Unix(const char *str) {
  std::tm tm = {};
  std::istringstream ss(str);
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  long ts = std::mktime(&tm);
  // ts += tm.tm_gmtoff;
  return ts;
}

inline long long UnixMilli() {
#ifdef _WIN32
  auto now = std::chrono::system_clock::now();
  return static_cast<long long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
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
#ifdef _WIN32
  return ::Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}
} // namespace libtime
