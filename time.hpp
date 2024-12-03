#pragma once
// 封装的time函数

#include <string>
#include <iomanip>
#include <ctime>
#include <sstream>
#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#else
#include <thread>
#include <chrono>
#endif

namespace libtime {
// 格式化时间
inline std::string Format(long long t = 0) {
  if (t >= 1e15) {
    t /= 1000000;
  } else if (t >= 1e12) {
    t /= 1000;
  } else if (t == 0) {
    t = time(NULL);
  }
  std::time_t t0 = t;
  struct tm timeinfo = {0};
#ifdef _WIN32
  localtime_s(&timeinfo, &t0);
#elif __linux__
  localtime_r(&t0, &timeinfo);
#endif
  char szText[24] = {0};
  strftime(szText, sizeof(szText), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return std::string(szText);
}

inline std::time_t Format2Unix(const char *str) {
  std::tm tm = {};
  std::istringstream ss(str);
  if (ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S")) {
    return std::mktime(&tm);
  }
  return 0;
}

inline long long UnixMilli() {
#ifdef __linux__
  struct timeval tv;
  gettimeofday(&tv, 0);
  long long llRet = tv.tv_sec;
  llRet *= 1000;
  llRet += tv.tv_usec / 1000;
  return llRet;
#else
  auto now = std::chrono::system_clock::now();
  return static_cast<long long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
#endif
}

// 超时
inline long long Since(long long tb) { return UnixMilli() - tb; }

inline void Sleep(unsigned int ms) {
#ifdef __linux__
  usleep(ms * 1000);
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}
} // namespace libtime
