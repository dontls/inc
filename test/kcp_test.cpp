#include "../ikcp.hpp"
#include <cstdio>

int main(int argc, char const *argv[]) {
  libkcp::Client cli;
  cli.Dial("127.0.0.1", 1234);
  for (;;) {
    char buf[512] = {0};
    int n = cli.Read(buf, 512);
    if (n > 0) {
      printf("%s\n", buf);
    }
  }
  return 0;
}
