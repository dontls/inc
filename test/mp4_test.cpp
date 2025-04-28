#include "../mp4.hpp"
#include "../fmp4.hpp"
#include "framefile.hpp"

int main(int argc, char const *argv[]) {
  libfile::FMp4 mp4file("1080p.mp4");
  long ts = 100000;
  libfile::Frame ffile;
  ffile.Open("1080p.h264");
  for (;;) {
    int len, ftype;
    char *b = ffile.Read(len, ftype);
    if (b == nullptr) {
      break;
    }
    // printf("length %d\n", offset);
    if (mp4file.Write(ts, ftype == 1, b, len) < 0) {
      break;
    }
    ts += 40;
  }
  return 0;
}
