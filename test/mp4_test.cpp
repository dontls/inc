#include "../mp4l/muxer.hpp"

int main(int argc, char const *argv[]) {
  mp4::Muxer muxer;
  muxer.Open("test.mp4");
  muxer.WriteVideo(1, 1, NULL, 0);
  muxer.Close();
  return 0;
}
