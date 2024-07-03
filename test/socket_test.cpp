#include "../socket.hpp"
#include <cstdio>

int main(int argc, char const *argv[]) {
  // libnet::TcpServer listener;
  // listener.Listen(3300);
  // for (;;) {
  //   libnet::TcpConn conn = listener.Accept();
  // }
  try {
    libnet::TcpConn client;
    client.Dial("127.0.0.1", 12345);
    for (;;) {
      char buf[1024] = {0};
      int n = client.Read(buf, 1024);
      if (n == 0) {
        printf("time out\n");
      }
    }
  } catch (libnet::Exception &e) {
    printf("%s\n", e.PrintError());
  }
  return 0;
}
