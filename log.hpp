#pragma once

#include <cstdio>
#include <mutex>

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
  std::mutex mtx_;

public:
  template <typename... T>
  void output(FILE *file, int level, const char *filename, int line,
              const char *fmt, T &&...args) {
    fprintf(file, "%s%s:%d:\x1b[0m ", Colors[level], filename, line);
    fprintf(file, fmt, args...);
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
    if (file_) {
      std::lock_guard<std::mutex> lock(mtx_);
      this->output(file_, level, filename, line, fmt, args...);
    } else {
      this->output(stdout, level, filename, line, fmt, args...);
    }
  }

  static Logger &Instance() {
    static Logger ins;
    return ins;
  }
};

} // namespace liblog

#define LogTrace(...)                                                          \
  liblog::Logger::Instance().Println(liblog::TRACE, __FILE__, __LINE__,        \
                                     ##__VA_ARGS__)

#define LogDebug(...)                                                          \
  liblog::Logger::Instance().Println(liblog::DEBUG, __FILE__, __LINE__,        \
                                     ##__VA_ARGS__)

#define LogInfo(...)                                                           \
  liblog::Logger::Instance().Println(liblog::INFO, __FILE__, __LINE__,         \
                                     ##__VA_ARGS__)

#define LogWarn(b, ...)                                                        \
  liblog::Logger::Instance().Println((b) ? liblog::WARN : liblog::TRACE,       \
                                     __FILE__, __LINE__, ##__VA_ARGS__);

#define LogError(b, ...)                                                       \
  liblog::Logger::Instance().Println((b) ? liblog::ERROR : liblog::TRACE,      \
                                     __FILE__, __LINE__, ##__VA_ARGS__);

#define LogFatal(b, ...)                                                       \
  liblog::Logger::Instance().Println((b) ? liblog::FATAL : liblog::TRACE,      \
                                     __FILE__, __LINE__, ##__VA_ARGS__);
