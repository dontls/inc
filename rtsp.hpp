#pragma once
#include "buffer.hpp"
#include "socket.hpp"
#include "time.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <functional>

#define DBG printf
#define AUTHORIZAION 1

#if AUTHORIZAION
#include "crypto/md5.h"
#include "crypto/base64.h"
#endif

namespace librtsp {

inline std::string getItemVar(std::string &str, const char *s, const char *e) {
  auto p0 = str.find(s);
  if (p0 == std::string::npos) {
    return "";
  }
  p0 += strlen(s);
  auto p1 = str.find(e, p0);
  if (p1 == std::string::npos) {
    return "";
  }
  return str.substr(p0, p1 - p0);
}

// rtsp url解析
struct url {
  std::string path;
  std::string user;
  std::string password;
  std::string ip;
  unsigned short port;
  std::string baseAuth;
  std::string realm;
  std::string nonce;
  bool Parse(const char *s);
  void SetAuth(std::string s);
  std::string GetAuth(std::string type);
};

inline bool url::Parse(const char *uri) {
  baseAuth = "";
  if (strncmp(uri, "rtsp://", 7)) {
    return false;
  }
  path = "rtsp://";
  char tmp[1024] = {0};
  strcpy(tmp, uri + 7);
  char *s = tmp;
  char *ptr = strchr(tmp, '@');
  if (ptr) {
    *ptr = '\0';
    // 携带用户名密码
    char *p1 = strchr(s, ':');
    if (p1) {
      *p1 = '\0';
      password = p1 + 1;
    }
    user = s;
    s = ptr + 1;
  }
  path += s;
  ptr = strchr(s, '/');
  if (ptr) {
    *ptr = '\0';
  }
  ptr = strchr(s, ':');
  if (ptr) {
    *ptr = '\0';
    port = std::atoi(ptr + 1);
  } else {
    port = 554;
  }
  ip = s;
  return true;
} // namespace url

// WWW-Authenticate: Digest realm="happytimesoft", nonce="000039B3000054DE"
// Authorization: Digest username="admin", realm="happytimesoft",
// nonce="000001EB000026E9", uri="rtsp://172.16.60.219:554/test.mp4",
// response="382675796381e60b0d7fedf5812ce42d"

inline void url::SetAuth(std::string s) {
  if (s.find("Basic") != std::string::npos) {
    baseAuth = "Authorization: Basic ";
  } else {
    realm = getItemVar(s, "realm=\"", "\"");
    nonce = getItemVar(s, "nonce=\"", "\"");
    baseAuth = "Authorization: Digest ";
    baseAuth += ("username=\"" + user);
    baseAuth += ("\", realm=\"" + realm);
    baseAuth += ("\", nonce=\"" + nonce);
    baseAuth += ("\", uri=\"" + path + "\"");
  }
}

inline std::string url::GetAuth(std::string type) {
  if (!baseAuth.empty()) {
#ifdef AUTHORIZAION
    if (baseAuth.find("Basic") != std::string::npos) {
      return baseAuth + base64_encode(user + ":" + password);
    }
    std::string hex = md5::md5_hash_hex(user + ":" + realm + ":" + password);
    hex += (":" + nonce + ":" + md5::md5_hash_hex(type + ":" + this->path));
    return baseAuth + (", response=\"" + md5::md5_hash_hex(hex) + "\"\r\n");
#endif
  }
  return "";
}

struct sdp {
  std::string session;
  int i;
  struct {
    int format; // 96 /98
    std::string rtpmap;
    std::string sprops;
    std::string id;
  } control[3];
  void Parse(std::vector<std::string> &ss);
};

inline void sdp::Parse(std::vector<std::string> &ss) {
  i = -1;
  for (size_t j = 0; j < ss.size(); j++) {
    std::string &s = ss[j];
    size_t pos = s.find("Session: ");
    if (pos != std::string::npos) {
      session = s.substr(9);
    } else if ((pos = s.find("m=")) != std::string::npos) {
      i++;
    } else if (i >= 0) {
      if ((pos = s.find("a=rtpmap:")) != std::string::npos) {
        std::istringstream sline(s.c_str() + 9);
        sline >> control[i].format >> control[i].rtpmap;
      } else if ((pos = s.find("sprop")) != std::string::npos) {
        control[i].sprops = s.substr(pos);
      } else if ((pos = s.find("a=control:")) != std::string::npos) {
        control[i].id = s.substr(pos + 10);
      }
    }
  }
}

inline uint16_t GetUint16(uint8_t *b) { return uint16_t(b[0]) << 8 | b[1]; }

inline uint32_t GetUint32(uint8_t *b) {
  return uint32_t(b[0]) << 24 | uint32_t(b[0]) << 16 | uint32_t(b[2]) << 8 |
         b[3];
}

static char nalu[] = {0x00, 0x00, 0x00, 0x01};

struct rtp {
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
  uint32_t timestamp;
  /* bytes 8-11 */
  uint32_t ssrc;
  // payload
  uint8_t *data;
  // payload size
  int size;
  void Unmarshal(uint8_t *b, int len);
};

inline void rtp::Unmarshal(uint8_t *b, int len) {
  this->csrcLen = b[0] >> 4;
  this->extension = (b[0] >> 3) & 0x01;
  this->padding = (b[0] >> 2) & 0x01;
  this->version = b[0] & 0x03;
  this->payloadType = b[1] & 0x7F;
  this->marker = b[1] >> 7;
  this->seq = GetUint16(&b[2]);
  this->timestamp = GetUint32(&b[4]);
  this->ssrc = GetUint32(&b[8]);
  this->data = b + 12;
  this->size = len - 12;
}

static uint8_t Unmarshal264(rtp *r, libyte::Buffer &b) {
  uint8_t sflag = 0;
  uint8_t ntype = r->data[1];
  if ((r->data[0] & 0x1f) == 28) {
    sflag = (ntype & 0x80) | (ntype & 0x40);
    ntype -= sflag;
    // 开始或这单独结束包
    if (sflag & 0x80 || b.Empty()) {
      b.Write(nalu, 4);
      b.Write((uint8_t)(ntype | 0x60));
    }
    r->data += 2;
    r->size -= 2;
  } else {
    b.Write(nalu, 4);
    ntype = r->data[0];
  }
  b.Write((char *)r->data, r->size);
  ntype = ntype & 0x1F;
  if (r->marker == 1 && (ntype == 1 || ntype == 5)) {
    sflag = 0x40;
  }
  return sflag;
}

// NOTE. sps/vps/pps没有和i帧合并
static uint8_t Unmarshal265(rtp *r, libyte::Buffer &b) {
  uint8_t sflag = 0;
  uint8_t flag = r->data[0] >> 1;
  uint8_t ntype = 0;
  if (flag == 49) {
    ntype = r->data[2];
    sflag = (ntype & 0x80) | (ntype & 0x40);
    ntype -= sflag;
    // 开始或这单独结束包
    if (sflag & 0x80 || b.Empty()) {
      b.Write(nalu, 4);
      ntype = ntype << 1;
      b.Write(ntype);
      b.Write((uint8_t)0x01);
    }
    r->data += 3;
    r->size -= 3;
  } else {
    ntype = r->data[0];
    b.Write(nalu, 4);
  }
  b.Write((char *)r->data, r->size);
  // printf("%d 0x%02x\n", flag, sflag);
  ntype = (ntype & 0x7e) >> 1;
  if (r->marker == 1 && (ntype == 1 || ntype == 19)) {
    sflag = 0x40;
  }
  return sflag;
}

enum {
  OPTIONS,
  DESCRIBE,
  SETUP,
  PLAY,
  GET_PARAMETER,
  SET_PARAMETER,
  PAUSE,
  ANNOUNCE,
  TEARDOWN,
  SETAUDIO
};

inline const char *Format(int type) {
  switch (type) {
  case OPTIONS:
    return "OPTIONS %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "%s" // Authorization
           "User-Agent: myrtsp\r\n"
           "\r\n";
  case DESCRIBE:
    return "DESCRIBE %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "%s" // Authorization
           "User-Agent: myrtsp\r\n"
           "Accept: application/sdp\r\n"
           "\r\n";
  case SETUP:
    return "SETUP %s/%s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s" // Authorization
           "User-Agent: myrtsp\r\n"
           "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
           "\r\n";
  case PLAY:
    return "PLAY %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s" // Authorization
           "User-Agent: myrtsp\r\n"
           "Range: npt=0.000-\r\n"
           "\r\n";
  case PAUSE:
    return "PAUSE %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s"
           "%s"
           "\r\n";
  case ANNOUNCE:
    return "ANNOUNCE %s RTSP/1.0\r\n"
           "CSeq: %d/r/n"
           "Content-Type: application/sdp\r\n"
           "%s"
           "Content-length: %d\r\n\r\n"
           "%s\r\n";
  case SET_PARAMETER:
    return "SET_PARAMETER %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "User-Agent: myrtsp\r\n"
           "Content-length: %d\r\n\r\n"
           "%s: %s\r\n";
  case GET_PARAMETER:
    return "GET_PARAMETER %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s"
           "User-Agent: myrtsp\r\n"
           "\r\n";
  case TEARDOWN:
    return "TEARDOWN %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "User-Agent: myrtsp\r\n"
           "\r\n";
  default:
    break;
  }
  return "";
}

#define PKG_LEN 2048

class Client : libnet::Conn {
private:
  libyte::Buffer rbuf_;
  libyte::Buffer dbuf_;
  int cmdType_;
  int seq_ = 0;
  sdp sdp_;
  url url_;
  rtp rtp_;
  std::function<uint8_t(rtp *, libyte::Buffer &)> decode_;

public:
  Client(/* args */) {}
  ~Client() { Close(); }

  // rtsp://admin:123456@127.0.0.1:554/test.mp4
  bool Play(const char *sUrl) {
    if (url_.Parse(sUrl) == false) {
      return false;
    }
    try {
      Dial(url_.ip.c_str(), url_.port);
      doWriteCmd(OPTIONS, seq_++, "");
      long ts = libtime::UnixMilli();
      for (;;) {
        char buf[PKG_LEN] = {0};
        int n = Read(buf, PKG_LEN);
        rbuf_.Write(buf, n);
        for (;;) {
          uint8_t *b = (uint8_t *)rbuf_.Bytes();
          int blen = rbuf_.Len() - 4;
          if (blen < 0) {
            break;
          }
          if (b[0] == '$') {
            uint8_t ch = b[1];
            int dlen = GetUint16(&b[2]);
            if (blen < dlen) {
              break;
            }
            rtp_.Unmarshal(b + 4, dlen);
            uint8_t code = 0x40;
            if (rtp_.payloadType == 96) {
              code = decode_(&rtp_, dbuf_);
            } else if (rtp_.payloadType == 72) {
              code = 0;
            } else {
              dbuf_.Write((char *)rtp_.data, rtp_.size);
            }
            if (code & 0x40) {
              DBG("%ld channel %d, %ld\n", ts, ch, dbuf_.Len());
              dbuf_.Reset(0);
            }
            rbuf_.Remove(dlen + 4);
          } else {
            // GET_PARAMETER response
            doRtspParse((char *)b);
          }
        }
        if (libtime::Since(ts) > 50000) {
          ts = libtime::UnixMilli();
          doWriteCmd(GET_PARAMETER, seq_++, sdp_.session.c_str(),
                     url_.GetAuth("GET_PARAMETER").c_str());
        }
      }
    } catch (libnet::Exception &e) {
      DBG("%s\n", e.PrintError());
    }
    return true;
  }

private:
  template <class... Args> void doWriteCmd(int type, Args... args) {
    char buf[PKG_LEN] = {0};
    sprintf(buf, Format(type), url_.path.c_str(), args...);
    DBG("\nwrite --> \n%s", buf);
    Write(buf, strlen(buf));
    cmdType_ = type;
  }

  void doRtspParse(char *b) {
    std::vector<std::string> res;
    char *ptr = b, *ptr1 = nullptr;
    while ((ptr1 = strstr(ptr, "\r\n"))) {
      res.push_back(std::string(ptr, ptr1 - ptr));
      ptr = ptr1 + 2;
    }
    int len = ptr - b;
    b[len - 1] = '\0';
    DBG("%d read --> \n%s", len, b);
    switch (cmdType_) {
    case OPTIONS: {
      auto it = std::find_if(res.begin(), res.end(), [&](std::string &s) {
        return s.find("WWW-Authenticate") != std::string::npos;
      });
      if (it == res.end()) {
        doWriteCmd(DESCRIBE, seq_++, url_.GetAuth("DESCRIBE").c_str());
      } else {
        url_.SetAuth(it->c_str());
        doWriteCmd(OPTIONS, seq_++, url_.GetAuth("OPTIONS").c_str());
      }
      rbuf_.Reset(0);
      return;
    };
    case DESCRIBE: {
      sdp_.Parse(res);
      this->decode_ = Unmarshal264;
      if (sdp_.control[0].rtpmap.find("H265") != std::string::npos) {
        this->decode_ = Unmarshal265;
      }
      doWriteCmd(SETUP, sdp_.control[0].id.c_str(), seq_++,
                 sdp_.session.c_str(), url_.GetAuth("SETUP").c_str());
      if (sdp_.i > 1) {
        cmdType_ = SETAUDIO;
      }
      rbuf_.Reset(0);
      return;
    };
    case SETUP:
      doWriteCmd(PLAY, seq_++, sdp_.session.c_str(),
                 url_.GetAuth("PLAY").c_str());
      rbuf_.Reset(0);
      return;
    case SETAUDIO:
      doWriteCmd(SETUP, sdp_.control[1].id.c_str(), seq_++,
                 sdp_.session.c_str(), url_.GetAuth("SETUP").c_str());
      rbuf_.Reset(0);
      return;
    }
    rbuf_.Remove(len);
  }
};

} // namespace librtsp