#include "../rtsp.hpp"
int main(int argc, char const *argv[]) {
  try {
    librtsp::Client cli(true);
    auto firts = libtime::UnixMilli();
    cli.Play("rtsp://admin:hxzy1234@192.168.1.220:554/live/ch00_0",
             [&](const char *format, uint8_t ftype, char *data, size_t len) {
               auto ts = libtime::UnixMilli();
               printf("%s %lld type %d size %lu\n", format, ts, ftype, len);
               return ts - firts > 10000 ? -1 : 0;
             });
  } catch (libnet::Exception) {
  }
  return 0;
}
