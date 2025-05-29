#include "../rtsp.hpp"
#include "../mp4.hpp"
#include "../faac.hpp"
#include "../g711.h"
#include "../ffmp4.hpp"
int main(int argc, char const *argv[]) {
  libffmpeg::Mp4 file("00rtsp.mp4");
  try {
    uint16_t pcm[1024] = {};
    librtsp::Client cli(true);
    auto firts = libtime::UnixMilli();
    cli.Play("rtsp://admin:12345@172.16.50.219/test.mp4",
             [&](const char *format, uint8_t ftype, char *data, size_t len) {
               auto ts = libtime::UnixMilli();
               //  printf("%s %lld type %d size %lu\n", format, ts, ftype, len);
               if (ftype == 3) {
                 for (size_t i = 0; i < len; i++) {
                   pcm[i] = ulaw_to_linear(data[i]);
                 }
                 len *= 2;
               }
               if (len > 0) {
                 file.WriteFrame(ts, ftype, data, len);
               }
               if (ts - firts > 5000) {
                 cli.Stop();
               }
             });
  } catch (libnet::Exception) {
  }
  file.Close();
  return 0;
}
