#pragma once

#include <cstddef>
#include <stdio.h>
#include <string.h>

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

class FrameFile {
private:
  char *data_;
  FILE *file_;

public:
  FrameFile(/* args */) : file_(NULL) { data_ = new char[READ_BUF_SIZ]; }
  ~FrameFile() {
    if (data_) {
      delete data_;
    }
  }

  bool Open(const char *filename) {
    if (file_) {
      fclose(file_);
      file_ = NULL;
    }
    file_ = fopen(filename, "rb");
    return file_ != NULL;
  }

  char *Read(int &len, int &ftype) {
    if (file_ == NULL) {
      return NULL;
    }
    memset(data_, 0, READ_BUF_SIZ);
    int n = fread(data_, 1, READ_BUF_SIZ, file_);
    if (n <= 0) {
      return NULL;
    }
    int offset = read264frame(data_, n - 5);
    if (offset < 0) {
      return NULL;
    }
    ftype = (data_[4] & 0x1F) == 1 ? 2 : 1;
    // printf("length %d\n", offset);
    len = offset;
    fseek(file_, offset - n, SEEK_CUR);
    return data_;
  }
};
