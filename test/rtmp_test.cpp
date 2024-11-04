#include "../rtmp.hpp"
#include "framefile.hpp"
// 0x67 0x68 0x65 0x41

int main(int argc, char const *argv[]) {
  FrameFile ffile;
  bool ok = ffile.Open("1080p.h264");
  if (!ok) {
    return -1;
  }
  long ts = 100000;
  librtmp::Client cli;
  ok = cli.Dial("rtmp://172.16.50.219:1935/live/0");
  while (ok) {
    int len, ftype;
    char *b = ffile.Read(len, ftype);
    if (b == nullptr) {
      ffile.Open("1080p.h264");
      continue;
    }
    cli.WriteVideo(b, len, ftype, ts);
    ts += 40;
    libtime::Sleep(40);
  }
  return 0;
}
