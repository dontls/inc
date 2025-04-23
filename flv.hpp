#pragma once

#include "buffer.hpp"
#include "sps.hpp"
#include "slice.hpp"

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
  libyte::Buffer *Marshal(nalu::Vector &nalus);
  libyte::Buffer *Marshal(char *data, size_t len, bool key);
  // aac spec info
  libyte::Buffer *Marshal(std::string &s);
  // 音频，数据长度len+2
  libyte::Buffer *Marshal(char *data, size_t len);

  libyte::Slice Pack(int tag, uint32_t ts);
};

// sps/pps/vps
inline libyte::Buffer *Packet::Marshal(nalu::Vector &nalus) {
  buf_.Reset(0);
  bool is264_ = nalu::IsH264(nalus[0].data[0]);
  nalu::Value *sps = nullptr, *pps = nullptr, *vps = nullptr;
  for (auto &it : nalus) {
    if (is264_) {
      switch (it.type & 0x1F) {
      case avc::NALU_TYPE_SPS:
        sps = &it;
        break;
      case avc::NALU_TYPE_PPS:
        pps = &it;
        break;
      default:
        break;
      }
    } else {
      switch ((it.type & 0x7E) >> 1) {
      case hevc::NALU_TYPE_SPS:
        sps = &it;
        break;
      case hevc::NALU_TYPE_PPS:
        pps = &it;
        break;
      case hevc::NALU_TYPE_VPS:
        vps = &it;
        break;
      default:
        break;
      }
    }
  }
  if (sps == nullptr || pps == nullptr) {
    return &buf_;
  }
  char out[512] = {0};
  int i = 0;
  // h264
  if (is264_) {
    out[i++] = 0x17;
    i += 4; // 4个字节0x00
    /*AVCDecoderConfigurationRecord*/
    out[i++] = 0x01;
    out[i++] = sps->data[1];
    out[i++] = sps->data[2];
    out[i++] = sps->data[3];
    out[i++] = 0xff;
    /*sps*/
    out[i++] = 0xe1;
    out[i++] = (sps->size >> 8) & 0xff;
    out[i++] = sps->size & 0xff;
    memcpy(&out[i], sps->data, sps->size);
    i += sps->size;
    /*pps*/
    out[i++] = 0x01;
    out[i++] = (pps->size >> 8) & 0xff;
    out[i++] = (pps->size) & 0xff;
    memcpy(&out[i], pps->data, pps->size);
    i += pps->size;
  } else {
    out[i++] = 0x1C;
    i += 5; // 5个字节的0x00
    // general_profile_idc 8bit
    out[i++] = sps->data[1];
    // general_profile_compatibility_flags 32 bit
    out[i++] = sps->data[2];
    out[i++] = sps->data[3];
    out[i++] = sps->data[4];
    out[i++] = sps->data[5];
    // 48 bit NUll nothing deal in rtmp
    out[i++] = sps->data[6];
    out[i++] = sps->data[7];
    out[i++] = sps->data[8];
    out[i++] = sps->data[9];
    out[i++] = sps->data[10];
    out[i++] = sps->data[11];
    // general_level_idc
    out[i++] = sps->data[12];
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
    out[i++] = (vps->size >> 8) & 0xff;
    out[i++] = (vps->size) & 0xff;
    memcpy(&out[i], vps->data, vps->size);
    i += vps->size;
    // sps
    out[i++] = 0x21; // sps 33
    out[i++] = (1 >> 8) & 0xff;
    out[i++] = 1 & 0xff;
    out[i++] = (sps->size >> 8) & 0xff;
    out[i++] = sps->size & 0xff;
    memcpy(&out[i], sps->data, sps->size);
    i += sps->size;
    // pps
    out[i++] = 0x22; // pps 34
    out[i++] = (1 >> 8) & 0xff;
    out[i++] = 1 & 0xff;
    out[i++] = (pps->size >> 8) & 0xff;
    out[i++] = (pps->size) & 0xff;
    memcpy(&out[i], pps->data, pps->size);
    i += pps->size;
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
  libyte::Slice s{};
  size_t dlen = buf_.Len() - offset_ + 11;
  s.data = buf_.Bytes() + offset_ - 11;
  int i = 0;
  s.data[i++] = tag;
  s.data[i++] = (uint8_t)(dlen >> 16); // data len
  s.data[i++] = (uint8_t)(dlen >> 8);  // data len
  s.data[i++] = (uint8_t)(dlen);       // data len
  s.data[i++] = (uint8_t)(dlen >> 16); // time stamp
  s.data[i++] = (uint8_t)(ts >> 8);    // time stamp
  s.data[i++] = (uint8_t)(ts);         // time stamp
  s.data[i++] = (uint8_t)(ts >> 24);   // time stamp
  s.data[i++] = 0x00;                  // stream id 0
  s.data[i++] = 0x00;                  // stream id 0
  s.data[i++] = 0x00;                  // stream id 0
  buf_.Write(uint32_t(buf_.Len()));
  s.length = dlen + 4;
  return s;
}
} // namespace libflv
