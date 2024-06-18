#include "../string.hpp"
#include "../log.hpp"
#include <iostream>

int main(int argc, char const *argv[]) {
  libstr::Fmt us(".%06d", 1000);
  LogDebug("%s %d", us.c_str(), us.length());
  libstr::Fmt t("%d", 123456);
  std::cout << t.c_str() << "\t" << t.length() << "\n";
  return 0;
}
