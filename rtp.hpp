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

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PKT_SIZE 1438

struct Packet {
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
  // payload
  uint8_t *data;
  // payload size
  int size;
};

static Packet Unmarshal(uint8_t *b, int len) {
  Packet pkt{};
  pkt.csrcLen = b[0] >> 4;
  pkt.extension = (b[0] >> 3) & 0x01;
  pkt.padding = (b[0] >> 2) & 0x01;
  pkt.version = b[0] & 0x03;
  pkt.payloadType = b[1] & 0x7F;
  pkt.marker = b[1] >> 7;
  pkt.seq = Uint16(&b[2]);
  pkt.timestamp = Uint32(&b[4]);
  pkt.ssrc = Uint32(&b[8]);
  pkt.data = b + RTP_HEADER_SIZE;
  pkt.size = len - RTP_HEADER_SIZE;
  return pkt;
}

namespace h264 {
static uint8_t Unmarshal(std::string &e, Packet *r, libyte::Buffer &b) {
  uint8_t sflag = 0, ntype = r->data[1], flag = r->data[0] & 0x1f;
  if (flag == 28) {
    sflag = (ntype & 0x80) | (ntype & 0x40);
    ntype -= sflag;
    r->data += 2;
    r->size -= 2;
  } else {
    b.Write(_nalu_header, 4);
    ntype = r->data[0];
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
  b.Write((char *)r->data, r->size);
  if (r->marker == 1 && (ntype == 1 || ntype == 5)) {
    return ntype;
  }
  // printf("rtp size %d %d %ld\n", r->size, ntype, b.Len());
  return 0;
}

// static void Marshal(char *data, size_t len,
//                     std::function<void(Packet *)> callrtp) {
//   nalu::Vector nalus;
//   char *ptr = nalu::Split(data, len, nalus);
//   Packet pkt{};
//   for (auto it : nalus) {
//     pkt.marker = 1;
//     pkt.data = (uint8_t *)it.data;
//     pkt.size = it.size;
//     callrtp(&pkt);
//   }
//   uint8_t payload[RTP_MAX_PKT_SIZE + 4] = {};
//   pkt.data = payload;
//   payload[0] = (ptr[0] & 0x60) | 28;
//   int rsize = len, i = 0;
//   while (rsize > RTP_MAX_PKT_SIZE) {
//     pkt.marker = 0;
//     payload[1] = ptr[0] & 0x1F;
//     if (i == 0) {
//       payload[1] |= 0x80;
//     }
//     memcpy((char *)&payload[2], ptr + i, RTP_MAX_PKT_SIZE);
//     pkt.size = RTP_MAX_PKT_SIZE + 2;
//     i += RTP_MAX_PKT_SIZE;
//     rsize -= RTP_MAX_PKT_SIZE;
//     callrtp(&pkt);
//   }
//   pkt.marker = 1;
//   payload[1] = ptr[0] & 0x1F;
//   payload[1] |= 0x40; // end
//   memcpy((char *)&payload[2], ptr + i, rsize);
//   pkt.size = rsize + 2;
//   callrtp(&pkt);
// }

}; // namespace h264

namespace h265 {

// NOTE. sps/vps/pps没有和i帧合并
static uint8_t Unmarshal(std::string &e, Packet *r, libyte::Buffer &b) {
  uint8_t sflag = 0, ntype = 0, flag = r->data[0] >> 1;
  if (flag == 49) {
    ntype = r->data[2];
    sflag = (ntype & 0x80) | (ntype & 0x40);
    ntype -= sflag;
    r->data += 3;
    r->size -= 3;
  } else {
    ntype = r->data[0];
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
  b.Write((char *)r->data, r->size);
  // printf("rtp size %d %ld\n", r->size, b.Len());
  if (r->marker == 1 && (sflag & 0x40) > 0) {
    ntype = (b.Bytes()[4] & 0x7e) >> 1;
    return ntype;
  }
  return 0;
}
} // namespace h265

} // namespace librtp

namespace librtcp {

struct rtcpRb {
  unsigned int ssrc;         /* data source being reported */
  unsigned int fraction : 8; /* fraction lost since last SR/RR */
  int lost : 24;             /* cumul. no. pkts lost (signed!) */
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
  unsigned short length;    /* pkt len in words, w/o this word */
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