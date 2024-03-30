#include "../time.hpp"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  printf("%s\n", Time::Format().c_str());
  printf("%s\n", Time::Format().c_str());
  long long startTime = Time::UnixMilli();
  if (!Time::Since(startTime, 90)) {
    printf("%lld\n", startTime);
  }
  sleep(2);
  if (Time::Since(startTime, 5000)) {
    printf("5s超时\n");
  } else {
    printf("-------------\n");
  }
  return 0;
}
