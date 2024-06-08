#include "../log.hpp"

int main(int argc, char const *argv[]) {
  liblog::Level = liblog::TRACE;
  LogTrace("%d", 1);
  LogDebug("%d", 1);
  LogInfo("%d %lld", 1, 1124555);
  LogWarn(true, "%d", 1);
  LogError(true, "%d", 1);
  LogFatal(true, "%d", 1);
  return 0;
}
