#include "../byte.hpp"
#include "../buffer.hpp"
#include <stdio.h>

unsigned short ModBusCRC16(unsigned char *ptr, unsigned char size) {
  unsigned short a, b, tmp, crc16, v;
  crc16 = 0xffff;
  for (a = 0; a < size; a++) {
    crc16 = *ptr ^ crc16;
    for (b = 0; b < 8; b++) {
      tmp = crc16 & 0x0001;
      crc16 = crc16 >> 1;
      if (tmp) {
        crc16 = crc16 ^ 0xa001;
      }
    }
    *ptr++;
  }
  v = ((crc16 & 0x00ff) << 8) | ((crc16 & 0xff00) >> 8);
  return v;
}

#pragma pack(1)
typedef struct tagRs485DownOrder {
  unsigned char cDevAddr;  // 设备地址 0x01
  unsigned char cFuncComd; // 功能代码
  unsigned char cAddressHi;
  unsigned char cAddressLo;
  unsigned char cMessageLenHi;
  unsigned char cMessageLenLo;
  unsigned short usCheck; // 校验
} Rs485DownOrder_t;
#pragma pack()

int main(int argc, char const *argv[]) {
  unsigned char src[13] = {0x11, 0x12, 0x13, 0x17, 0x18, 0x19,
                           0x23, 0x19, 0x12, 0x16, 0x90, 0xA9};
  unsigned char *tmp = src;
  //
  int i = 0;
  u8 c = Bytes::U8(tmp, i);
  u16 s = Bytes::LeU16(tmp, i);
  u32 k = Bytes::LeU32(tmp, i);
  u64 l = Bytes::LeU64(tmp, i);
  printf("0x%x 0x%02x 0x%04x 0x%016x\n", c, s, k, l);

  BytesBuffer binBuffer;
  for (int i = 0; i < 1000; i++) {
    binBuffer.Write((char *)src, sizeof(src));
  }
  printf("%s\n", binBuffer.Bytes());
  printf("Cap: %lu Len: %lu\n", binBuffer.Cap(), binBuffer.Len());
  char readStr[8];
  binBuffer.Read(readStr, 8);
  printf("Cap: %lu Len: %lu\n", binBuffer.Cap(), binBuffer.Len());

  //   binBuffer.WriteString("hello world");
  printf("%s\n", binBuffer.Bytes());
  printf("Cap: %lu Len: %lu\n", binBuffer.Cap(), binBuffer.Len());

  unsigned char sendData[] = {0x01, 0x03, 0x00, 0xCE, 0x00, 0x02};
  for (int i = 0; i < 6; i++) {
    printf("0x%02x ", sendData[i]);
  }
  unsigned char recvData[] = {0x01, 0x03, 0x04, 0x00, 0x00,
                              0x00, 0x00, 0xFA, 0x33};
  printf("modbus send crc 0x%04x\n", ModBusCRC16(sendData, sizeof(sendData)));

  int recvLen = sizeof(recvData);
  unsigned char *pData = recvData;
  // 作CRC校验
  unsigned short recvCrcCode =
      ((pData[recvLen - 2] & 0xff) << 8) | pData[recvLen - 1];
  printf("modbus recv crc 0x%04x\n", recvCrcCode);
  if (recvCrcCode != ModBusCRC16((unsigned char *)pData, recvLen - 2)) {
    printf("don't match\n");
  }
  unsigned char showRetainSensorCmd[] = {0x00, 0xCE, 0x00, 0x02};

  Rs485DownOrder_t DownOrderCmd = {0};
  DownOrderCmd.cDevAddr = 0x01;
  DownOrderCmd.cFuncComd = 0x03;
  DownOrderCmd.cAddressHi = 0x00;
  DownOrderCmd.cAddressLo = 0xCE;
  DownOrderCmd.cMessageLenHi = 0x00;
  DownOrderCmd.cMessageLenLo = 0x02;
  unsigned char *pBuf = (unsigned char *)&DownOrderCmd;
  for (int i = 0; i < sizeof(DownOrderCmd); i++) {
    printf("0x%02x ", pBuf[i]);
  }
  printf("\n");
  DownOrderCmd.usCheck = ModBusCRC16(pBuf, sizeof(showRetainSensorCmd) + 2);
  for (int i = 0; i < sizeof(DownOrderCmd); i++) {
    printf("0x%02x ", pBuf[i]);
  }
  printf("\n");
  return 0;
}
