#pragma once

#include <cstdio>

namespace liblog {
enum level {
  TRACE, //
  DEBUG, //
  INFO,  //
  WARN,  //
  ERROR, //
  FATAL
};

static const char *Colors[] = {
    "\x1b[0m",  //
    "\x1b[36m", //
    "\x1b[32m", //
    "\x1b[33m", //
    "\x1b[31m", //
    "\x1b[35m", //
};

static int Level = TRACE;

class Logger {
private:
  FILE *file_;

public:
  Logger(FILE *file) : file_(file) {}
  ~Logger() {}

  template <typename... T>
  void Fprintln(int level, const char *filename, int line, const char *fmt,
                T &&...args) {
    if (level < Level) {
      return;
    }
    fprintf(file_, "%s%s:%d:\x1b[0m ", Colors[level], filename, line);
    fprintf(file_, fmt, args...);
    fprintf(file_, "\n");
    fflush(file_);
  }
};

template <typename... T>
inline void fprintln(int level, const char *filename, int line, const char *fmt,
                     T &&...args) {
  Logger lg(stdout);
  lg.Fprintln(level, filename, line, fmt, args...);
}
} // namespace liblog

#define LogTrace(...)                                                          \
  liblog::fprintln(liblog::TRACE, __FILE__, __LINE__, ##__VA_ARGS__)

#define LogDebug(...)                                                          \
  liblog::fprintln(liblog::DEBUG, __FILE__, __LINE__, ##__VA_ARGS__)

#define LogInfo(...)                                                           \
  liblog::fprintln(liblog::INFO, __FILE__, __LINE__, ##__VA_ARGS__)

#define LogWarn(b, ...)                                                        \
  if (b) {                                                                     \
    liblog::fprintln(liblog::WARN, __FILE__, __LINE__, ##__VA_ARGS__);         \
  } else {                                                                     \
    liblog::fprintln(liblog::TRACE, __FILE__, __LINE__, ##__VA_ARGS__);        \
  }

#define LogError(b, ...)                                                       \
  if (b) {                                                                     \
    liblog::fprintln(liblog::ERROR, __FILE__, __LINE__, ##__VA_ARGS__);        \
  } else {                                                                     \
    liblog::fprintln(liblog::TRACE, __FILE__, __LINE__, ##__VA_ARGS__);        \
  }

#define LogFatal(b, ...)                                                       \
  if (b) {                                                                     \
    liblog::fprintln(liblog::FATAL, __FILE__, __LINE__, ##__VA_ARGS__);        \
  } else {                                                                     \
    liblog::fprintln(liblog::TRACE, __FILE__, __LINE__, ##__VA_ARGS__);        \
  }
