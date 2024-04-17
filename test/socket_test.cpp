#include "../socket.hpp"

int main(int argc, char const *argv[]) {
  libnet::Server listener;
  listener.Listen(3300);
  for (;;) {
    libnet::Conn conn = listener.Accept();
  }
  return 0;
}
