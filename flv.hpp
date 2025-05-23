#pragma once

#include "buffer.hpp"
#include "sps.hpp"
#include "byte.hpp"

namespace libflv {

const int HeadLength = 11;

class Packet {
private:
  bool is264_;
  int offset_;
  libyte::Buffer buf_;

public:
  Packet(int off) : is264_(true), offset_(off) {}
  ~Packet() {}
  // 媒体数据，数据长度len+9
  libyte::Buffer *Marshal(nalu::Units &nalus);
  libyte::Buffer *Marshal(char *data, size_t len, bool key);
  // aac spec info
  libyte::Buffer *Marshal(std::string &s);
  // 音频，数据长度len+2
  libyte::Buffer *Marshal(char *data, size_t len);

  libyte::Slice Pack(int tag, uint32_t ts);
};

// sps/pps/vps
inline libyte::Buffer *Packet::Marshal(nalu::Units &nalus) {
  buf_.Reset(0);
  nalu::Units units = nalu::Sort(nalus);
  is264_ = units.size() == 2;
  char out[512] = {0};
  int i = 0;
  // h264
  if (is264_) {
    out[i++] = 0x17;
    i += 4; // 4个字节0x00
    /*AVCDecoderConfigurationRecord*/
    out[i++] = 0x01;
    out[i++] = units[0].data[1];
    out[i++] = units[0].data[2];
    out[i++] = units[0].data[3];
    out[i++] = 0xff;
    /*sps*/
    out[i++] = 0xe1;
    out[i++] = (units[0].size >> 8) & 0xff;
    out[i++] = units[0].size & 0xff;
    memcpy(&out[i], units[0].data, units[0].size);
    i += units[0].size;
    /*pps*/
    out[i++] = 0x01;
    out[i++] = (units[1].size >> 8) & 0xff;
    out[i++] = (units[1].size) & 0xff;
    memcpy(&out[i], units[1].data, units[1].size);
    i += units[1].size;
  } else {
    out[i++] = 0x1C;
    i += 5; // 5个字节的0x00
    // general_profile_idc 8bit
    out[i++] = units[0].data[1];
    // general_profile_compatibility_flags 32 bit
    out[i++] = units[0].data[2];
    out[i++] = units[0].data[3];
    out[i++] = units[0].data[4];
    out[i++] = units[0].data[5];
    // 48 bit NUll nothing deal in rtmp
    out[i++] = units[0].data[6];
    out[i++] = units[0].data[7];
    out[i++] = units[0].data[8];
    out[i++] = units[0].data[9];
    out[i++] = units[0].data[10];
    out[i++] = units[0].data[11];
    // general_level_idc
    out[i++] = units[0].data[12];
    // 48 bit NUll nothing deal in rtmp
    i += 6;
    // bit(16) avgFrameRate;
    i += 2;
    /* bit(2) constantFrameRate; */
    /* bit(3) numTemporalLayers; */
    /* bit(1) temporalIdNested; */
    i += 1;
    /* unsigned int(8) numOfArrays; 03 */
    out[i++] = 0x03;
    // vps 32
    out[i++] = 0x20;
    out[i++] = (1 >> 8) & 0xff;
    out[i++] = 1 & 0xff;
    out[i++] = (units[3].size >> 8) & 0xff;
    out[i++] = (units[3].size) & 0xff;
    memcpy(&out[i], units[3].data, units[3].size);
    i += units[3].size;
    // sps
    out[i++] = 0x21; // sps 33
    out[i++] = (1 >> 8) & 0xff;
    out[i++] = 1 & 0xff;
    out[i++] = (units[0].size >> 8) & 0xff;
    out[i++] = units[0].size & 0xff;
    memcpy(&out[i], units[0].data, units[0].size);
    i += units[0].size;
    // pps
    out[i++] = 0x22; // pps 34
    out[i++] = (1 >> 8) & 0xff;
    out[i++] = 1 & 0xff;
    out[i++] = (units[1].size >> 8) & 0xff;
    out[i++] = (units[1].size) & 0xff;
    memcpy(&out[i], units[1].data, units[1].size);
    i += units[1].size;
  }
  buf_.Reset(i + offset_);
  memcpy(buf_.Bytes() + offset_, out, i);
  return &buf_;
}

// 媒体数据，数据长度len+9
inline libyte::Buffer *Packet::Marshal(char *data, size_t len, bool key) {
  buf_.Reset(offset_ + 9 + len);
  char *out = buf_.Bytes() + offset_;
  int i = 0;
  if (is264_) {
    out[i++] = key ? 0x17 : 0x27; // NALU size
  } else {
    out[i++] = key ? 0x1C : 0x2C; //
  }
  out[i++] = 0x01; // AVC NALU
  i += 3;          // 3个字节的0x00
  out[i++] = len >> 24 & 0xff;
  out[i++] = len >> 16 & 0xff;
  out[i++] = len >> 8 & 0xff;
  out[i++] = len & 0xff;
  // NALU data
  memcpy(&out[i], data, len);
  return &buf_;
}

// 音频，数据长度len+2
inline libyte::Buffer *Packet::Marshal(char *data, size_t len) {
  buf_.Reset(offset_ + 2 + len);
  char *out = buf_.Bytes() + offset_;
  int i = 0;
  out[i++] = 0xAF;
  out[i++] = (len == 2) ? 0x00 : 0x01;
  memcpy(&out[i], data, len);
  return &buf_;
}

// 音频，数据长度len+2
inline libyte::Buffer *Packet::Marshal(std::string &s) {
  buf_.Reset(offset_ + 2 + s.length());
  char *out = buf_.Bytes() + offset_;
  int i = 0;
  out[i++] = 0xAF;
  out[i++] = 0x00;
  memcpy(&out[i], s.c_str(), s.length());
  return &buf_;
}

inline libyte::Slice Packet::Pack(int tag, uint32_t ts) {
  size_t dlen = buf_.Len() - offset_ + 11;
  libyte::Slice s(buf_.Bytes(), offset_ - 11);
  s.AppendU8(tag);
  s.AppendU8(dlen >> 16); // data len
  s.AppendU8(dlen >> 8);  // data len
  s.AppendU8(dlen);       // data len
  s.AppendU8(ts >> 16);   // time stamp
  s.AppendU8(ts >> 8);    // time stamp
  s.AppendU8(ts);         // time stamp
  s.AppendU8(ts >> 24);   // time stamp
  s.AppendU8(0x00);       // stream id 0
  s.AppendU8(0x00);       // stream id 0
  s.AppendU8(0x00);       // stream id 0
  s.AppendLeU32(uint32_t(buf_.Len()));
  return s;
}
} // namespace libflv
