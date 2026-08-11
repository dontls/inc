// 演示：一个 TcpServer + 多个 TcpClient 共享同一个 Context
// 编译（在 test 目录下）：
//   g++ -I../ -g tcp_test.cpp -o tcp_test -pthread
#include "net/tcp.hpp"
#include "string.hpp"
#include <cstdio>
#include <string>

struct DVRHeader {
  long long lFrameTimeStamp;
  unsigned int nType;
  unsigned int nChannel;
  int nLength;
};

int main() {
  auto ctx = std::make_shared<libtcp::Context>();
  ctx->OnConnect = ([=](libtcp::ClientPtr self) {
    printf("client connected\n");
    const char *playDVR =
        "<message><head><version>1.0.1</version><id>1001</id></"
        "head><body><chn>%d</"
        "chn><type>0</type><stream>0</stream><action>1</action><link>0</"
        "link><addr></addr><port></port></body></message>";
    std::string msg = libstring::Sprintf(playDVR, self->Id());
    self->Send(msg);
  });
  ctx->OnMessage = ([&](libtcp::ConnPtr, libyte::Buffer &b) -> int {
    if (b.Len() < 32) {
      return 0;
    }
    DVRHeader *h = (DVRHeader *)(b.Bytes() + 8);
    int dlen = h->nLength + 32;
    if (b.Len() < dlen) {
      return 0;
    }
    if (h->nType == 1) {
      printf("%lld channel %d type %d length %d\n", h->lFrameTimeStamp,
             h->nChannel, h->nType, h->nLength);
    }
    return dlen;
  });
  ctx->OnClose = ([](libtcp::ClientPtr self) {
    printf("client closed\n");
    self->Reconnect(100);
  });
  ctx->RunAsync(); // 事件循环在独立线程中运行
  // 客户端由 Context 内部按 id 管理：丢弃返回值也保持连接存活并支持自动重连，
  // 随 Context 销毁统一回收；也可保留返回值主动 Send / Close / ReleaseClient。
  for (int i = 0; i < 4; i++) {
    ctx->Dial("172.16.50.67", 5677, i + 1);
  }

  for (;;) {
  }
}
