#include "../rtsp.hpp"
#include "../fmp4.hpp"

int main(int argc, char const *argv[]) {
  librtsp::Client cli;
  // FILE *file = fopen("12456.h264", "wb+");
  libfmp4::Writer file("12345.mp4");
  try {
    cli.Play("rtsp://172.16.50.222:554/stander/livestream/0/0",
             [&](const char *foramt, uint8_t ftype, char *data, int len) {
               printf("%s length %d type 0x%02x\n", foramt, len, ftype);
               file.WriteVideo(libtime::UnixMilli(), data[4] != 0x61, data,
                               len);
             });
  } catch (libnet::Exception &e) {
    printf("%s\n", e.PrintError());
  }
  file.Close();
  return 0;
}
