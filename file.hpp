#pragma once

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#endif
#include <string.h>
#include <functional>

namespace libfile {
#ifdef _WIN32
inline bool IsDir(const char *filename) {
  DWORD dwAttrs = ::GetFileAttributes(filename);
  if (dwAttrs == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

inline size_t Size(const char *filename) {
  HANDLE hFile =
      ::CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    return 0;
  }
  DWORD dwFileSize = ::GetFileSize(hFile, NULL); // 获取文件大小
  ::CloseHandle(hFile);
  return size_t(dwFileSize);
}

inline bool Access(const char *filename, int type = 0) {
  return _access(filename, type) == 0;
}

inline void Walk(const char *path, std::function<void(const char *)> const &f) {
  // Windows 实现
  WIN32_FIND_DATAA findData;
  std::string _path(path);
  HANDLE hFind = FindFirstFileA((_path + "\\*").c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    return; // 错误处理
  }
  while (FindNextFileA(hFind, &findData) != 0) {
    if (strcmp(findData.cFileName, ".") == 0 ||
        strcmp(findData.cFileName, "..") == 0) {
      continue;
    }
    f((const char *)findData.cFileName);
  }
  FindClose(hFind);
}

#else
inline bool IsDir(const char *filename) {
  struct stat statbuf;
  if (stat(filename, &statbuf) != 0) {
    return false;
  }
  return S_ISDIR(statbuf.st_mode) != 0;
}

inline size_t Size(const char *filename) {
  struct stat mystat;
  stat(filename, &mystat);
  return size_t(mystat.st_size);
}

inline bool Access(const char *filename, int type = 0) {
  return access(filename, type) == 0;
}

inline void Walk(const char *path, std::function<void(const char *)> const &f) {
  DIR *dir = opendir(path);
  if (!dir) {
    return;
  }
  struct dirent *entry = NULL;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    f(entry->d_name);
  }
  closedir(dir);
}
#endif
} // namespace libfile
