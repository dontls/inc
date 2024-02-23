#ifndef __SINGLEPROC_H__
#define __SINGLEPROC_H__

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
class CSingleProc {
 public:
  CSingleProc(std::string procname) { procExist(procname); }

 private:
  CSingleProc(const CSingleProc&) = default;
  const CSingleProc& operator=(const CSingleProc&) = delete;

  void procExist(std::string& procname) {
    int fd;
    char buf[16], fname[100];
    sprintf(fname, "/var/run/%s.pid", procname.c_str());
    fd = open(fname, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (fd < 0) {
      printf("open file \"%s\" failed!!!\n", fname);
      return;
    }

    if (flock(fd) == -1) {
      printf("flock file \"%s\" failed!!!\n", fname);
      close(fd);
      exit(-1);
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
#endif