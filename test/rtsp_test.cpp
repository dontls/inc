#include "../rtsp.hpp"

int main(int argc, char const *argv[]) {
  librtsp::client cli;
  cli.Play("rtsp://admin:admin@172.16.60.219:554/test.mp4");
  return 0;
}
