#include "../crypto/aes.h"
// #include "../crypto/md5.h"
// #include "../crypto/sha1.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {
#define BUFF_SIZE 256
  unsigned char cipherbuf[256];
  unsigned char plainbuf[256];
  unsigned char plaintext[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";             // our plaintext to encrypt
  unsigned char secret[] = "A SECRET PASSWORD"; // 16 bytes long

  int written = aesEncrypt(cipherbuf, BUFF_SIZE, plaintext, 26, secret);
  printf("%s\n", cipherbuf);
  written = aesDecrypt(plainbuf, 256, cipherbuf, written, secret);
  printf("%s\n", plainbuf);
  return 0;
}
