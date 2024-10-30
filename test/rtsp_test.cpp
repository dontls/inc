#include "../rtsp.hpp"
#include "../fmp4.hpp"

int main(int argc, char const *argv[]) {
  librtsp::Client cli;
  // FILE *file = fopen("12456.h264", "wb+");
  libfmp4::Writer file("12345.mp4");
  try {
    cli.Play("rtsp://admin:admin@172.16.50.219:554/test.mp4",
             [&](const char *foramt, char *data, int len) {
               printf("%s length %d type 0x%02x\n", foramt, len, data[4]);
               file.WriteVideo(libtime::UnixMilli(), data[4] != 0x61, data,
                               len);
             });
  } catch (libnet::Exception &e) {
    printf("%s\n", e.PrintError());
  }
  file.Close();
  return 0;
}
