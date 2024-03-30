#include "../socket.hpp"

int main(int argc, char const *argv[]) {
  Net::Server listener;
  listener.Listen(3300);
  for (;;) {
    Net::Conn conn = listener.Accept();
  }
  return 0;
}
