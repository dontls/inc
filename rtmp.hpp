#pragma once

#include "librtmp/log.h"
#include "librtmp/rtmp.h"
#include "flv.hpp"

#define RTMP_HEAD_SIZE (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

namespace librtmp {

class Client {
private:
  RTMP *rtmp_;
  libflv::Packet pkt_;
  long long fts_; // 第一帧时间

public:
  Client(/* args */) : rtmp_(nullptr), pkt_(RTMP_HEAD_SIZE), fts_(0) {}
  ~Client() { this->Close(); }

  void Close() {
    if (rtmp_) {
      RTMP_Close(rtmp_);
      RTMP_Free(rtmp_);
      rtmp_ = nullptr;
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
    printf("%s\n", errMsg);
    return false;
  }

  // 发送RTMP包
  int WritePacket(int rptype, libyte::Buffer *b, uint32_t dts) {
    RTMPPacket *pkt = (RTMPPacket *)b->Bytes();
    pkt->m_body = b->Bytes() + RTMP_HEAD_SIZE;
    pkt->m_nBodySize = b->Len() - RTMP_HEAD_SIZE;
    /*包体内存*/
    pkt->m_hasAbsTimestamp = 0;
    /*此处为类型有两种一种是音频,一种是视频*/
    pkt->m_packetType = rptype;
    pkt->m_nInfoField2 = rtmp_->m_stream_id;
    pkt->m_nChannel = 0x04;

    pkt->m_headerType = RTMP_PACKET_SIZE_LARGE;
    if (RTMP_PACKET_TYPE_AUDIO == rptype) {
      pkt->m_nChannel = 0x05;
      pkt->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    }
    pkt->m_nTimeStamp = dts;
    /*发送*/
    int nRet = 0;
    if (1 == RTMP_IsConnected(rtmp_)) {
      /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
      nRet = RTMP_SendPacket(rtmp_, pkt, 0);
    }
    return nRet;
  }

  // ftype:1 代表i帧
  int WriteFrame(char *frame, size_t len, int ftype, long long ts) {
    if (fts_ == 0) {
      if (ftype != 1) {
        return 0;
      }
      fts_ = ts;
    }
    long long dts = ts - fts_;
    if (ftype == 3) {
      libyte::Buffer *b = pkt_.Marshal(frame, len);
      return WritePacket(RTMP_PACKET_TYPE_AUDIO, b, dts);
    }
    nalu::Units nalus;
    char *data = nalu::Split(frame, len, nalus);
    if (data == nullptr) {
      return 0;
    }
    if (ftype == 1) {
      libyte::Buffer *b = pkt_.Marshal(nalus);
      WritePacket(RTMP_PACKET_TYPE_VIDEO, b, dts);
    }
    libyte::Buffer *b = pkt_.Marshal(data, len, ftype == 1);
    return WritePacket(RTMP_PACKET_TYPE_VIDEO, b, dts);
  }
};

} // namespace librtmp
