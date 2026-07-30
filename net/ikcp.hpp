#pragma once
// https://github.com/skywind3000/kcp
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "kcp/ikcp.c"

namespace libkcp {

/* get system time */
inline void itimeofday(long *sec, long *usec) {
#if defined(__unix)
  struct timeval time;
  gettimeofday(&time, NULL);
  if (sec)
    *sec = time.tv_sec;
  if (usec)
    *usec = time.tv_usec;
#else
  static long mode = 0, addsec = 0;
  BOOL retval;
  static IINT64 freq = 1;
  IINT64 qpc;
  if (mode == 0) {
    retval = QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    freq = (freq == 0) ? 1 : freq;
    retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    addsec = (long)time(NULL);
    addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
    mode = 1;
  }
  retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
  retval = retval * 2;
  if (sec)
    *sec = (long)(qpc / freq) + addsec;
  if (usec)
    *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
}

/* get clock in millisecond 64 */
inline IINT64 iclock64(void) {
  long s, u;
  IINT64 value;
  itimeofday(&s, &u);
  value = ((IINT64)s) * 1000 + (u / 1000);
  return value;
}

inline IUINT32 iclock() { return (IUINT32)(iclock64() & 0xfffffffful); }

class Client {

private:
  int sockfd_;
  ikcpcb *kcp_;
  struct sockaddr_in sockaddr_;

private:
  static int udpOutPut(const char *buf, int len, ikcpcb *kcp, void *user) {
    //  printf("使用udpOutPut发送数据\n");
    Client *c = (Client *)user;
    // 发送信息
    int n = sendto(c->socketfd(), buf, len, 0, c->sockAddr(),
                   sizeof(struct sockaddr_in)); // 【】
    if (n >= 0) {
      // 会重复发送，因此牺牲带宽; 24字节的KCP头部
      // printf("udpOutPut-send: c->sockfd_ %d 字节 =%d \n", c->sockfd_, n -
      // 24);
    } else {
      printf("udpOutPut: %d bytes send, error\n", n);
    }
    return n;
  }

public:
  int socketfd() { return sockfd_; }
  struct sockaddr *sockAddr() { return (sockaddr *)&sockaddr_; }

  ~Client() {
    if (kcp_) {
      ikcp_release(kcp_);
    }
    close(sockfd_);
  }

  bool Dial(const char *host, uint16_t port) {
    this->kcp_ = ikcp_create(0x1, (void *)this);
    if (this->kcp_ == NULL) {
      return false;
    }
    this->sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->sockfd_ < 0) {
      return false;
    }
    kcp_->output = Client::udpOutPut;
    ikcp_nodelay(kcp_, 0, 10, 0, 0); //(kcp1, 0, 10, 0, 0); 1, 10, 2, 1
    ikcp_wndsize(kcp_, 128, 128);
    bzero(&sockaddr_, sizeof(sockaddr_));
    // 设置服务器ip、port
    sockaddr_.sin_family = AF_INET;
    sockaddr_.sin_addr.s_addr = inet_addr(host);
    sockaddr_.sin_port = htons(port);
    return true;
  }

  size_t Write(char *buf, size_t length) {
    return ikcp_send(kcp_, buf, length);
  }

  // 毫秒轮询
  size_t Read(char *buf, size_t length) {
    IUINT32 ts = iclock();
    ikcp_update(kcp_, ts);
    unsigned int len = sizeof(struct sockaddr_in);
    // 处理收消息
    int n = recvfrom(sockfd_, buf, length, MSG_DONTWAIT, sockAddr(),
                     (socklen_t *)&len);
    if (n > 0) {
      n = ikcp_input(kcp_, buf, n);
    }
    return n;
  }
};

} // namespace  libkcp
