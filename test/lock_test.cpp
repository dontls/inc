#include "../lock.hpp"
#include <cstdio>
#include <thread>
#include <unistd.h>

libthread::Mutex mtx_;

static int count = 0;
void display() {
  libthread::Lock lock(&mtx_);
  count++;
  printf("%d\n", count);
}

int main(int argc, char const *argv[]) {
  std::thread th[10];
  for (int i = 0; i < 10; i++) {
    th[i] = std::thread(&display);
  }
  sleep(5);
  for (int i = 0; i < 10; i++) {
    th[i].join();
  }
}
