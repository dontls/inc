#include "../mp4.hpp"

#define READ_BUF_SIZ 500000

// 0x67 0x68 0x65 0x41
inline int read264frame(char *b, int len) {
  int offset = -1;
  bool ok = false;
  for (int i = 0; i < len; i++) {
    if (b[i] == 0x00 && b[i + 1] == 0x00 && b[i + 2] == 0x00 &&
        b[i + 3] == 0x01) {
      if (ok) {
        offset = i;
        break;
      }
      char type = b[i + 4] & 0x1F;
      if (type == 1 || type == 5) {
        ok = true;
      }
    }
  }
  return offset;
}

int main(int argc, char const *argv[]) {
  FILE *file = fopen("1080p.h264", "rb");
  if (file == NULL) {
    return -1;
  }
  libmp4::Writer w("1080p.mp4");
  long ts = 100000;
  char *b = new char[READ_BUF_SIZ];
  for (;;) {
    memset(b, 0, READ_BUF_SIZ);
    int n = fread(b, 1, READ_BUF_SIZ, file);
    if (n <= 0) {
      break;
    }
    int offset = read264frame(b, n - 5);
    if (offset < 0) {
      break;
    }
    bool type = (b[4] & 0x1F) == 1 ? false : true;
    // printf("length %d\n", offset);
    w.WriteVideo(ts, type, b, offset);
    fseek(file, offset - n, SEEK_CUR);
    ts += 40;
  }
  return 0;
}
