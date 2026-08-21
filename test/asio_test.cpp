// 演示：一个 TcpServer + 多个 TcpClient 共享同一个 Context
// 编译（在 test 目录下）：
//   g++ -I../ -I../asio-1.30.2/include -g asio_test.cpp -o asio_test -pthread
#include "net/asio.hpp"
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
  ctx->OnMessage = ([&](libtcp::ClientPtr, char *data, size_t n) -> int {
    if (n < 32) {
      return 0;
    }
    DVRHeader *h = (DVRHeader *)(data + 8);
    int dlen = h->nLength + 32;
    if (n < dlen) {
      return 0;
    }
    if (h->nType == 1) {
      printf("%lld channel %d type %d length %d\n", h->lFrameTimeStamp,
             h->nChannel, h->nType, h->nLength);
    }
    return dlen;
  });
  ctx->OnClose = ([](libtcp::ClientPtr self, const std::error_code &ec) {
    printf("client closed: %s\n", ec.message().c_str());
    self->Reconnect(1000);
  });
  ctx->RunAsync(); // 事件循环在独立线程中运行
  // Keep the client alive; TcpClient owns the connection and its destructor
  // closes it.  Discarding Dial()'s return value closes it immediately.
  for (int i = 0; i < 4; i++) {
    ctx->Dial("172.16.50.67", 5677, i + 1);
  }

  for (;;) {
  }
}
