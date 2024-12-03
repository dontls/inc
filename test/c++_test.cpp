#include "../singleton.hpp"
#include "../channel.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>
class A {
private:
  /* data */
public:
  void Display() { std::cout << "Display " << this << "\n"; }
};

int main(int argc, char const *argv[]) {
  libcomm::Singleton<A>::Instance().Display();
  libcomm::Singleton<A>::Instance().Display();
  libcomm::Singleton<A>::Instance().Display();

  libsync::Channel<int> chls(1);
  std::thread t([&] {
    for (const auto &it : chls) {
      sleep(2);
      printf(":------> read %d\n", it);
    }
  });
  sleep(1);
  for (int i = 0; i < 8; i++) {
    chls << i;
    printf(":---------> write %d\n", i);
  }
  sleep(3);
  for (int i = 8; i < 15; i++) {
    chls << i;
    usleep(10000);
    printf(":---------> write %d\n", i);
  }
  chls.close();
  t.join();
  return 0;
}
