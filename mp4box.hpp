#pragma once

#include "types.h"
#include "sps.hpp"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <cstdint>
#include <string>

#define LibMp4Debug(fmt, ...) // printf(fmt, ##__VA_ARGS__)

namespace libmp4 {

static u32 HTOBE32(u32 v) {
  u32 r = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) |
          ((v >> 24) & 0xFF);
  return r;
}

static u32 LE32TYPE(const char *b) {
  u32 r = b[0];
  r |= u32(b[1]) << 8;
  r |= u32(b[2]) << 16;
  r |= u32(b[3]) << 24;
  return r;
}

static void Matrix(u32 *matrix) {
  // matrix[1] = htonl(65536);
  // matrix[3] = htonl(-65536);
  // matrix[6] = htonl(0x03840000);
  // matrix[8] = htonl(0x40000000);
  matrix[0] = HTOBE32(0x00010000);
  matrix[4] = HTOBE32(0x00010000);
  matrix[8] = HTOBE32(0x40000000);
}

#define HTOBE32_SIZEOF(x) HTOBE32(sizeof(x))

namespace box {
#pragma pack(1)
struct ftyp {
  u32 size;
  u32 type;
  u32 brand;
  u32 version;
  u32 compat1;
  u32 compat2;
  u32 compat3;
  u32 compat4;
  void *Marshal();
};

inline void *ftyp::Marshal() {
  size = HTOBE32_SIZEOF(ftyp);
  type = LE32TYPE("ftyp");
  brand = LE32TYPE("isom");
  version = HTOBE32(512);
  compat1 = LE32TYPE("isom");
  compat2 = LE32TYPE("iso2");
  compat4 = LE32TYPE("mp41");
  return this;
}
struct stsdv {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 entry_count;
  struct {
    u32 size; // ahvc1 <- avc1 & hvc1
    u32 type;
    u8 reserved1[6];
    u16 data_refidx;
    u16 predefined1;
    u16 reserved2;
    u32 predefined2[3];
    u16 width;
    u16 height;
    u32 horiz_res;
    u32 vert_res;
    u32 reserved3;
    u16 frame_count;
    u8 compressor[32];
    u16 depth;
    u16 predefined;
  } avc1;
  const char *Marshal(u32 n);
};

inline const char *stsdv::Marshal(u32 n) {
  type = LE32TYPE("stsd");
  entry_count = HTOBE32(1);
  // avc1.type = LE32TYPE("avc1"); // 外部赋值
  avc1.data_refidx = u16(HTOBE32(1) >> 16);
  avc1.width = u16(HTOBE32(avc1.width) >> 16);
  avc1.height = u16(HTOBE32(avc1.height) >> 16);
  avc1.horiz_res = HTOBE32(0x480000);
  avc1.vert_res = HTOBE32(0x480000);
  avc1.frame_count = u16(HTOBE32(1) >> 16);
  avc1.depth = u16(HTOBE32(24) >> 16);
  avc1.predefined = 0xFFFF;
  avc1.size = HTOBE32(n + sizeof(avc1));
  size = HTOBE32(n + sizeof(stsdv));
  return (const char *)this;
}

struct stsda {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 entry_count;
  struct {
    u32 size;
    u32 type;
    u8 reserved1[6];
    u16 data_refidx;
    u32 reserved2[2];
    u16 channel_num;
    u16 sample_size;
    u16 predefined1;
    u16 reserved3;
    u32 sample_rate;
    struct {
      u32 size;
      u32 type;
      u8 version;
      u8 flags[3];
      u8 esdesc_tag;    // 0x03
      u16 esdesc_len;   // ((25 << 8) | 0x80)
      u16 esdesc_id;    // 0x0200
      u8 esdesc_flags;  // 0x00
      u8 deccfg_tag;    // 0x04
      u16 deccfg_len;   // 17
      u8 deccfg_object; // 0x40 - aac, 0x6B - mp3
      u8 deccfg_stream; // 0x15
      u8 deccfg_buffer_size[3];
      u32 deccfg_max_bitrate;
      u32 deccfg_avg_bitrate;
      //++ if deccfg_object == aac
      // u8 decspec_tag; // 0x05
      // u8 decspec_len; // 2
      // u16 decspec_info;
      // //-- if deccfg_object == aac
      u8 slcfg_tag;      // 0x06
      u16 slcfg_len;     // 1
      u8 slcfg_reserved; // 0x02
    } esds;
  } mp4a;
  const char *Marshal(char *aacspec);
};

inline const char *stsda::Marshal(char *aacspec) {
  size = HTOBE32_SIZEOF(stsda);
  type = LE32TYPE("stsd");
  entry_count = HTOBE32(1);
  mp4a.size = HTOBE32_SIZEOF(mp4a);
  mp4a.type = LE32TYPE("mp4a");
  mp4a.data_refidx = u16(HTOBE32(1) >> 16);
  mp4a.channel_num = u16(HTOBE32(1) >> 16);
  mp4a.sample_size = u16(HTOBE32(16) >> 16);
  mp4a.sample_rate = HTOBE32(8000 << 16);
  mp4a.esds.size = HTOBE32_SIZEOF(mp4a.esds);
  mp4a.esds.type = LE32TYPE("esds");
  mp4a.esds.esdesc_tag = 0x03;
  mp4a.esds.esdesc_len = u16(25 << 8 | 0x80);
  mp4a.esds.esdesc_id = 0x0200;
  mp4a.esds.esdesc_flags = 0x00;
  mp4a.esds.deccfg_tag = 0x04;
  mp4a.esds.deccfg_len = u16(17 << 8 | 0x80);
  mp4a.esds.deccfg_object = 0x40;
  mp4a.esds.deccfg_stream = 0x15;
  // mp4a.esds.decspec_tag = 0x05;
  // mp4a.esds.decspec_len = 2;
  // mp4a.esds.decspec_info = 0x0812;
  mp4a.esds.slcfg_tag = 0x06;
  mp4a.esds.slcfg_len = u16(1 << 8 | 0x80);
  mp4a.esds.slcfg_reserved = 0x02;
  return (const char *)this;
}

// sample_deltas ts
struct stts {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 count;
  const char *Marshal(u32 num);
};

inline const char *stts::Marshal(u32 num) {
  size = HTOBE32(sizeof(stts) + num * sizeof(u32));
  type = LE32TYPE("stts");
  count = HTOBE32(num / 2);
  return (const char *)this;
}

// sample_numbers
struct stss {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 count;
  const char *Marshal(u32 num);
};

inline const char *stss::Marshal(u32 num) {
  size = HTOBE32(sizeof(stss) + num * sizeof(u32));
  type = LE32TYPE("stss");
  count = HTOBE32(num);
  return (const char *)this;
}

struct stsc {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 count;
  u32 first_chunk;
  u32 samp_per_chunk;
  u32 samp_desc_id;
  const char *Marshal();
};
inline const char *stsc::Marshal() {
  size = HTOBE32(sizeof(stsc));
  type = LE32TYPE("stsc");
  count = HTOBE32(1);
  first_chunk = HTOBE32(1);
  samp_per_chunk = HTOBE32(1);
  samp_desc_id = HTOBE32(1);
  return (const char *)this;
}

// sample_sizes
struct stsz {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 sample_size;
  u32 count;
  const char *Marshal(u32 num);
};

inline const char *stsz::Marshal(u32 num) {
  size = HTOBE32(sizeof(stsz) + num * sizeof(u32));
  type = LE32TYPE("stsz");
  count = HTOBE32(num);
  return (const char *)this;
}

// chunk_offsets
struct stco {
  u32 size;
  u32 type;
  u8 version;
  u8 flags[3];
  u32 count;
  const char *Marshal(u32 num);
};

inline const char *stco::Marshal(u32 num) {
  size = HTOBE32(sizeof(stco) + num * sizeof(u32));
  type = LE32TYPE("stco");
  count = HTOBE32(num);
  return (const char *)this;
}

struct trak {
  u32 size;
  u32 type;
  struct {
    u32 size;
    u32 type;
    u8 version;
    u8 flags[3];
    u32 create_time;
    u32 modify_time;
    u32 trackid;
    u32 reserved1;
    u32 duration;
    u32 reserved2;
    u32 reserved3;
    u16 layer;
    u16 alternate_group;
    u16 volume;
    u16 reserved4;
    u32 matrix[9];
    u32 width;
    u32 height;
  } tkhd;
  struct {
    u32 size;
    u32 type;
    struct {
      u32 size;
      u32 type;
      u8 version;
      u8 flags[3];
      u32 create_time;
      u32 modify_time;
      u32 timescale;
      u32 duration;
      u16 language;
      u16 predefined;
    } mdhd;
    struct {
      u32 size;
      u32 type;
      u8 version;
      u8 flags[3];
      u8 predefined[4];
      u32 handler_type;
      u8 reserved[12];
      u8 name[16];
    } hdlr;
    struct {
      u32 size;
      u32 type;
      struct {
        u32 size;
        u32 type;
        u8 version;
        u8 flags[3];
        u32 graph_mode;
        u32 opcolor;
      } vmhd;
      struct {
        u32 size;
        u32 type;
        struct {
          u32 size;
          u32 type;
          u8 version;
          u8 flags[3];
          u32 entry_count;
          struct {
            u32 size;
            u32 type;
            u8 version;
            u8 flags[3];
          } url;
        } dref;
      } dinf;
      struct {
        int32_t size;
        u32 type;
      } stbl;
    } minf;
  } mdia;
  const char *Marshal(u32 id, u32 length);
};

inline const char *trak::Marshal(u32 id, u32 length) {
  // tkhd
  tkhd.size = HTOBE32_SIZEOF(tkhd);
  tkhd.type = LE32TYPE("tkhd");
  tkhd.flags[2] = 0xF;
  tkhd.trackid = HTOBE32(id);
  Matrix(tkhd.matrix);
  tkhd.width = HTOBE32(tkhd.width << 16);   //
  tkhd.height = HTOBE32(tkhd.height << 16); //

  // mdhd
  mdia.mdhd.size = HTOBE32_SIZEOF(mdia.mdhd);
  mdia.mdhd.type = LE32TYPE("mdhd");
  mdia.mdhd.timescale = HTOBE32(1000);
  // hdlr
  mdia.hdlr.size = HTOBE32_SIZEOF(mdia.hdlr);
  mdia.hdlr.type = LE32TYPE("hdlr");
  if (id == 1) {
    mdia.hdlr.handler_type = LE32TYPE("vide");
    mdia.minf.vmhd.type = LE32TYPE("vmhd");
  } else {
    mdia.hdlr.handler_type = LE32TYPE("soun");
    mdia.minf.vmhd.type = LE32TYPE("smhd"); // 音频
  }
  ::strcpy((char *)mdia.hdlr.name, "dontls");
  mdia.minf.vmhd.size = HTOBE32_SIZEOF(mdia.minf.vmhd);
  mdia.minf.vmhd.flags[2] = 1;

  mdia.minf.dinf.size = HTOBE32_SIZEOF(mdia.minf.dinf);
  mdia.minf.dinf.type = LE32TYPE("dinf");

  mdia.minf.dinf.dref.url.size = HTOBE32_SIZEOF(mdia.minf.dinf.dref.url);
  mdia.minf.dinf.dref.url.type = LE32TYPE("url ");
  mdia.minf.dinf.dref.url.flags[2] = 1;

  mdia.minf.dinf.dref.size = HTOBE32_SIZEOF(mdia.minf.dinf.dref);
  mdia.minf.dinf.dref.type = LE32TYPE("dref");
  mdia.minf.dinf.dref.entry_count = HTOBE32(1);

  // minf
  mdia.minf.size = HTOBE32(sizeof(mdia.minf) + length);
  mdia.minf.type = LE32TYPE("minf");
  LibMp4Debug("minf size = %u\n", HTOBE32(mdia.minf.size));

  // mdia
  mdia.size = HTOBE32(sizeof(mdia) + length);
  mdia.type = LE32TYPE("mdia");
  LibMp4Debug("mdia size = %u\n", HTOBE32(mdia.size));

  // stbl
  mdia.minf.stbl.size = HTOBE32(sizeof(mdia.minf.stbl) + length);
  mdia.minf.stbl.type = LE32TYPE("stbl");
  LibMp4Debug("stbl size = %u\n", HTOBE32(mdia.minf.stbl.size));

  size = HTOBE32(sizeof(trak) + length);
  type = LE32TYPE("trak");
  LibMp4Debug("trak size = %u\n", HTOBE32(size));
  return (const char *)this;
}

struct moov {
  u32 size;
  u32 type;
  struct {
    u32 size;
    u32 type;
    u8 version;
    u8 flags[3];
    u32 create_time;
    u32 modify_time;
    u32 timescale;
    u32 duration;
    u32 playrate;
    u16 volume;
    u8 reserved[10];
    u32 matrix[9];
    u8 predefined[24];
    u32 next_trackid;
  } mvhd;
  void *Marshal(u32 len1, u32 len2 = 0);
};

inline void *moov::Marshal(u32 len1, u32 len2) {
  size = HTOBE32(sizeof(moov) + len1 + len2);
  type = LE32TYPE("moov");
  mvhd.size = HTOBE32_SIZEOF(mvhd);
  mvhd.type = LE32TYPE("mvhd");
  mvhd.create_time = HTOBE32((u32)time(NULL));
  mvhd.modify_time = mvhd.create_time;
  // mvhd.duration = ; // 外部更新
  mvhd.timescale = HTOBE32(1000);

  mvhd.playrate = HTOBE32(0x00010000);
  mvhd.volume = u16(HTOBE32(1));
  Matrix(mvhd.matrix);
  mvhd.next_trackid = len2 > 0 ? HTOBE32(3) : HTOBE32(2);
  return this;
}

struct mdat {
  u32 size;
  u32 type;
};
#pragma pack()
} // namespace box
struct avcc {
  u32 size;
  u32 type;
  u8 config_ver;
  u8 avc_profile;
  u8 profile_compat;
  u8 avc_level;
  u8 nalulen;
  u8 sps_num;
  u16 sps_len;
  u8 *sps;
  u8 pps_num;
  u16 pps_len;
  u8 *pps;
};

inline int AvccMarshal(char *buf, nalu::Value &sps, nalu::Value &pps) {
  // 附加avcc信息
  avcc *avc = (avcc *)(buf);
  avc->type = LE32TYPE("avcC");
  avc->config_ver = 1;
  avc->avc_profile = sps.size > 1 ? sps.data[1] : 0;
  avc->profile_compat = sps.size > 2 ? sps.data[2] : 0;
  avc->avc_level = sps.size > 3 ? sps.data[3] : 0;
  avc->nalulen = 0xFF;
  int i = offsetof(avcc, sps_num);
  buf[i++] = char((0x7 << 5) | (1 << 0));
  buf[i++] = (sps.size << 8) & 0xff;
  buf[i++] = sps.size & 0xff;
  ::memcpy(&buf[i], sps.data, sps.size);
  i += sps.size;
  buf[i++] = 1;
  buf[i++] = (pps.size << 8) & 0xff;
  buf[i++] = pps.size & 0xff;
  ::memcpy(&buf[i], pps.data, pps.size);
  i += pps.size;
  avc->size = HTOBE32(i);
  return i;
}

struct hvcc {
  u32 size;
  u32 type;
  u8 configurationVersion;
  u8 general_profile_idc : 5;
  u8 general_tier_flag : 1;
  u8 general_profile_space : 2;
  u32 general_profile_compatibility_flags;
  u32 general_constraint_indicator_flags0;
  u16 general_constraint_indicator_flags1;
  u8 general_level_idc;
  u16 min_spatial_segmentation_idc;
  u8 parallelismType;
  u8 chromaFormat;
  u8 bitDepthLumaMinus8;
  u8 bitDepthChromaMinus8;
  u16 avgFrameRate;
  u8 lengthSizeMinusOne : 2;
  u8 temporalIdNested : 1;
  u8 numTemporalLayers : 3;
  u8 constantFrameRate : 2;
  u8 numOfArrays;
};

inline int HvccMarshal(char *buf, nalu::Value &sps, nalu::Value &pps,
                       nalu::Value &vps) {
  hvcc *hvc = (hvcc *)(buf);
  hvc->type = LE32TYPE("hvcC");
  hvc->configurationVersion = 1;
  // hvc->general_profile_space= 0;
  // hvc->general_tier_flag    = 0;
  hvc->general_profile_idc = 1;
  hvc->general_profile_compatibility_flags = 0x60;
  hvc->general_constraint_indicator_flags0 = 0x90;
  hvc->general_constraint_indicator_flags1 = 0x00;
  hvc->general_level_idc = 63;
  hvc->min_spatial_segmentation_idc = 0xf0;
  hvc->parallelismType = 0 | 0xfc;
  hvc->chromaFormat = 1 | 0xfc;
  hvc->bitDepthLumaMinus8 = 0 | 0xf8;
  hvc->bitDepthChromaMinus8 = 0 | 0xf8;
  // hvc->avgFrameRate = (u16)(htonl(mp4a.frate) >> 16);
  hvc->avgFrameRate = 0;
  // hvc->constantFrameRate    = 0;
  hvc->numTemporalLayers = 1;
  hvc->temporalIdNested = 1;
  hvc->lengthSizeMinusOne = 3;
  hvc->numOfArrays = 3;
  int i = sizeof(hvcc);
  buf[i++] = 32;
  buf[i++] = 0x00;
  buf[i++] = 0x01;
  buf[i++] = (vps.size >> 8) & 0xff;
  buf[i++] = vps.size & 0xff;
  memcpy(&buf[i], vps.data, vps.size);
  i += vps.size;

  buf[i++] = 33;
  buf[i++] = 0x00;
  buf[i++] = 0x01;
  buf[i++] = (sps.size >> 8) & 0xff;
  buf[i++] = sps.size & 0xff;
  memcpy(&buf[i], sps.data, sps.size);
  i += sps.size;

  buf[i++] = 34;
  buf[i++] = 0x00;
  buf[i++] = 0x01;
  buf[i++] = (pps.size >> 8) & 0xff;
  buf[i++] = pps.size & 0xff;
  memcpy(&buf[i], pps.data, pps.size);
  i += pps.size;
  hvc->size = HTOBE32(i);
  return i;
}

class Trak {
private:
  u32 id_;
  box::trak trak_;
  struct {
    box::stts stts;
    box::stss stss;
    box::stsc stsc;
    box::stsz stsz;
    box::stco stco;
  } stbl;
  int count_;
  std::vector<u32> value[4];
  std::string stsdv_;

public:
  Trak() : id_(0), trak_{}, stbl{}, count_(0), stsdv_{} {}
  ~Trak() {}
  void AppendSample(int64_t ts, u32 &offset, u32 length);
  u32 MakeVideo(nalu::Vector &nalus);
  u32 MakeAudio(char *accspec);
  int Marshal(u32 beDur);
  const char *Value() { return stsdv_.c_str(); }
};

inline void Trak::AppendSample(int64_t dur, u32 &offset, u32 length) {
  if (dur < 0) {
    dur = 0;
  }
  value[0].emplace_back(HTOBE32(u32(dur))); // stts
  if (id_ == 1) {
    value[1].emplace_back(HTOBE32(1)); // stss
  }
  value[2].emplace_back(HTOBE32(length)); // stsz
  value[3].emplace_back(HTOBE32(offset)); // stco
  count_++;
  offset += length;
}

inline int Trak::Marshal(u32 beDur) {
  trak_.tkhd.duration = beDur;
  trak_.mdia.mdhd.duration = beDur;
  int samplesize = sizeof(u32) * count_;
  std::string str(stbl.stts.Marshal(count_), sizeof(box::stts));
  str.append((char *)value[0].data(), samplesize);
  if (id_ == 1) {
    str.append(stbl.stss.Marshal(count_), sizeof(box::stss));
    str.append((char *)value[1].data(), samplesize);
  }
  str.append(stbl.stsc.Marshal(), sizeof(box::stsc));
  str.append(stbl.stsz.Marshal(count_), sizeof(box::stsz));
  str.append((char *)value[2].data(), samplesize);
  str.append(stbl.stco.Marshal(count_), sizeof(box::stco));
  str.append((char *)value[3].data(), samplesize);
  u32 size = u32(stsdv_.length() + str.length());
  stsdv_ = std::string(trak_.Marshal(id_, size), sizeof(box::trak)) + stsdv_;
  stsdv_.append(str);
  return size + sizeof(box::trak);
}

static nalu::Vector naluSort(nalu::Vector &nalus) {
  nalu::Vector res(2);
  nalu::Value *vps = nullptr;
  bool b264 = nalu::IsH264(nalus[0].type);
  for (auto &v : nalus) {
    if (b264) {
      switch (v.type & 0X1F) {
      case avc::NALU_TYPE_SPS:
        res[0] = v;
        /* code */
        break;
      case avc::NALU_TYPE_PPS:
        res[1] = v;
        break;
      }
    } else {
      switch ((v.type & 0x7E) >> 1) {
      case hevc::NALU_TYPE_SPS:
        res[0] = v;
        /* code */
        break;
      case hevc::NALU_TYPE_PPS:
        res[1] = v;
        break;
      case hevc::NALU_TYPE_VPS:
        vps = &v;
        break;
      }
    }
  }
  if (vps != nullptr) {
    res.push_back(*vps);
  }
  return res;
}

inline u32 Trak::MakeVideo(nalu::Vector &nalus) {
  id_ = 1;
  int fps = 0, n = 0;
  char buf[256] = {0};
  box::stsdv stsd{0};
  auto sss = naluSort(nalus);
  nalu::Value &sps = sss[0];
  if (sss.size() == 2) {
    stsd.avc1.type = LE32TYPE("avc1");
    avc::decode_sps((BYTE *)sps.data, sps.size, trak_.tkhd.width,
                    trak_.tkhd.height, fps);
    n = AvccMarshal(buf, sps, sss[1]);
  } else {
    stsd.avc1.type = LE32TYPE("hvc1");
    hevc::decode_sps((BYTE *)sps.data, sps.size, trak_.tkhd.width,
                     trak_.tkhd.height, fps);
    n = HvccMarshal(buf, sps, sss[1], sss[2]);
  }
  stsd.avc1.width = trak_.tkhd.width;
  stsd.avc1.height = trak_.tkhd.height;
  stsdv_ = std::string(stsd.Marshal(n), sizeof(box::stsdv));
  stsdv_.append(buf, n);
  return stsd.avc1.type;
}

inline u32 Trak::MakeAudio(char *accspec) {
  id_ = 2;
  // box::stsda stsd{};
  // const char *data = stsd.Marshal(accspec);
  // stsdv_.append(data, sizeof(box::stsda));
  // //ffmpeg
  unsigned char data[] = {
      0x00, 0x00, 0x00, 0x60, 0x73, 0x74, 0x73, 0x64, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x6D, 0x70, 0x34, 0x61,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
      0x1F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x65, 0x73, 0x64, 0x73,
      0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x80, 0x80, 0x1B, 0x00, 0x02, 0x00,
      0x04, 0x80, 0x80, 0x80, 0x0D, 0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x3E, 0x80, 0x00, 0x00, 0x35, 0xDB, 0x06, 0x80, 0x80, 0x80, 0x01, 0x02};
  stsdv_.append((char *)data, sizeof(data));
  return 0;
}

} // namespace libmp4