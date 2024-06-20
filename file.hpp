#pragma once

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace libfile {
#ifdef _WIN32
inline bool IsDir(const char *path) {
  DWORD dwAttrs = ::GetFileAttributes(path);
  if (dwAttrs == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

inline size_t Size(const char *filename) {
  HANDLE hFile =
      CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  DWORD dwFileSize = GetFileSize(hFile, NULL); // 获取文件大小
  return size_t(dwFileSize);
}

#else
inline bool IsDir(const char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0) {
    return false;
  }
  return S_ISDIR(statbuf.st_mode) != 0;
}

inline size_t Size(const char *filename) {
  struct stat mystat;
  stat(filename, &mystat);
  return size_t(mystat.st_size);
}
#endif
} // namespace libfile
