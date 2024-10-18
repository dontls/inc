#pragma once

#include "types.h"
#include "sps.hpp"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <cstdint>
#include <string>

#define Debug printf

namespace libmp4 {

static u32 Htobe32(u32 v) {
  u32 r = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) |
          ((v >> 24) & 0xFF);
  return r;
}

static u32 Le32Type(const char *b) {
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
  matrix[0] = Htobe32(0x00010000);
  matrix[4] = Htobe32(0x00010000);
  matrix[8] = Htobe32(0x40000000);
}

#define Htobe32_Sizeof(x) Htobe32(sizeof(x))

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
  size = Htobe32_Sizeof(ftyp);
  type = Le32Type("ftyp");
  brand = Le32Type("isom");
  version = Htobe32(512);
  compat1 = Le32Type("isom");
  compat2 = Le32Type("iso2");
  compat4 = Le32Type("mp41");
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
  type = Le32Type("stsd");
  entry_count = Htobe32(1);
  // avc1.type = Le32Type("avc1"); // 外部赋值
  avc1.data_refidx = u16(Htobe32(1) >> 16);
  avc1.width = u16(Htobe32(avc1.width) >> 16);
  avc1.height = u16(Htobe32(avc1.height) >> 16);
  avc1.horiz_res = Htobe32(0x480000);
  avc1.vert_res = Htobe32(0x480000);
  avc1.frame_count = u16(Htobe32(1) >> 16);
  avc1.depth = u16(Htobe32(24) >> 16);
  avc1.predefined = 0xFFFF;
  avc1.size = Htobe32(n + sizeof(avc1));
  size = Htobe32(n + sizeof(stsdv));
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
      u8 deccfg_len;    // 17
      u8 deccfg_object; // 0x40 - aac, 0x6B - mp3
      u8 deccfg_stream; // 0x15
      u8 deccfg_buffer_size[3];
      u32 deccfg_max_bitrate;
      u32 deccfg_avg_bitrate;
      //++ if deccfg_object == aac
      u8 decspec_tag; // 0x05
      u8 decspec_len; // 2
      u16 decspec_info;
      //-- if deccfg_object == aac
      u8 slcfg_tag;      // 0x06
      u8 slcfg_len;      // 1
      u8 slcfg_reserved; // 0x02
    } esds;
  } mp4a;
  const char *Marshal(char *aacspec);
};

inline const char *stsda::Marshal(char *aacspec) {
  size = Htobe32_Sizeof(stsda);
  type = Le32Type("stsd");
  entry_count = Htobe32(1);
  // avc1.type = Le32Type("avc1"); // 外部赋值
  mp4a.size = Htobe32_Sizeof(mp4a);
  mp4a.type = Le32Type("mp4a");
  mp4a.data_refidx = (u16)(Htobe32(1) >> 16);
  // mp4a.channel_num =
  mp4a.esds.size = Htobe32_Sizeof(mp4a.esds);
  mp4a.esds.type = Le32Type("esds");
  mp4a.esds.esdesc_tag = 0x03;
  mp4a.esds.esdesc_len = ((25 << 8) | 0x80);
  mp4a.esds.esdesc_id = 0x0200;
  mp4a.esds.esdesc_flags = 0x00;
  mp4a.esds.deccfg_tag = 0x04;
  mp4a.esds.deccfg_len = 17;
  mp4a.esds.deccfg_object = 0x40;
  mp4a.esds.deccfg_stream = 0x15;
  mp4a.esds.decspec_tag = 0x05;
  mp4a.esds.decspec_len = 2;
  mp4a.esds.decspec_info = aacspec ? (aacspec[1] << 8) | (aacspec[0] << 0) : 0;
  mp4a.esds.slcfg_tag = 0x06;
  mp4a.esds.slcfg_len = 1;
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
  size = Htobe32(sizeof(stts) + num * sizeof(u32));
  type = Le32Type("stts");
  count = Htobe32(num / 2);
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
  size = Htobe32(sizeof(stss) + num * sizeof(u32));
  type = Le32Type("stss");
  count = Htobe32(num);
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
  size = Htobe32(sizeof(stsc));
  type = Le32Type("stsc");
  count = Htobe32(1);
  first_chunk = Htobe32(1);
  samp_per_chunk = Htobe32(1);
  samp_desc_id = Htobe32(1);
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
  size = Htobe32(sizeof(stsz) + num * sizeof(u32));
  type = Le32Type("stsz");
  count = Htobe32(num);
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
  size = Htobe32(sizeof(stco) + num * sizeof(u32));
  type = Le32Type("stco");
  count = Htobe32(num);
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
  tkhd.size = Htobe32_Sizeof(tkhd);
  tkhd.type = Le32Type("tkhd");
  tkhd.flags[2] = 0xF;
  tkhd.trackid = Htobe32(id);
  Matrix(tkhd.matrix);
  tkhd.width = Htobe32(tkhd.width << 16);   //
  tkhd.height = Htobe32(tkhd.height << 16); //

  // mdhd
  mdia.mdhd.size = Htobe32_Sizeof(mdia.mdhd);
  mdia.mdhd.type = Le32Type("mdhd");
  mdia.mdhd.timescale = Htobe32(1000);
  // hdlr
  mdia.hdlr.size = Htobe32_Sizeof(mdia.hdlr);
  mdia.hdlr.type = Le32Type("hdlr");
  mdia.hdlr.handler_type = Le32Type("vide");
  ::strcpy((char *)mdia.hdlr.name, "dontls");

  mdia.minf.vmhd.size = Htobe32_Sizeof(mdia.minf.vmhd);
  if (id == 1) {
    mdia.minf.vmhd.type = Le32Type("vmhd");
  } else {
    mdia.minf.vmhd.type = Le32Type("smhd"); // 音频
  }
  mdia.minf.vmhd.flags[2] = 1;

  mdia.minf.dinf.size = Htobe32_Sizeof(mdia.minf.dinf);
  mdia.minf.dinf.type = Le32Type("dinf");

  mdia.minf.dinf.dref.url.size = Htobe32_Sizeof(mdia.minf.dinf.dref.url);
  mdia.minf.dinf.dref.url.type = Le32Type("url ");
  mdia.minf.dinf.dref.url.flags[2] = 1;

  mdia.minf.dinf.dref.size = Htobe32_Sizeof(mdia.minf.dinf.dref);
  mdia.minf.dinf.dref.type = Le32Type("dref");
  mdia.minf.dinf.dref.entry_count = Htobe32(1);

  // minf
  mdia.minf.size = Htobe32(sizeof(mdia.minf) + length);
  mdia.minf.type = Le32Type("minf");
  Debug("minf size = %u\n", Htobe32(mdia.minf.size));

  // mdia
  mdia.size = Htobe32(sizeof(mdia) + length);
  mdia.type = Le32Type("mdia");
  Debug("mdia size = %u\n", Htobe32(mdia.size));

  // stbl
  mdia.minf.stbl.size = Htobe32(sizeof(mdia.minf.stbl) + length);
  mdia.minf.stbl.type = Le32Type("stbl");
  Debug("stbl size = %u\n", Htobe32(mdia.minf.stbl.size));

  size = Htobe32(sizeof(trak) + length);
  type = Le32Type("trak");
  Debug("trak size = %u\n", Htobe32(size));
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
  size = Htobe32(sizeof(moov) + len1 + len2);
  type = Le32Type("moov");
  mvhd.size = Htobe32_Sizeof(mvhd);
  mvhd.type = Le32Type("mvhd");
  mvhd.create_time = Htobe32((u32)time(NULL));
  mvhd.modify_time = mvhd.create_time;
  // mvhd.duration = ; // 外部更新
  mvhd.timescale = Htobe32(1000);

  mvhd.playrate = Htobe32(0x00010000);
  mvhd.volume = u16(Htobe32(1));
  Matrix(mvhd.matrix);
  if (len2 > 0) {
    mvhd.next_trackid = Htobe32(3);
  } else {
    mvhd.next_trackid = Htobe32(2);
  }
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
  avc->type = Le32Type("avcC");
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
  avc->size = Htobe32(i);
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
  hvc->type = Le32Type("hvcC");
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
  hvc->size = Htobe32(i);
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
  int64_t firts_;
  int64_t lsts_;
  int count_;
  std::vector<u32> value[4];
  std::string str;
  std::string stsdv_;

public:
  Trak() : id_(0), trak_{0}, stbl{0}, firts_(0), lsts_(0), count_(0) {}
  ~Trak() {}
  void AppendSample(int64_t ts, u32 &offset, u32 length);
  u32 MakeVideo(nalu::Vector &nalus);
  u32 MakeAudio(char *accspec);
  int Marshal();
  u32 Duration() { return Htobe32(u32(lsts_ - firts_)); }
  const char *Value() { return str.c_str(); }
};

inline void Trak::AppendSample(int64_t ts, u32 &offset, u32 length) {
  if (firts_ == 0) {
    firts_ = lsts_ = ts;
  }
  value[0].emplace_back(Htobe32(u32(ts - lsts_))); // stts
  value[1].emplace_back(Htobe32(1));          // stss
  value[2].emplace_back(Htobe32(length));     // stsz
  value[3].emplace_back(Htobe32(offset));     // stco
  lsts_ = ts;
  count_++;
  offset += length;
}

inline int Trak::Marshal() {
  u32 dur = Htobe32(u32(lsts_ - firts_));
  trak_.tkhd.duration = dur;
  trak_.mdia.mdhd.duration = dur;
  int samplesize = sizeof(u32) * count_;
  u32 size = u32(sizeof(stbl) + stsdv_.length() + samplesize * 4);
  str = std::string(trak_.Marshal(id_, size), sizeof(box::trak));
  str.append(stsdv_);
  str.append(stbl.stts.Marshal(count_), sizeof(box::stts));
  str.append((char *)value[0].data(), samplesize);
  str.append(stbl.stss.Marshal(count_), sizeof(box::stss));
  str.append((char *)value[1].data(), samplesize);
  str.append(stbl.stsc.Marshal(), sizeof(box::stsc));
  str.append(stbl.stsz.Marshal(count_), sizeof(box::stsz));
  str.append((char *)value[2].data(), samplesize);
  str.append(stbl.stco.Marshal(count_), sizeof(box::stco));
  str.append((char *)value[3].data(), samplesize);
  return size + sizeof(box::trak);
}

inline u32 Trak::MakeVideo(nalu::Vector &nalus) {
  assert(id_ == 0);
  int fps = 0, n = 0;
  char buf[256] = {0};
  nalu::Value &sps = nalus[0];
  box::stsdv stsd;
  if (nalu::IsH264(sps.type)) {
    stsd.avc1.type = Le32Type("avc1");
    avc::decode_sps((BYTE *)sps.data, sps.size, trak_.tkhd.width,
                    trak_.tkhd.height, fps);
    n = AvccMarshal(buf, sps, nalus[1]);
  } else {
    stsd.avc1.type = Le32Type("hvc1");
    hevc::decode_sps((BYTE *)sps.data, sps.size, trak_.tkhd.width,
                     trak_.tkhd.height, fps);
    n = HvccMarshal(buf, sps, nalus[1], nalus[2]);
  }
  stsd.avc1.width = trak_.tkhd.width;
  stsd.avc1.height = trak_.tkhd.height;
  stsdv_ = std::string(stsd.Marshal(n), sizeof(box::stsdv));
  stsdv_.append(buf, n);
  id_ = 1;
  return stsd.avc1.type;
}

inline u32 Trak::MakeAudio(char *accspec) {
  assert(id_ == 0);
  id_ = 2;
  box::stsda stsd;
  stsdv_.append(stsd.Marshal(accspec), sizeof(box::stsda));
  return 0;
}

} // namespace libmp4