#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

namespace libproc {

class Single {
public:
  explicit Single(const char *proc) { running(proc); }

private:
  Single(const Single &) = default;
  const Single &operator=(const Single &) = delete;

  void running(const char *proc) {
    char buf[16], fname[100];
    sprintf(fname, "/var/run/%s.pid", proc);
    int fd =
        open(fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0 || flock(fd) < 0) {
      close(fd);
      exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
  }

  /**
   * @description: 对文件进行枷锁
   * @param {type} fd 文件描述符
   * @return:
   */
  int flock(int fd) {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
  }
};
} // namespace libproc