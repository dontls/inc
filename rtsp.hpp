#pragma once
#include "buffer.hpp"
#include "socket.hpp"
#include "time.hpp"
#include "crypto/base64.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

#define LibRtspDebug printf
#define SUPPORT_DIGEST 1
#define USER_AGENT "github/dontls"

#if SUPPORT_DIGEST
#include "crypto/md5.h"
#endif

namespace librtsp {

static char _nalu_header[] = {0x00, 0x00, 0x00, 0x01};

inline std::string findVar(std::string &str, const char *s, const char *e) {
  auto p0 = str.find(s);
  if (p0 == std::string::npos) {
    return "";
  }
  p0 += strlen(s);
  auto p1 = str.find(e, p0);
  if (p1 == std::string::npos) {
    return str.substr(p0);
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
  std::string digestHex;
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
    baseAuth =
        "Authorization: Basic " + base64_encode(user + ":" + password) + "\r\n";
  } else if (s.find("Digest") != std::string::npos) {
    std::string realm = findVar(s, "realm=\"", "\"");
    std::string nonce = findVar(s, "nonce=\"", "\"");
    baseAuth = "Authorization: Digest ";
    baseAuth += ("username=\"" + user);
    baseAuth += ("\", realm=\"" + realm);
    baseAuth += ("\", nonce=\"" + nonce);
    baseAuth += ("\", uri=\"" + path + "\"");
#ifdef SUPPORT_DIGEST
    digestHex = md5::md5_hash_hex(user + ":" + realm + ":" + password) + ":" +
                nonce + ":";
#endif
  }
}

inline std::string url::GetAuth(std::string type) {
  if (baseAuth.find("Basic") != std::string::npos) {
    return baseAuth;
  } else if (baseAuth.find("Digest") != std::string::npos) {
#ifdef SUPPORT_DIGEST
    std::string s = digestHex + md5::md5_hash_hex(type + ":" + this->path);
    return baseAuth + (", response=\"" + md5::md5_hash_hex(s) + "\"\r\n");
#endif
  }
  return "";
}

struct sdp {
  std::string session;
  struct media {
    int format; // 96 /98
    std::string name;
    std::string rtpmap;
    std::string sprops;
    std::string id;
  };
  std::string spsvalue;
  std::vector<media> medias;
  std::map<int, std::string> formats;
  void Parse(std::vector<std::string> &ss);
};

inline void sdp::Parse(std::vector<std::string> &ss) {
  media *it = nullptr;
  for (size_t j = 0; j < ss.size(); j++) {
    std::string &s = ss[j];
    size_t pos = s.find("Session: ");
    if (pos != std::string::npos) {
      session = s.substr(9);
    } else if ((pos = s.find("m=")) != std::string::npos) {
      medias.resize(medias.size() + 1);
      it = &medias.back();
      it->name = std::string(s.c_str() + 2);
    } else if (it != nullptr) {
      if ((pos = s.find("a=rtpmap:")) != std::string::npos) {
        std::istringstream sline(s.c_str() + 9);
        sline >> it->format >> it->rtpmap;
        if ((pos = it->rtpmap.find_first_of("/")) != std::string::npos) {
          formats[it->format] = it->rtpmap.substr(0, pos);
        }
      } else if ((pos = s.find("sprop")) != std::string::npos) {
        it->sprops = s.substr(pos);
      } else if ((pos = s.find("a=control:")) != std::string::npos) {
        it->id = s.substr(pos + 10);
      }
    }
  }
  auto m = std::find_if(medias.begin(), medias.end(), [](sdp::media &m) {
    return m.id.find("video") != std::string::npos;
  });
  if (m == medias.end()) {
    return;
  }
  std::string sps, pps, vps;
  if (m->rtpmap.find("H264") != std::string::npos) {
    // sprop-parameter-sets=
    sps = findVar(m->sprops, "sprop-parameter-sets=", ",");
    pps = findVar(m->sprops, ",", ",");
  } else {
    // sprop-vps=QAEMAf//IWAAAAMAAAMAAAMAAAMAlqwJ;sprop-sps=QgEBIWAAAAMAAAMAAAMAAAMAlqADwIARB8u605KJLuagQEBAgAg9YADN/mAE;sprop-pps=RAHAcvAbJA==
    vps = findVar(m->sprops, "vps=", ";");
    sps = findVar(m->sprops, "sps=", ";");
    pps = findVar(m->sprops, "pps=", ";");
  }
  if (sps.empty()) {
    return;
  }
  spsvalue = std::string(_nalu_header, 4);
  spsvalue.append(base64_decode(sps));
  spsvalue.append(_nalu_header, 4);
  spsvalue.append(base64_decode(pps));
  if (!vps.empty()) {
    spsvalue.append(_nalu_header, 4);
    spsvalue.append(base64_decode(vps));
  }
}

inline uint16_t GetUint16(uint8_t *b) { return uint16_t(b[0]) << 8 | b[1]; }

inline uint32_t GetUint32(uint8_t *b) {
  return uint32_t(b[0]) << 24 | uint32_t(b[0]) << 16 | uint32_t(b[2]) << 8 |
         b[3];
}
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
  // 自定义frame type
  uint8_t ftype;
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
  this->ftype = 3;
}

static uint8_t Unmarshal264(sdp &s, rtp *r, libyte::Buffer &b) {
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
      b.Write(s.spsvalue);
    }
    b.Write(_nalu_header, 4);
    b.Write(uint8_t(ntype | 0x60));
  }
  b.Write((char *)r->data, r->size);
  if (r->marker == 1 && (ntype == 1 || ntype == 5)) {
    r->ftype = ntype;
    sflag = 0x40;
  }
  // printf("rtp size %d %d %ld\n", r->size, ntype, b.Len());
  return sflag;
}

// NOTE. sps/vps/pps没有和i帧合并
static uint8_t Unmarshal265(sdp &s, rtp *r, libyte::Buffer &b) {
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
      b.Write(s.spsvalue);
    }
    b.Write(_nalu_header, 4);
    b.Write(ntype);
    b.Write(uint8_t(0x01));
  }
  ntype = (ntype & 0x7e) >> 1;
  b.Write((char *)r->data, r->size);
  // printf("rtp size %d %ld\n", r->size, b.Len());
  if (r->marker == 1 && (ntype == 1 || ntype == 19)) {
    r->ftype = ntype;
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
  SETUP_NOSS,
  SETAUDIO
};

inline const char *Format(int type) {
  switch (type) {
  case OPTIONS:
    return "OPTIONS %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "%s" // Authorization
           "User-Agent: " USER_AGENT "\r\n"
           "\r\n";
  case DESCRIBE:
    return "DESCRIBE %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "%s" // Authorization
           "User-Agent: " USER_AGENT "\r\n"
           "Accept: application/sdp\r\n"
           "\r\n";
  case SETUP:
    return "SETUP %s/%s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s" // Authorization
           "User-Agent: " USER_AGENT "\r\n"
           "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
           "\r\n";
  case SETUP_NOSS:
    return "SETUP %s/%s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "%s"
           "%s" // Authorization
           "User-Agent: " USER_AGENT "\r\n"
           "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
           "\r\n";
  case PLAY:
    return "PLAY %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s" // Authorization
           "User-Agent: " USER_AGENT "\r\n"
           "Range: npt=0.000-\r\n"
           "\r\n";
  case PAUSE:
    return "PAUSE %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "User-Agent: " USER_AGENT "\r\n"
           "%s" // Authorization
           "%s"
           "\r\n";
  case ANNOUNCE:
    return "ANNOUNCE %s RTSP/1.0\r\n"
           "CSeq: %d/r/n"
           "Content-Type: application/sdp\r\n"
           "%s"
           "Content-length: %d\r\n\r\n"
           "User-Agent: " USER_AGENT "\r\n"
           "%s\r\n";
  case SET_PARAMETER:
    return "SET_PARAMETER %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "User-Agent: " USER_AGENT "\r\n"
           "Content-length: %d\r\n\r\n"
           "%s: %s\r\n";
  case GET_PARAMETER:
    return "GET_PARAMETER %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "User-Agent: " USER_AGENT "\r\n"
           "%s" // Authorization
           "\r\n";
  case TEARDOWN:
    return "TEARDOWN %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "User-Agent: " USER_AGENT "\r\n"
           "%s" // Authorization
           "\r\n";
  default:
    break;
  }
  return "";
}

#define PKG_LEN 2048

class Client : libnet::TcpConn {
private:
  int cmd_;
  int seq_;
  rtp rtp_;
  uint8_t atype_; // audio rtp type
  url url_;
  sdp sdp_;
  libyte::Buffer rbuf_;
  libyte::Buffer dbuf_;
  std::function<uint8_t(sdp &, rtp *, libyte::Buffer &)> decode_;

public:
  Client(bool reqAudio = false) : cmd_(0), seq_(0), rtp_{0} {
    atype_ = reqAudio ? 0xff : 0;
  }
  ~Client() { Close(); }
  using FrameCallack = std::function<void(const char *format, uint8_t type,
                                          char *data, size_t length)>;
  // rtsp://admin:123456@127.0.0.1:554/test.mp4
  bool Play(const char *sUrl, FrameCallack callback = nullptr) {
    if (url_.Parse(sUrl) == false) {
      return false;
    }
    Dial(url_.ip.c_str(), url_.port);
    doWriteCmd(OPTIONS, seq_++, "");
    long long ts = libtime::UnixMilli();
    for (;;) {
      char buf[PKG_LEN] = {0};
      int n = Read(buf, PKG_LEN, 1000);
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
          uint8_t code = 0;
          if (rtp_.payloadType == 96 || rtp_.payloadType == 98) {
            code = this->decode_(sdp_, &rtp_, dbuf_);
            rtp_.ftype = rtp_.ftype == 1 ? 2 : 1;
          } else if (rtp_.payloadType == atype_) {
            code = 0x40;
            dbuf_.Write((char *)rtp_.data, rtp_.size);
          }
          if (code & 0x40) {
            auto fmt = sdp_.formats[rtp_.payloadType];
            if (callback) {
              callback(fmt.c_str(), rtp_.ftype, dbuf_.Bytes(), dbuf_.Len());
            } else {
              LibRtspDebug("rtp type %d channel %d, %ld\n", rtp_.payloadType,
                           ch, dbuf_.Len());
            }
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
    return true;
  }

  void Stop() {
    doWriteCmd(TEARDOWN, seq_++, sdp_.session.c_str(),
               url_.GetAuth("TEARDOWN").c_str());
  }

private:
  template <class... Args> void doWriteCmd(int type, Args... args) {
    cmd_ = type;
    char buf[PKG_LEN] = {0};
    if (type == SETUP && sdp_.session.empty()) {
      type = SETUP_NOSS;
    }
    sprintf(buf, Format(type), url_.path.c_str(), args...);
    LibRtspDebug("\nwrite --> \n%s", buf);
    Write(buf, strlen(buf));
  }

  void doRtspParse(char *b) {
    std::vector<std::string> res;
    char *ptr = b, *ptr1 = nullptr;
    while ((ptr1 = strstr(ptr, "\r\n"))) {
      std::string s(ptr, ptr1 - ptr);
      res.emplace_back(s);
      ptr = ptr1 + 2;
    }
    int len = ptr - b;
    LibRtspDebug("%d read --> \n%s", len, b);
    switch (cmd_) {
    case OPTIONS: {
      auto it = std::find_if(res.begin(), res.end(), [&](std::string &s) {
        return s.find("WWW-Authenticate") != std::string::npos;
      });
      if (it == res.end()) {
        doWriteCmd(DESCRIBE, seq_++, url_.GetAuth("DESCRIBE").c_str());
      } else {
        if (!url_.baseAuth.empty()) {
          throw libnet::Exception(libnet::EAccessDenied);
        }
        url_.SetAuth(it->c_str());
        doWriteCmd(OPTIONS, seq_++, url_.GetAuth("OPTIONS").c_str());
      }
      break;
    };
    case DESCRIBE: {
      sdp_.Parse(res);
      auto m = std::find_if(sdp_.medias.begin(), sdp_.medias.end(),
                            [](sdp::media &m) {
                              return m.name.find("video") != std::string::npos;
                            });
      if (m->rtpmap.find("H265") != std::string::npos) {
        this->decode_ = Unmarshal265;
      } else {
        this->decode_ = Unmarshal264;
      }
      doWriteCmd(SETUP, m->id.c_str(), seq_++, sdp_.session.c_str(),
                 url_.GetAuth("SETUP").c_str());
      cmd_ = SETAUDIO;
      break;
    };
    case SETAUDIO: {
      if (sdp_.session.empty()) {
        auto it = std::find_if(res.begin(), res.end(), [&](std::string &s) {
          return s.find("Session: ") != std::string::npos;
        });
        if (it != res.end()) {
          auto pos = it->find_first_of(';');
          sdp_.session = it->substr(9, pos - 9);
        }
      }
      if (atype_ == 0xff) {
        auto m = std::find_if(
            sdp_.medias.begin(), sdp_.medias.end(), [](sdp::media &m) {
              return m.name.find("audio") != std::string::npos;
            });
        if (m != sdp_.medias.end()) {
          atype_ = m->format;
          doWriteCmd(SETUP, m->id.c_str(), seq_++, sdp_.session.c_str(),
                     url_.GetAuth("SETUP").c_str());
          break;
        }
      }
    }
    case SETUP:
      doWriteCmd(PLAY, seq_++, sdp_.session.c_str(),
                 url_.GetAuth("PLAY").c_str());
      break;
    default:
      rbuf_.Remove(len);
      return;
    }
    memset(b, 0, len);
    rbuf_.Reset(0);
  }
};

} // namespace librtsp
