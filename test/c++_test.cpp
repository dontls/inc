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

  libcomm::Channel<int> chls(0);
  std::thread t([&] {
    for (const auto &it : chls) {
      printf("read %d\n", it);
    }
  });
  sleep(1);
  for (int i = 0; i < 8; i++) {
    chls << i;
    printf("write %d\n", i);
  }
  sleep(10);
  for (int i = 5; i < 10; i++) {
    chls << i;
    printf("write %d\n", i);
  }
  chls.close();
  t.join();
  return 0;
}
