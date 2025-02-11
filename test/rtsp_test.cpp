#include "../rtsp.hpp"
// #include "../fmp4.hpp"

int main(int argc, char const *argv[]) {
  FILE *file = fopen("12456.h264", "wb+");
  librtsp::Client cli;
  cli.Play("rtsp://172.16.50.223:554/stander/livestream/0/0",
           [&](const char *format, uint8_t ftype, char *data, size_t len) {
             printf("%s type 0x%02x length %ld \n", format, ftype, len);
             fwrite(data, len, 1, file);
           });
  fclose(file);
  return 0;
}
