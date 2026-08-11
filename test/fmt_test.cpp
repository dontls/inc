#include "../string.hpp"
#include "../log.hpp"
#include <iostream>
#include <string.h>

const char *dlt =
    "% ^DRIVING "
    "LICENSESTESTSMR.^^?;6007641111111111120=180919770412=?249999959 00100";

void parsedlt(const char *_data) {
  char *p0 = (char *)strchr(_data, ';');
  if (p0) {
    p0 += 1;
    char *p1 = strchr(p0, '?');
    if (p1) {
      std::string card(p0, p1 - p0);
      printf("%s\n", card.c_str());
    }
  }
}

int main(int argc, char const *argv[]) {
  parsedlt(dlt);
  libstring::Fmt us(".%06d", 1000);
  LogDebug("%s %d", us.c_str(), us.length());
  libstring::Fmt t("%d", 123456);
  std::cout << t.c_str() << "\t" << t.length() << "\n";
  return 0;
}
