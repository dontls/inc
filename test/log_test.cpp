#include "../log.hpp"

int main(int argc, char const *argv[]) {
  FILE *file = fopen("./log.txt", "w+");
  liblog::Logger::Instance().SetOutput(file);
  liblog::Level = liblog::DEBUG;
  LogDebug("%d", 1);
  LogInfo("%d %lld", 1, 1124555);
  LogWarn(true, "%d", 1);
  LogError(true, "%d", 1);
  fclose(file);
  return 0;
}
