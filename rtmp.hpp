#pragma once

#include "librtmp/log.h"
#include "librtmp/rtmp.h"
#include "sps.hpp"
#include "buffer.hpp"
#include "log.hpp"

namespace librtmp {

#define RTMP_HEAD_SIZE (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

inline int Marshal(char *out, nalu::Vector &nalus, bool ish264) {
  nalu::Value *sps = nullptr, *pps = nullptr, *vps = nullptr;
  for (auto &it : nalus) {
    if (ish264) {
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
    return 0;
  }
  int i = 0;
  // h264
  if (ish264) {
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
  return i;
}
// 媒体数据，数据长度len+9
inline int Marshal(char *out, char *data, size_t len, int type,
                   bool ish264 = true) {
  int i = 0;
  if (ish264) {
    out[i++] = type == 1 ? 0x17 : 0x27; // NALU size
  } else {
    out[i++] = type == 1 ? 0x1C : 0x2C; //
  }
  out[i++] = 0x01; // AVC NALU
  i += 3;          // 3个字节的0x00
  out[i++] = len >> 24 & 0xff;
  out[i++] = len >> 16 & 0xff;
  out[i++] = len >> 8 & 0xff;
  out[i++] = len & 0xff;
  // NALU data
  memcpy(&out[i], data, len);
  return len + i;
}

// 音频，数据长度len+2
inline int Marshal(char *out, char *data, size_t len, bool ispecial = false) {
  int i = 0;
  out[i++] = 0xAF;
  out[i++] = ispecial ? 0x00 : 0x01;
  memcpy(&out[i], data, len);
  return i + len;
}
class Client {
private:
  RTMP *rtmp_;
  bool wKey_;
  bool ish264_;
  long firsts_; // 第一帧数据时间戳
  libyte::Buffer sBuf_;

private:
  // 发送RTMP包
  int SendPacket(RTMPPacket *pkt, unsigned int nPktType, unsigned int size,
                 uint32_t ts) {
    /*包体内存*/
    pkt->m_nBodySize = size;
    pkt->m_hasAbsTimestamp = 0;
    /*此处为类型有两种一种是音频,一种是视频*/
    pkt->m_packetType = nPktType;
    pkt->m_nInfoField2 = rtmp_->m_stream_id;
    pkt->m_nChannel = 0x04;

    pkt->m_headerType = RTMP_PACKET_SIZE_LARGE;
    if (RTMP_PACKET_TYPE_AUDIO == nPktType) {
      pkt->m_nChannel = 0x05;
      pkt->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    }
    pkt->m_nTimeStamp = ts;
    /*发送*/
    int nRet = 0;
    if (1 == RTMP_IsConnected(rtmp_)) {
      /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
      nRet = RTMP_SendPacket(rtmp_, pkt, 0);
    }
    return nRet;
  }

  size_t WriteMeta(nalu::Vector &nalus, long pts) {
    char buf[RTMP_HEAD_SIZE + 1024] = {0};
    RTMPPacket *pkt = (RTMPPacket *)buf;
    pkt->m_body = (char *)pkt + RTMP_HEAD_SIZE;
    int n = Marshal(pkt->m_body, nalus, ish264_);
    return SendPacket(pkt, RTMP_PACKET_TYPE_VIDEO, n, pts);
  }

public:
  Client(/* args */) : rtmp_(nullptr), wKey_(true), ish264_(true) {}
  ~Client() {
    if (rtmp_) {
      RTMP_Close(rtmp_);
      RTMP_Free(rtmp_);
    }
  }

  bool Dial(const char *url) {
    // 不显示打印日志
    RTMP_LogSetLevel(RTMP_LOGCRIT);
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
    const char *errMsg = "";
    // set connection timeout,default 30s
    if (!RTMP_SetupURL(rtmp_, (char *)url)) {
      errMsg = "SetupURL Error";
      goto Error;
    }
    //
    RTMP_EnableWrite(rtmp_);
    if (0 == RTMP_Connect(rtmp_, NULL)) {
      errMsg = "Connect Server Error";
      goto Error;
    }
    // 连接流
    if (0 == RTMP_ConnectStream(rtmp_, 0)) {
      errMsg = "Connect Stream Error";
      RTMP_Close(rtmp_);
      goto Error;
    }
    return true;
  Error:
    RTMP_Free(rtmp_);
    rtmp_ = NULL;
    LogError(1, "%s", errMsg);
    return false;
  }

  // type:1 代表i帧
  int WriteVideo(char *data, size_t len, int type, long pts) {
    nalu::Vector nalus;
    char *b = nalu::Split(data, len, nalus);
    if (b == nullptr) {
      return 0;
    }
    if (wKey_ && type == 1) {
      wKey_ = false;
      ish264_ = nalu::IsH264(b[0]);
      firsts_ = pts;
    }
    if (wKey_) {
      return 0;
    }
    pts = pts - firsts_;
    if (type == 1) {
      WriteMeta(nalus, pts);
    }
    sBuf_.Realloc(len + 9 + RTMP_HEAD_SIZE);
    RTMPPacket *pkt = (RTMPPacket *)sBuf_.Bytes();
    pkt->m_body = (char *)pkt + RTMP_HEAD_SIZE;
    int n = Marshal(pkt->m_body, b, len, type, ish264_);
    return SendPacket(pkt, RTMP_PACKET_TYPE_VIDEO, n, pts);
  }

  int WriteAAC(char *data, size_t len, long pts, bool ispecial = false) {
    if (wKey_) {
      return 0;
    }
    pts = pts - firsts_;
    sBuf_.Realloc(len + 2 + RTMP_HEAD_SIZE);
    RTMPPacket *pkt = (RTMPPacket *)sBuf_.Bytes();
    pkt->m_body = (char *)pkt + RTMP_HEAD_SIZE;
    int n = Marshal(pkt->m_body, data, len, ispecial);
    return SendPacket(pkt, RTMP_PACKET_TYPE_AUDIO, n, pts);
  }
};

} // namespace librtmp
