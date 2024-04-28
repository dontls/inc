#include "../time.hpp"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  printf("%s\n", libtime::Format().c_str());
  printf("%s\n", libtime::Format().c_str());
  long long startTime = libtime::UnixMilli();
  if (!libtime::Since(startTime, 90)) {
    printf("%lld\n", startTime);
  }
  sleep(2);
  if (libtime::Since(startTime, 5000)) {
    printf("5s超时\n");
  } else {
    printf("-------------\n");
  }
  return 0;
}
