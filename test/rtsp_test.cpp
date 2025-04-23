#include "../rtsp.hpp"
// #include "../fmp4.hpp"

int main(int argc, char const *argv[]) {
  FILE *file = fopen("12456.264", "wb+");
  librtsp::Client cli;
  cli.Play("rtsp://172.16.50.225:554/stander/livestream/0/0",
           [=](const char *format, uint8_t ftype, libyte::Buffer &b) {
             printf("%s type %d size %ld\n", format, ftype, b.Len());
             fwrite(b.Bytes(), b.Len(), 1, file);
           });
  fclose(file);
  return 0;
}
