#include "../singleton.hpp"
#include <iostream>

class A {
private:
  /* data */
public:
  void Display() { std::cout << "Display " << this << "\n"; }
};

int main(int argc, char const *argv[]) {
  libsingle::Object<A>::Instance().Display();
  libsingle::Object<A>::Instance().Display();
  libsingle::Object<A>::Instance().Display();
  return 0;
}
