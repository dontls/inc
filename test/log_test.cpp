#include "../log.hpp"


int main(int argc, char const *argv[]) {
  FILE *file = fopen("./log-rotating.txt", "a+");
  liblog::Default().SetOutput(file);
  liblog::Default().SetLevel(liblog::DEBUG);
  SPDLOG_DEBUG("{}", 1);
  SPDLOG_INFO("{}", 1);
  SPDLOG_WARN( "{}", 1);
  SPDLOG_ERROR("{}", 1);
  fclose(file);
  return 0;
}
