#pragma once

#ifdef _WIN32
#include <cstdarg>
#else
#include <stdarg.h>
#endif

#include <cstdio>
#include <mutex>

#ifndef LOG_USE_TIME_LINE
#define LOG_USE_TIME_LINE 1
#endif

#if LOG_USE_TIME_LINE
#include "time.hpp"
#endif

namespace liblog {
enum level {
  DEBUG, //
  INFO,  //
  WARN,  //
  ERR    //
};

static const char *_levelout[] = {
    "",                      //
    "\x1b[32m INFO\x1b[0m",  //
    "\x1b[33m WARN\x1b[0m",  //
    "\x1b[31m ERROR\x1b[0m", //
};

static int Level = DEBUG;

class Logger {
private:
  FILE *file_;
  std::mutex mtx_;

  // template <typename... T>
  // void foutput(FILE *file, const char *fmt, T &&...args) {
  //   fprintf(file, fmt, args...);
  //   fprintf(file, "\n");
  //   fflush(file);
  // }

  void foutput(FILE *file, const char *fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);
    vfprintf(file, fmt, vlist);
    va_end(vlist);
    fprintf(file, "\n");
    fflush(file);
  }

public:
  Logger() : file_(nullptr) {}
  ~Logger() {}

  void SetOutput(FILE *file) { file_ = file; }

  template <typename... T>
  void Println(int level, const char *filename, int line, const char *fmt,
               T &&...args) {
    if (level < Level) {
      return;
    }
#if LOG_USE_TIME_LINE
    long long ts = libtime::UnixMilli();
    std::string s = libtime::Format(ts / 1000);
    int mills = int(ts % 1000);
    fprintf(stdout, "%s.%03d%s %s:%d ", s.c_str(), mills, _levelout[level],
            filename, line);
#endif
    this->foutput(stdout, fmt, args...);
    if (file_) {
      std::lock_guard<std::mutex> lock(mtx_);
#if LOG_USE_TIME_LINE
      fprintf(file_, "%s.%03d%s %s:%d ", s.c_str(), mills, _levelout[level],
              filename, line);
#endif
      this->foutput(file_, fmt, args...);
    }
  }

  static Logger &Instance() {
    static Logger ins;
    return ins;
  }
};

} // namespace liblog

#define LogDebug(...)                                                          \
  liblog::Logger::Instance().Println(liblog::DEBUG, __FILE__, __LINE__,        \
                                     ##__VA_ARGS__)

#define LogInfo(...)                                                           \
  liblog::Logger::Instance().Println(liblog::INFO, __FILE__, __LINE__,         \
                                     ##__VA_ARGS__)

#define LogWarn(b, ...)                                                        \
  liblog::Logger::Instance().Println((b) ? liblog::WARN : liblog::DEBUG,       \
                                     __FILE__, __LINE__, ##__VA_ARGS__);

#define LogError(b, ...)                                                       \
  liblog::Logger::Instance().Println((b) ? liblog::ERR : liblog::DEBUG,        \
                                     __FILE__, __LINE__, ##__VA_ARGS__);
