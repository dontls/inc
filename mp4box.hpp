#pragma once

#include "byte.hpp"
#include "sps.hpp"
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
struct stsd {
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

inline const char *stsd::Marshal(u32 n) {
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
  size = Htobe32(n + sizeof(stsd));
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
class stbl {
public:
  box::stsd stsd;

private:
  box::stts stts;
  box::stss stss;
  box::stsc stsc;
  box::stsz stsz;
  box::stco stco;
  int total_;
  int count_;
  u32 *sample_[4];
  std::string str;

public:
  stbl()
      : stsd{0}, stts{0}, stss{0}, stsc{0}, stsz{0}, stco{0}, total_(0),
        count_(0), sample_{0} {}
  void AppendSample(u32 dur, u32 &offset, u32 length);
  void MarshalStsd(char *buf, int n);
  int Marshal();
  const char *Value() { return str.c_str(); }
};

inline void stbl::AppendSample(u32 dur, u32 &offset, u32 length) {
  if (count_ == total_) {
    total_ += 256;
    for (int i = 0; i < 4; i++) {
      sample_[i] = (u32 *)realloc(sample_[i], total_ * sizeof(u32));
    }
  }
  sample_[0][count_] = Htobe32(dur);    // stts
  sample_[1][count_] = Htobe32(1);      // stss
  sample_[2][count_] = Htobe32(length); // stsz
  sample_[3][count_] = Htobe32(offset); // stco
  count_++;
  offset += length;
}

inline void stbl::MarshalStsd(char *buf, int n) {
  str = std::string(stsd.Marshal(n), sizeof(stsd));
  str.append(buf, n);
}

inline int stbl::Marshal() {
  int samplesize = sizeof(u32) * count_;
  str.append(stts.Marshal(count_), sizeof(stts));
  if (count_ > 0) {
    str.append((const char *)sample_[0], samplesize);
  }
  str.append(stss.Marshal(count_), sizeof(stss));
  if (count_ > 0) {
    str.append((const char *)sample_[1], samplesize);
  }
  str.append(stsc.Marshal(), sizeof(stsc));
  str.append(stsz.Marshal(count_), sizeof(stsz));
  if (count_ > 0) {
    str.append((const char *)sample_[2], samplesize);
  }
  str.append(stco.Marshal(count_), sizeof(stco));
  if (count_ > 0) {
    str.append((const char *)sample_[3], samplesize);
  }
  return int(str.length());
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
  void Marshal(u32 length);
};

/**
 * @brief
 *
 * @param length stbl
 */

inline void trak::Marshal(u32 length) {
  // tkhd
  tkhd.size = Htobe32_Sizeof(tkhd);
  tkhd.type = Le32Type("tkhd");
  tkhd.flags[2] = 0xF;
  tkhd.trackid = Htobe32(1);
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
  mdia.minf.vmhd.type = Le32Type("vmhd");
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
  trak trakv;
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
  trakv.Marshal(len1);
  return this;
}

struct mdat {
  uint32_t size;
  uint32_t type;
};
#pragma pack()

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

inline int AvccMarshal(char *buf, std::string &sps, std::string &pps) {
  u32 spslen = sps.length(), ppslen = pps.length();
  // 附加avcc信息
  avcc *avc = (avcc *)(buf);
  avc->type = Le32Type("avcC");
  avc->config_ver = 1;
  avc->avc_profile = spslen > 1 ? sps[1] : 0;
  avc->profile_compat = spslen > 2 ? sps[2] : 0;
  avc->avc_level = spslen > 3 ? sps[3] : 0;
  avc->nalulen = 0xFF;
  int i = offsetof(avcc, sps_num);
  buf[i++] = char((0x7 << 5) | (1 << 0));
  buf[i++] = (spslen << 8) & 0xff;
  buf[i++] = spslen & 0xff;
  ::memcpy(&buf[i], sps.c_str(), spslen);
  i += spslen;
  buf[i++] = 1;
  buf[i++] = (ppslen << 8) & 0xff;
  buf[i++] = ppslen & 0xff;
  ::memcpy(&buf[i], pps.c_str(), ppslen);
  i += ppslen;
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

inline int HvccMarshal(char *buf, std::string &sps, std::string &pps,
                       std::string &vps) {
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
  // hvc->avgFrameRate = (u16)(htonl(mp4->frate) >> 16);
  hvc->avgFrameRate = 0;
  // hvc->constantFrameRate    = 0;
  hvc->numTemporalLayers = 1;
  hvc->temporalIdNested = 1;
  hvc->lengthSizeMinusOne = 3;
  hvc->numOfArrays = 3;
  int i = sizeof(hvcc);
  u32 spslen = sps.length(), ppslen = pps.length(), vpslen = vps.length();
  buf[i++] = 32;
  buf[i++] = 0x00;
  buf[i++] = 0x01;
  buf[i++] = (vpslen >> 8) & 0xff;
  buf[i++] = vpslen & 0xff;
  memcpy(&buf[i], vps.c_str(), vpslen);
  i += vpslen;

  buf[i++] = 33;
  buf[i++] = 0x00;
  buf[i++] = 0x01;
  buf[i++] = (spslen >> 8) & 0xff;
  buf[i++] = spslen & 0xff;
  memcpy(&buf[i], sps.c_str(), spslen);
  i += spslen;

  buf[i++] = 34;
  buf[i++] = 0x00;
  buf[i++] = 0x01;
  buf[i++] = (ppslen >> 8) & 0xff;
  buf[i++] = ppslen & 0xff;
  memcpy(&buf[i], pps.c_str(), ppslen);
  i += ppslen;
  hvc->size = Htobe32(i);
  return i;
}

inline const void stbl_decode(std::vector<nalu::Value> &nalus, box::stbl &stbl,
                              u32 &w, u32 &h) {
  int fps = 0, n = 0;
  char buf[256] = {0};
  std::string sps = std::string(nalus[0].data + 4, nalus[0].size - 4);
  std::string pps = std::string(nalus[1].data + 4, nalus[1].size - 4);
  if (sps[0] == 0x67) {
    stbl.stsd.avc1.type = Le32Type("avc1");
    avc::decode_sps((BYTE *)sps.c_str(), sps.length(), w, h, fps);
    n = AvccMarshal(buf, sps, pps);
  } else {
    stbl.stsd.avc1.type = Le32Type("hvc1");
    std::string vps = std::string(nalus[2].data - 4, nalus[1].size - 4);
    hevc::decode_sps((BYTE *)sps.c_str(), sps.length(), w, h, fps);
    n = HvccMarshal(buf, sps, pps, vps);
  }
  stbl.stsd.avc1.width = w;
  stbl.stsd.avc1.height = h;
  stbl.MarshalStsd(buf, n);
}
} // namespace box

} // namespace libmp4