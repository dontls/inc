#pragma once
#include "rtp.hpp"
#include "socket.hpp"
#include "time.hpp"
#include "crypto/base64.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

#define LibRtspDebug // printf

#define SUPPORT_DIGEST 1
#define USER_AGENT "github/dontls"

#if SUPPORT_DIGEST
#include "crypto/md5.h"
#endif

namespace librtsp {

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
  std::string content;
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
        if ((pos = it->id.rfind("/")) != std::string::npos) {
          it->id = it->id.substr(pos + 1);
        }
      }
    }
  }
  auto m = std::find_if(medias.begin(), medias.end(), [](sdp::media &m) {
    return m.name.find("video") != std::string::npos;
  });
  if (m == medias.end()) {
    return;
  }
  std::string sps, pps, vps;
  if (m->format == 96) {
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

enum {
  OPTIONS,
  DESCRIBE,
  SETUP,
  PLAY,
  SCALE,
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
           "Range: npt=%0.3f-\r\n"
           "\r\n";
  case SCALE:
    return "PLAY %s RTSP/1.0\r\n"
           "CSeq: %d\r\n"
           "Session: %s\r\n"
           "%s" // Authorization
           "User-Agent: " USER_AGENT "\r\n"
           "Scale: %0.1f\r\n"
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

class Client : libnet::TcpConn {
private:
  int cmd_;
  int seq_;
  uint8_t atype_; // audio rtp type
  url url_;
  sdp sdp_;
  libyte::Buffer dbuf_;
  std::function<uint8_t(std::string &, librtp::Packet *, libyte::Buffer &)>
      decode_;

public:
  Client(bool reqAudio = false) : cmd_(0), seq_(0), OnRTPAnyPacket(nullptr) {
    atype_ = reqAudio ? 0xff : 0;
  }
  ~Client() { Close(); }
  // 转发rtp包代理
  std::function<void(uint8_t *, int)> OnRTPAnyPacket;
  // 解析帧
  using OnFrame = std::function<void(const char *, uint8_t, libyte::Buffer &)>;

  // rtsp://admin:123456@127.0.0.1:554/test.mp4
  bool Play(const char *sUrl, OnFrame callFrame) {
    if (!url_.Parse(sUrl)) {
      return false;
    }
    Dial(url_.ip.c_str(), url_.port);
    doWriteCmd(OPTIONS, seq_++, "");
    long long ts = libtime::UnixMilli();
    this->LoopRead(
        [&](libyte::Buffer &buf) {
          int blen = static_cast<int>(buf.Len()) - 4;
          if (blen < 0) {
            return 0;
          }
          uint8_t *b = (uint8_t *)buf.Bytes();
          if (b[0] != '$') {
            // GET_PARAMETER response
            return doRtspParse((char *)b, blen + 4);
          }
          uint8_t ch = b[1];
          int dlen = Uint16(&b[2]);
          if (blen < dlen) {
            return 0;
          }
          if (OnRTPAnyPacket) {
            this->OnRTPAnyPacket(b, dlen + 4);
          }
          auto rtp = librtp::Unmarshal(b + 4, dlen);
          if (callFrame) {
            uint8_t ftype = 0;
            if (rtp.payloadType == 96 || rtp.payloadType == 98) {
              ftype = this->decode_(sdp_.spsvalue, &rtp, dbuf_);
              if (ftype > 0) {
                ftype = ftype == 1 ? 2 : 1;
              }
            } else if (rtp.payloadType == atype_) {
              ftype = 3;
              dbuf_.Write((char *)rtp.data, rtp.size);
            }
            if (ftype > 0) {
              auto &fmt = sdp_.formats[rtp.payloadType];
              callFrame(fmt.c_str(), ftype, dbuf_);
              dbuf_.Reset(0);
            }
          }
          LibRtspDebug("%u rtp type %d channel %d, %d\n", rtp.timestamp,
                       rtp.payloadType, ch, dlen);
          // 保活机制
          if (libtime::Since(ts) > 10000) {
            ts = libtime::UnixMilli();
            this->doKeepalive(&rtp);
          }
          return dlen + 4;
        },
        1000);
    return true;
  }

  void Stop() {
    doWriteCmd(TEARDOWN, seq_++, sdp_.session.c_str(),
               url_.GetAuth("TEARDOWN").c_str());
  }

private:
  template <class... Args> void doWriteCmd(int type, Args... args) {
    cmd_ = type;
    char buf[2048] = {0};
    if (type == SETUP && sdp_.session.empty()) {
      type = SETUP_NOSS;
    }
    sprintf(buf, Format(type), url_.path.c_str(), args...);
    LibRtspDebug("\nwrite --> \n%s", buf);
    Write(buf, int(strlen(buf)));
  }

  void doKeepalive(librtp::Packet *rtp) {
    doWriteCmd(GET_PARAMETER, seq_++, sdp_.session.c_str(),
               url_.GetAuth("GET_PARAMETER").c_str());
    // librtcp::Packet pkt{0};
    // pkt.pt = 200;
    // pkt.r.sr.rtp_ts = rtp->timestamp;
    // pkt.r.sr.rb.ssrc = rtp->seq;
    // this->Write((char *)&pkt, sizeof(pkt));
  }

  int doRtspParse(char *b, int len) {
    char *h0 = strstr(b, "\r\n\r\n");
    if (h0 == nullptr) {
      return 0;
    }
    int hlen = int(h0 - b) + 4;
    std::string hs(b, hlen);
    int dlen = std::atoi(findVar(hs, "Content-Length: ", "\r\n").c_str());
    if (dlen + hlen > len) {
      return 0;
    }
    len = hlen;
    if (dlen > 0) {
      len += dlen;
      hs = std::string(b, len);
    }
    std::vector<std::string> res;
    char *ptr = (char *)hs.c_str(), *ptr1 = nullptr;
    while ((ptr1 = strstr(ptr, "\r\n"))) {
      std::string s(ptr, ptr1 - ptr);
      res.emplace_back(s);
      ptr = ptr1 + 2;
    }
    LibRtspDebug("%d read --> \n%s", len, hs.c_str());
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
      sdp_.content = hs.substr(hlen);
      sdp_.Parse(res);
      auto m = std::find_if(sdp_.medias.begin(), sdp_.medias.end(),
                            [](sdp::media &m) {
                              return m.name.find("video") != std::string::npos;
                            });
      this->decode_ = (m->rtpmap.find("H265") != std::string::npos)
                          ? librtp::h265::Unmarshal
                          : librtp::h264::Unmarshal;
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
                 url_.GetAuth("PLAY").c_str(), 0);
      break;
    default:
      break;
    }
    return len;
  }
};

} // namespace librtsp
