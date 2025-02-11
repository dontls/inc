#include "../rtsp.hpp"
#include "../fmp4.hpp"

int main(int argc, char const *argv[]) {
  // FILE *file = fopen("12456.h264", "wb+");
  libfmp4::Writer file("12345.mp4");
  librtsp::Client cli;
  cli.OnFrame = [&](const char *foramt, uint8_t ftype, char *data, size_t len) {
    printf("%s type 0x%02x length %ld \n", foramt, ftype, len);
    //  file.WriteVideo(libtime::UnixMilli(), ftype ==1, data,
    //                  len);
  };
  cli.Play("rtsp://172.16.50.223:554/stander/livestream/0/0");
  file.Close();
  return 0;
}
