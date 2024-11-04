#include "../mp4.hpp"
#include "../fmp4.hpp"
#include "framefile.hpp"

int main(int argc, char const *argv[]) {
  libfmp4::Writer w("1080p.mp4");
  long ts = 100000;
  FrameFile ffile;
  ffile.Open("1080p.h264");
  for (;;) {
    int len, ftype;
    char *b = ffile.Read(len, ftype);
    if (b == nullptr) {
      break;
    }
    // printf("length %d\n", offset);
    if (w.WriteVideo(ts, ftype == 1, b, len) < 0) {
      break;
    }
    ts += 40;
  }
  return 0;
}
