#pragma once
#include <cstdint>
#include "buffer.hpp"

inline uint16_t Uint16(uint8_t *b) { return uint16_t(b[0]) << 8 | b[1]; }

inline uint32_t Uint32(uint8_t *b) {
  return uint32_t(b[0]) << 24 | uint32_t(b[0]) << 16 | uint32_t(b[2]) << 8 |
         b[3];
}

static char _nalu_header[] = {0x00, 0x00, 0x00, 0x01};

namespace librtp {

struct Header {
  /* byte 0 */
  uint8_t csrcLen : 4;
  uint8_t extension : 1;
  uint8_t padding : 1;
  uint8_t version : 2;
  /* byte 1 */
  uint8_t payloadType : 7;
  uint8_t marker : 1;
  /* bytes 2,3 */
  uint16_t seq;
  /* bytes 4-7 */
  unsigned int timestamp;
  /* bytes 8-11 */
  uint32_t ssrc;
};

#define RTP_HEADER_SIZE 12
#define RTP_MAX_SIZE 1438

class Packet {
private:
  Header hr_ = {};

public:
  // payload
  uint8_t *data;
  // payload size
  int size;

private:
  uint8_t UnmarshalAVC(std::string &e, libyte::Buffer &b) {
    uint8_t sflag = 0, ntype = data[1], flag = data[0] & 0x1f;
    if (flag == 28) {
      sflag = (ntype & 0x80) | (ntype & 0x40);
      ntype -= sflag;
      data += 2;
      size -= 2;
    } else {
      b.Write(_nalu_header, 4);
      ntype = data[0];
    }
    ntype = ntype & 0x1F;
    // 开始或这单独结束包
    if (flag == 28 && sflag & 0x80) {
      // rtp中无sps
      if (ntype == 5 && b.Empty()) {
        b.Write(e);
      }
      b.Write(_nalu_header, 4);
      b.WriteByte(ntype | 0x60);
    }
    b.Write((char *)data, size);
    if (hr_.marker == 1 && (ntype == 1 || ntype == 5)) {
      return ntype;
    }
    // printf("rtp size %d %d %ld\n", r->size, ntype, b.Len());
    return 0;
  }
  // NOTE. sps/vps/pps没有和i帧合并
  uint8_t UnmarshalHVC(std::string &e, libyte::Buffer &b) {
    uint8_t sflag = 0, ntype = 0, flag = data[0] >> 1;
    if (flag == 49) {
      ntype = data[2];
      sflag = (ntype & 0x80) | (ntype & 0x40);
      ntype -= sflag;
      data += 3;
      size -= 3;
    } else {
      ntype = data[0];
      b.Write(_nalu_header, 4);
    }
    // 开始或这单独结束包
    if (flag == 49 && sflag & 0x80) {
      ntype = ntype << 1;
      uint8_t t = (ntype & 0x7e) >> 1;
      if (t == 19 && b.Empty()) {
        b.Write(e);
      }
      b.Write(_nalu_header, 4);
      b.WriteByte(ntype);
      b.WriteByte(0x01);
    }
    b.Write((char *)data, size);
    // printf("rtp size %d %ld\n", r->size, b.Len());
    if (hr_.marker == 1 && (sflag & 0x40) > 0) {
      ntype = (b.Bytes()[4] & 0x7e) >> 1;
      return ntype;
    }
    return 0;
  }

public:
  uint8_t PayloadType() { return hr_.payloadType; }

  Packet *Unmarshal(uint8_t *b, int len) {
    hr_.csrcLen = b[0] >> 4;
    hr_.extension = (b[0] >> 3) & 0x01;
    hr_.padding = (b[0] >> 2) & 0x01;
    hr_.version = b[0] & 0x03;
    hr_.payloadType = b[1] & 0x7F;
    hr_.marker = b[1] >> 7;
    hr_.seq = Uint16(&b[2]);
    hr_.timestamp = Uint32(&b[4]);
    hr_.ssrc = Uint32(&b[8]);
    data = b + RTP_HEADER_SIZE;
    size = len - RTP_HEADER_SIZE;
    return this;
  }

  uint8_t Decode(std::string &e, libyte::Buffer &b, bool bHevc) {
    return bHevc ? UnmarshalHVC(e, b) : UnmarshalAVC(e, b);
  }
};

} // namespace librtp

namespace librtcp {

struct rtcpRb {
  unsigned int ssrc;         /* data source being reported */
  unsigned int fraction : 8; /* fraction lost since last SR/RR */
  int lost : 24;             /* cumul. no.  lost (signed!) */
  unsigned int last_seq;     /* extended last seq. no. received */
  unsigned int jitter;       /* interarrival jitter */
  unsigned int lsr;          /* last SR packet from this source */
  unsigned int dlsr;         /* delay since last SR packet */
};

struct Packet {
  unsigned int version : 2; /* protocol version */
  unsigned int p : 1;       /* padding flag */
  unsigned int count : 5;   /* varies by packet type */
  unsigned int pt : 8;      /* RTCP packet type
  abbrev.  name                 value
  SR       sender report          200
  RR       receiver report        201
  SDES     source description     202
  BYE      goodbye                203
  APP      application-defined    204*/
  unsigned short length;    /* len in words, w/o this word */
  union {
    /* sender report (SR) */
    struct {
      unsigned int ssrc;     /* sender generating this report */
      unsigned int ntp_sec;  // NTP timestamp   <--| These are the only
      unsigned int ntp_frac; //                 <--| additional 20-bytes
      unsigned int rtp_ts;   // RTP timestamp   <--| fields besides
      unsigned int psent;    // packets sent    <--| the RR
      unsigned int osent;    // octets sent     <--| header.
      struct rtcpRb rb;      /* variable-length list */
    } sr;

    /* reception report (RR) */
    struct {
      unsigned int ssrc; /* receiver generating this report */
      struct rtcpRb rb;  /* variable-length list */
    } rr;
    /* BYE */
    struct {
      unsigned int ssrc; /* list of sources */

      // OPTIONAL reason for leaving and length of reason in bytes
      unsigned int length : 8;
      unsigned char reason[256];
    } bye;
  } r;
};
} // namespace librtcp