#include "../string.hpp"

int main(int argc, char const *argv[]) {
  std::string s("123abc456abc789abc");
  auto v = libstr::Split(s, "abc");
  for (const auto &s1 : v) {
    printf("%s\n", s1.c_str());
  }
  libstr::Fmt f("%f", 12.34);
  printf("%s\n", f.c_str());
  return 0;
}
