#include "../rtsp.hpp"

int main(int argc, char const *argv[]) {
  librtsp::Client cli(true);
  cli.Play("rtsp://admin:admin@172.16.60.219:554/test.mp4",
           [](const char *foramt, char *data, int length) {
             printf("%s length %d\n", foramt, length);
           });
  return 0;
}
