#include "../rtsp.hpp"
#include "../mp4.hpp"
#include "../faac.hpp"

int main(int argc, char const *argv[]) {
  libfile::Mp4 file("00rtsp.mp4");
  try {
    libfaac::Encoder faac(8000, 1, 16, libfaac::STREAM_RAW);
    librtsp::Client cli(true);
    auto firts = libtime::UnixMilli();
    cli.Play("rtsp://admin:12345@172.16.50.219/test.mp4",
             [&](const char *format, uint8_t ftype, char *data, int len) {
               auto ts = libtime::UnixMilli();
               printf("%s %lld type %d size %d\n", format, ts, ftype, len);
               if (ftype == 3) {
                 data = faac.Encode(data, len, len);
               }
               if (len > 0) {
                 file.Write(ts, ftype, data, len);
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
