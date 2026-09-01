#define USE_LIB_SPDLOG 1
#include "log.hpp"

int main() {
  SPDLOG_INFO("Hello, {}!", "World");
  SPDLOG_WARN_AUTO(1, "Hello, {}!", "World");
  SPDLOG_ERROR_AUTO(1, "Hello, {}!", "World");
  return 0;
}
