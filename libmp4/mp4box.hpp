
/**
 * @file mp4box.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-01-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "sps.hpp"

#include <stdint.h>
#include <time.h>
#include <string.h>
#include <cstdint>
#include <string>
#include <endian.h>

#define MP4_FOURCC(a, b, c, d)                                                 \
  (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define htobe32_sizeof(x) htobe32(sizeof(x))

#define log_printf printf

namespace libmp4 {

namespace box {

static void set_matrix(uint32_t *matrix) {
  // mvhd_matrix[1] = htonl(65536);
  // matrix[3] = htonl(-65536);
  // matrix[6] = htonl(0x03840000);
  // matrix[8] = htonl(0x40000000);
  matrix[0] = htobe32(0x00010000);
  matrix[4] = htobe32(0x00010000);
  matrix[8] = htobe32(0x40000000);
}

#pragma pack(1)
struct ftyp {
  uint32_t size;
  uint32_t type;
  uint32_t brand;
  uint32_t version;
  uint32_t compat1;
  uint32_t compat2;
  uint32_t compat3;
  uint32_t compat4;
  std::string Marshal();
};

inline std::string ftyp::Marshal() {
  size = htobe32_sizeof(ftyp);
  type = MP4_FOURCC('f', 't', 'y', 'p');
  brand = MP4_FOURCC('i', 's', 'o', 'm');
  version = htole32(512);
  compat1 = MP4_FOURCC('i', 's', 'o', 'm');
  compat2 = MP4_FOURCC('i', 's', 'o', '2');
  compat4 = MP4_FOURCC('m', 'p', '4', '1');
  return std::string((char *)this, sizeof(ftyp));
}

struct Avcc {
  uint32_t size;
  uint32_t type;
  uint8_t config_ver;
  uint8_t avc_profile;
  uint8_t profile_compat;
  uint8_t avc_level;
  uint8_t nalulen;
  uint8_t sps_num;
  uint16_t sps_len;
  uint8_t *sps;
  uint8_t pps_num;
  uint16_t pps_len;
  uint8_t *pps;
};

struct Hvcc {
  uint32_t size;
  uint32_t type;
  uint8_t configurationVersion;
  uint8_t general_profile_idc : 5;
  uint8_t general_tier_flag : 1;
  uint8_t general_profile_space : 2;
  uint32_t general_profile_compatibility_flags;
  uint32_t general_constraint_indicator_flags0;
  uint16_t general_constraint_indicator_flags1;
  uint8_t general_level_idc;
  uint16_t min_spatial_segmentation_idc;
  uint8_t parallelismType;
  uint8_t chromaFormat;
  uint8_t bitDepthLumaMinus8;
  uint8_t bitDepthChromaMinus8;
  uint16_t avgFrameRate;
  uint8_t lengthSizeMinusOne : 2;
  uint8_t temporalIdNested : 1;
  uint8_t numTemporalLayers : 3;
  uint8_t constantFrameRate : 2;
  uint8_t numOfArrays;
};

struct stsd {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  uint8_t flags[3];
  uint32_t entry_count;
  struct {
    uint32_t size; // ahvc1 <- avc1 & hvc1
    uint32_t type;
    uint8_t reserved1[6];
    uint16_t data_refidx;
    uint16_t predefined1;
    uint16_t reserved2;
    uint32_t predefined2[3];
    uint16_t width;
    uint16_t height;
    uint32_t horiz_res;
    uint32_t vert_res;
    uint32_t reserved3;
    uint16_t frame_count;
    uint8_t compressor[32];
    uint16_t depth;
    uint16_t predefined;
  } avc1;
  std::string Marshal(std::string &sps, std::string &pps);
  std::string Marshal(std::string &sps, std::string &pps, std::string vps);
};

inline std::string stsd::Marshal(std::string &sps, std::string &pps) {
  uint32_t spslen = sps.length(), ppslen = pps.length();
  type = MP4_FOURCC('s', 't', 's', 'd');
  entry_count = htobe32(1);
  avc1.type = MP4_FOURCC('a', 'v', 'c', '1');
  avc1.data_refidx = uint16_t(htobe32(1) >> 16);
  avc1.width = uint16_t(htobe32(avc1.width) >> 16);
  avc1.height = uint16_t(htobe32(avc1.height) >> 16);
  avc1.horiz_res = htobe32(0x480000);
  avc1.vert_res = htobe32(0x480000);
  avc1.frame_count = uint16_t(htobe32(1) >> 16);
  avc1.depth = uint16_t(htobe32(24) >> 16);
  avc1.predefined = 0xFFFF;
  uint8_t buf[128] = {0};
  // 附加avcc信息
  Avcc *avc = (Avcc *)(buf);
  avc->type = MP4_FOURCC('a', 'v', 'c', 'C');
  avc->config_ver = 1;
  avc->avc_profile = spslen > 1 ? sps[1] : 0;
  avc->profile_compat = spslen > 2 ? sps[2] : 0;
  avc->avc_level = spslen > 3 ? sps[3] : 0;
  avc->nalulen = 0xFF;
  int i = offsetof(Avcc, sps_num);
  buf[i++] = (0x7 << 5) | (1 << 0);
  buf[i++] = (spslen << 8) & 0xff;
  buf[i++] = spslen & 0xff;
  ::memcpy(&buf[i], sps.c_str(), spslen);
  i += spslen;
  buf[i++] = 1;
  buf[i++] = (ppslen << 8) & 0xff;
  buf[i++] = ppslen & 0xff;
  ::memcpy(&buf[i], pps.c_str(), ppslen);
  i += ppslen;

  avc->size = htobe32(i);
  avc1.size = htobe32(i + sizeof(avc1));
  size = htobe32(i + sizeof(stsd));
  std::string s1((char *)this, sizeof(stsd));
  s1.append(std::string((char *)buf, i));
  log_printf("stsd size = %lu\n", s1.length());
  return s1;
}

inline std::string stsd::Marshal(std::string &sps, std::string &pps,
                                 std::string vps) {
  type = MP4_FOURCC('s', 't', 's', 'd');
  entry_count = htobe32(1);
  avc1.type = MP4_FOURCC('h', 'v', 'c', '1');
  avc1.data_refidx = uint16_t(htobe32(1) >> 16);
  avc1.width = uint16_t(htobe32(avc1.width) >> 16);
  avc1.height = uint16_t(htobe32(avc1.height) >> 16);
  avc1.horiz_res = htobe32(0x480000);
  avc1.vert_res = htobe32(0x480000);
  avc1.frame_count = uint16_t(htobe32(1) >> 16);
  avc1.depth = uint16_t(htobe32(24) >> 16);
  avc1.predefined = 0xFFFF;
  uint8_t buf[254] = {0};
  Hvcc *hvc = (Hvcc *)(buf);
  hvc->type = MP4_FOURCC('h', 'v', 'c', 'C');
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
  // hvc->avgFrameRate = (uint16_t)(htonl(mp4->frate) >> 16);
  hvc->avgFrameRate = 0;
  // hvc->constantFrameRate    = 0;
  hvc->numTemporalLayers = 1;
  hvc->temporalIdNested = 1;
  hvc->lengthSizeMinusOne = 3;
  hvc->numOfArrays = 3;
  int i = sizeof(Hvcc);
  uint32_t spslen = sps.length(), ppslen = pps.length(), vpslen = vps.length();
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

  hvc->size = htobe32(i);
  avc1.size = htobe32(i + sizeof(avc1));
  size = htobe32(i + sizeof(stsd));
  std::string s1((char *)this, sizeof(stsd));
  s1.append(std::string((char *)buf, i));
  return s1;
}

class stValue {
private:
  uint32_t *value_;
  uint32_t total_;
  uint32_t offi_;

private:
  void Realloc(uint32_t n) {
    value_ = (uint32_t *)realloc(value_, n * sizeof(uint32_t));
  }

public:
  stValue() : total_(0), offi_(0) {}
  ~stValue() {
    if (value_) {
      free(value_);
      value_ = NULL;
    }
  }
  void Put(uint32_t v) {
    if (offi_ >= total_) {
      total_ += 128;
      Realloc(total_);
    }
    value_[offi_++] = v;
  }
  void AppendTo(std::string &out) {
    for (uint32_t i = 0; i < offi_; i++) {
      value_[i] = htobe32(value_[i]);
    }
    out.append((char *)value_, offi_ * sizeof(uint32_t));
  }
  uint32_t Count() { return offi_; }
  uint32_t Length() { return offi_ * sizeof(uint32_t); }
};

// sample_deltas
struct stts {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  uint8_t flags[3];
  uint32_t count;
  void append(std::string &out, stValue &v);
};

inline void stts::append(std::string &out, stValue &v) {
  size = htobe32(sizeof(stts) + v.Length());
  type = MP4_FOURCC('s', 't', 't', 's');
  count = htobe32(v.Count() / 2);
  out.append(std::string((char *)this, sizeof(stts)));
  v.AppendTo(out);
  log_printf("stts size = %u\n", be32toh(size));
}

// sample_numbers
struct stss {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  uint8_t flags[3];
  uint32_t count;
  void append(std::string &out, stValue &v);
};

inline void stss::append(std::string &out, stValue &v) {
  size = htobe32(sizeof(stss) + v.Length());
  type = MP4_FOURCC('s', 't', 's', 's');
  count = htobe32(v.Count());
  out.append(std::string((char *)this, sizeof(stss)));
  v.AppendTo(out);
  log_printf("stss size = %u\n", be32toh(size));
}

struct stsc {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  uint8_t flags[3];
  uint32_t count;
  uint32_t first_chunk;
  uint32_t samp_per_chunk;
  uint32_t samp_desc_id;
  void append(std::string &out);
};

inline void stsc::append(std::string &out) {
  size = htobe32_sizeof(stsc);
  type = MP4_FOURCC('s', 't', 's', 'c');
  count = htobe32(1);
  first_chunk = htobe32(1);
  samp_per_chunk = htobe32(1);
  samp_desc_id = htobe32(1);
  out.append(std::string((char *)this, sizeof(stsc)));
  log_printf("stsc size = %u\n", be32toh(size));
}

// sample_sizes
struct stsz {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  uint8_t flags[3];
  uint32_t sample_size;
  uint32_t count;
  void append(std::string &out, stValue &v);
};

inline void stsz::append(std::string &out, stValue &v) {
  size = htobe32(sizeof(stsz) + v.Length());
  type = MP4_FOURCC('s', 't', 's', 'z');
  count = htobe32(v.Count());
  out.append(std::string((char *)this, sizeof(stsz)));
  v.AppendTo(out);
  log_printf("stsz size = %u\n", be32toh(size));
}

// chunk_offsets
struct stco {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  uint8_t flags[3];
  uint32_t count;
  void append(std::string &out, stValue &v);
};

inline void stco::append(std::string &out, stValue &v) {
  size = htobe32(sizeof(stco) + v.Length());
  type = MP4_FOURCC('s', 't', 'c', 'o');
  count = htobe32(v.Count());
  out.append(std::string((char *)this, sizeof(stco)));
  v.AppendTo(out);
  log_printf("stco size = %u\n", be32toh(size));
}

struct stbl {
  std::string str;
  box::stsd stsd;
  box::stValue sttsv;
  box::stts stts;
  box::stValue stssv;
  box::stss stss;
  box::stsc stsc;
  box::stValue stszv;
  box::stsz stsz;
  box::stValue stcov;
  box::stco stco;
  std::string &to_string();
};

inline std::string &stbl::to_string() {
  stts.append(str, sttsv);
  stss.append(str, stssv);
  stsc.append(str);
  stsz.append(str, stszv);
  stco.append(str, stcov);
  return str;
}

struct trak {
  uint32_t size;
  uint32_t type;
  struct {
    uint32_t size;
    uint32_t type;
    uint8_t version;
    uint8_t flags[3];
    uint32_t create_time;
    uint32_t modify_time;
    uint32_t trackid;
    uint32_t reserved1;
    uint32_t duration;
    uint32_t reserved2;
    uint32_t reserved3;
    uint16_t layer;
    uint16_t alternate_group;
    uint16_t volume;
    uint16_t reserved4;
    uint32_t matrix[9];
    uint32_t width;
    uint32_t height;
  } tkhd;
  struct {
    uint32_t size;
    uint32_t type;
    struct {
      uint32_t size;
      uint32_t type;
      uint8_t version;
      uint8_t flags[3];
      uint32_t create_time;
      uint32_t modify_time;
      uint32_t timescale;
      uint32_t duration;
      uint16_t language;
      uint16_t predefined;
    } mdhd;
    struct {
      uint32_t size;
      uint32_t type;
      uint8_t version;
      uint8_t flags[3];
      uint8_t predefined[4];
      uint32_t handler_type;
      uint8_t reserved[12];
      uint8_t name[16];
    } hdlr;
    struct {
      uint32_t size;
      uint32_t type;
      struct {
        uint32_t size;
        uint32_t type;
        uint8_t version;
        uint8_t flags[3];
        uint32_t graph_mode;
        uint32_t opcolor;
      } vmhd;
      struct {
        uint32_t size;
        uint32_t type;
        struct {
          uint32_t size;
          uint32_t type;
          uint8_t version;
          uint8_t flags[3];
          uint32_t entry_count;
          struct {
            uint32_t size;
            uint32_t type;
            uint8_t version;
            uint8_t flags[3];
          } url;
        } dref;
      } dinf;
      struct {
        int32_t size;
        uint32_t type;
      } stbl;
    } minf;
  } mdia;
  void Marshal(uint32_t length);
};

/**
 * @brief
 *
 * @param length stbl
 */

inline void trak::Marshal(uint32_t length) {
  // tkhd
  tkhd.size = htobe32_sizeof(tkhd);
  tkhd.type = MP4_FOURCC('t', 'k', 'h', 'd');
  tkhd.flags[2] = 0xF;
  tkhd.trackid = htobe32(1);
  set_matrix(tkhd.matrix);
  tkhd.width = htobe32(tkhd.width << 16);   //
  tkhd.height = htobe32(tkhd.height << 16); //

  // mdhd
  mdia.mdhd.size = htobe32_sizeof(mdia.mdhd);
  mdia.mdhd.type = MP4_FOURCC('m', 'd', 'h', 'd');
  mdia.mdhd.timescale = htobe32(1000);
  // hdlr
  mdia.hdlr.size = htobe32_sizeof(mdia.hdlr);
  mdia.hdlr.type = MP4_FOURCC('h', 'd', 'l', 'r');
  mdia.hdlr.handler_type = MP4_FOURCC('v', 'i', 'd', 'e');
  ::strcpy((char *)mdia.hdlr.name, "VideoHandler");

  mdia.minf.vmhd.size = htobe32_sizeof(mdia.minf.vmhd);
  mdia.minf.vmhd.type = MP4_FOURCC('v', 'm', 'h', 'd');
  mdia.minf.vmhd.flags[2] = 1;

  mdia.minf.dinf.size = htobe32_sizeof(mdia.minf.dinf);
  mdia.minf.dinf.type = MP4_FOURCC('d', 'i', 'n', 'f');

  mdia.minf.dinf.dref.url.size = htobe32_sizeof(mdia.minf.dinf.dref.url);
  mdia.minf.dinf.dref.url.type = MP4_FOURCC('u', 'r', 'l', ' ');
  mdia.minf.dinf.dref.url.flags[2] = 1;

  mdia.minf.dinf.dref.size = htobe32_sizeof(mdia.minf.dinf.dref);
  mdia.minf.dinf.dref.type = MP4_FOURCC('d', 'r', 'e', 'f');
  mdia.minf.dinf.dref.entry_count = htobe32(1);

  // minf
  mdia.minf.size = htobe32(sizeof(mdia.minf) + length);
  mdia.minf.type = MP4_FOURCC('m', 'i', 'n', 'f');
  log_printf("minf size = %u\n", be32toh(mdia.minf.size));

  // mdia
  mdia.size = htobe32(sizeof(mdia) + length);
  mdia.type = MP4_FOURCC('m', 'd', 'i', 'a');
  log_printf("mdia size = %u\n", be32toh(mdia.size));

  // stbl
  mdia.minf.stbl.size = htobe32(sizeof(mdia.minf.stbl) + length);
  mdia.minf.stbl.type = MP4_FOURCC('s', 't', 'b', 'l');
  log_printf("stbl size = %u\n", be32toh(mdia.minf.stbl.size));

  size = htobe32(sizeof(trak) + length);
  type = MP4_FOURCC('t', 'r', 'a', 'k');
  log_printf("trak size = %u\n", be32toh(size));
}

struct moov {
  uint32_t size;
  uint32_t type;
  struct {
    uint32_t size;
    uint32_t type;
    uint8_t version;
    uint8_t flags[3];
    uint32_t create_time;
    uint32_t modify_time;
    uint32_t timescale;
    uint32_t duration;
    uint32_t playrate;
    uint16_t volume;
    uint8_t reserved[10];
    uint32_t matrix[9];
    uint8_t predefined[24];
    uint32_t next_trackid;
  } mvhd;
  trak trakv;
  void Marshal(uint32_t len1, uint32_t len2 = 0);
};

inline void moov::Marshal(uint32_t len1, uint32_t len2) {
  size = htobe32(sizeof(moov) + len1 + len2);
  type = MP4_FOURCC('m', 'o', 'o', 'v');
  mvhd.size = htobe32_sizeof(mvhd);
  mvhd.type = MP4_FOURCC('m', 'v', 'h', 'd');
  mvhd.create_time = htobe32((uint32_t)time(NULL));
  mvhd.modify_time = mvhd.create_time;
  mvhd.timescale = htobe32(1000);
  // 外部更新
  // mvhd.duration = ;
  mvhd.playrate = htobe32(0x00010000);
  mvhd.volume = uint16_t(htobe32(1));
  set_matrix(mvhd.matrix);
  if (len2 > 0) {
    mvhd.next_trackid = htobe32(3);
  } else {
    mvhd.next_trackid = htobe32(2);
  }
  trakv.Marshal(len1);
}

struct moof {
  uint32_t size;
  uint32_t type;
  struct {
    uint32_t size;
    uint32_t type;
  } mfhd;
  struct {
    uint32_t size;
    uint32_t type;
  } tfdt;
  struct {
    uint32_t size;
    uint32_t type;
  } trun;
};

struct mdat {
  uint32_t size;
  uint32_t type;
};

} // namespace box

#pragma pack()

} // namespace libmp4