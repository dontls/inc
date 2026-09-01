#pragma once

#if USE_LIB_SPDLOG

#include "spdlog/spdlog.h"
#define SPDLOG_WARN2(b, ...)                                                   \
  SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(),                             \
                     (b) ? spdlog::level::warn : spdlog::level::debug,         \
                     __VA_ARGS__)

#define SPDLOG_ERROR2(b, ...)                                                  \
  SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(),                             \
                     (b) ? spdlog::level::err : spdlog::level::debug,          \
                     __VA_ARGS__)
#else

#include "time.hpp"
#define FMT_HEADER_ONLY 1
#include "fmt/base.h"
#ifdef _WIN32
#include <cstdarg>
#else
#include <stdarg.h>
#endif

#include <cstdio>
#include <mutex>

namespace liblog {
enum level {
  DEBUG, //
  INFO,  //
  WARN,  //
  ERR    //
};

static const char *_level_text[] = {
    "DEBUG", //
    " INFO", //
    " WARN", //
    "ERROR", //
};
static const char *_level_text_color[] = {
    "DEBUG",                //
    "\x1b[32m INFO\x1b[0m", //
    "\x1b[33m WARN\x1b[0m", //
    "\x1b[31mERROR\x1b[0m", //
};

class Logger {
private:
  int level_ = INFO;
  FILE *file_ = nullptr;
  std::mutex mtx_;

public:
  Logger() : file_(nullptr) {}
  ~Logger() {}

  void SetOutput(FILE *file) { file_ = file; }

  void SetLevel(int level) { level_ = level; }

  template <typename... T>
  void Println(int level, const char *filename, int line, const char *fmt,
               T &&...args) {
    if (level < level_) {
      return;
    }
    long long ts = libtime::UnixMilli();
    std::string s = libtime::Format(ts / 1000);
    int mills = int(ts % 1000);
    fmt::print(stdout, "[{}.{:03d}] {} {}:{} ", s, mills,
               _level_text_color[level], filename, line);
    fmt::println(stdout, fmt, args...);
    if (file_) {
      std::lock_guard<std::mutex> lock(mtx_);
      fmt::print(file_, "[{}.{:03d}] {} {}:{} ", s.c_str(), mills,
                 _level_text[level], filename, line);
      fmt::println(file_, fmt, args...);
    }
  }
};

inline Logger &Default() {
  static Logger logger;
  return logger;
}

} // namespace liblog

#define SPDLOG_DEBUG(...)                                                      \
  liblog::Default().Println(liblog::DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#define SPDLOG_INFO(...)                                                       \
  liblog::Default().Println(liblog::INFO, __FILE__, __LINE__, __VA_ARGS__)

#define SPDLOG_WARN(...)                                                       \
  liblog::Default().Println(liblog::WARN, __FILE__, __LINE__, __VA_ARGS__)

#define SPDLOG_ERROR(...)                                                      \
  liblog::Default().Println(liblog::ERR, __FILE__, __LINE__, __VA_ARGS__)

#define SPDLOG_WARN2(b, ...)                                                   \
  liblog::Default().Println((b) ? liblog::WARN : liblog::DEBUG, __FILE__,      \
                            __LINE__, ##__VA_ARGS__)

#define SPDLOG_ERROR2(b, ...)                                                  \
  liblog::Default().Println((b) ? liblog::ERR : liblog::DEBUG, __FILE__,       \
                            __LINE__, ##__VA_ARGS__)

#endif