#pragma once

#include "mp4box.hpp"
#include <string>

namespace libfmp4 {

static u32 _version_flag(u32 v, u32 f) {
  v = (v << 24) | f;
  return v;
}
namespace box {
#pragma pack(1)
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
  struct {
    u32 size;
    u32 type;
    struct {
      u32 size;
      u32 type;
      u32 verflags; //
      u32 track_id;
      u32 default_sample_descriptionIndex;
      u32 default_sample_duration;
      u32 default_sample_size;
      u32 default_sample_flags;
    } trex;
  } mvex;
  libmp4::box::trak trakv;
  void *Marshal(u32 len1, u32 len2 = 0);
};

inline void *moov::Marshal(u32 len1, u32 len2) {
  size = libmp4::Htobe32(sizeof(moov) + len1 + len2);
  type = libmp4::Le32Type("moov");
  mvhd.size = libmp4::Htobe32_Sizeof(mvhd);
  mvhd.type = libmp4::Le32Type("mvhd");
  mvhd.create_time = libmp4::Htobe32((u32)time(NULL));
  mvhd.modify_time = mvhd.create_time;
  // mvhd.duration = ; // 外部更新
  mvhd.timescale = libmp4::Htobe32(1000);

  mvhd.playrate = libmp4::Htobe32(0x00010000);
  mvhd.volume = u16(libmp4::Htobe32(1));
  libmp4::Matrix(mvhd.matrix);
  if (len2 > 0) {
    mvhd.next_trackid = libmp4::Htobe32(3);
  } else {
    mvhd.next_trackid = libmp4::Htobe32(2);
  }

  mvex.size = libmp4::Htobe32_Sizeof(mvex);
  mvex.type = libmp4::Le32Type("mvex");
  mvex.trex.size = libmp4::Htobe32_Sizeof(mvex.trex);
  mvex.trex.type = libmp4::Le32Type("trex");
  mvex.trex.verflags = libmp4::Htobe32(_version_flag(0, 0));
  mvex.trex.default_sample_descriptionIndex = libmp4::Htobe32(1);
  mvex.trex.track_id = libmp4::Htobe32(1);
  trakv.Marshal(len1);
  return this;
}

struct moof {
  u32 size;
  u32 type;
  struct {
    u32 size;
    u32 type;
    u32 verflags;
    u32 sequenceNumber;
  } mfhd;
  struct {
    u32 size;
    u32 type;
    struct {
      u32 size;
      u32 type;
      u32 verflags;
      u32 trackId;
    } tfhd;
    struct {
      u32 size;
      u32 type;
      u32 verflags; // ver=1 64位decode_time, 默认0
      u32 decode_time;
    } tfdt;
    struct {
      u32 size;
      u32 type;
      u32 verflags;
      u32 sample_count;
      u32 data_offset; // 默认0
      struct {
        u32 duration;
        u32 size;
        u32 flags;
        u32 time_offset;
      } sample;
    } trun;
  } traf;
  libmp4::box::mdat mdatv;
  void *Marshal(u32 trackId, u32 seq);
};

inline void *moof::Marshal(u32 trackId, u32 seq) {
  size = libmp4::Htobe32(sizeof(moof) - sizeof(libmp4::box::mdat));
  type = libmp4::Le32Type("moof");

  mfhd.size = libmp4::Htobe32_Sizeof(mfhd);
  mfhd.type = libmp4::Le32Type("mfhd");
  mfhd.verflags = libmp4::Htobe32(_version_flag(0, 0));
  mfhd.sequenceNumber = libmp4::Htobe32(seq);

  traf.size = libmp4::Htobe32_Sizeof(traf);
  traf.type = libmp4::Le32Type("traf");

  traf.tfhd.size = libmp4::Htobe32_Sizeof(traf.tfhd);
  traf.tfhd.type = libmp4::Le32Type("tfhd");
  traf.tfhd.verflags = libmp4::Htobe32(_version_flag(0, 0x020000));
  traf.tfhd.trackId = libmp4::Htobe32(trackId);

  traf.tfdt.size = libmp4::Htobe32_Sizeof(traf.tfdt);
  traf.tfdt.type = libmp4::Le32Type("tfdt");
  traf.tfdt.verflags = libmp4::Htobe32(_version_flag(0, 0));
  // traf.tfdt.decode_time = libmp4::Htobe32(traf.tfdt.decode_time);

  traf.trun.size = libmp4::Htobe32_Sizeof(traf.trun);
  traf.trun.type = libmp4::Le32Type("trun");
  traf.trun.verflags = libmp4::Htobe32(_version_flag(1, 0xf01));
  traf.trun.data_offset = libmp4::Htobe32_Sizeof(moof); // moof+mdat_header

  traf.trun.sample_count = libmp4::Htobe32(1);
  traf.trun.sample.time_offset = 0;

  mdatv.type = libmp4::Le32Type("mdat");
  mdatv.size = libmp4::Htobe32(mdatv.size + sizeof(libmp4::box::mdat));
  return this;
}
} // namespace box
#pragma pack()
// fmp4
// ftyp+moov+(moof+mdat,moof+mdat,....)
class Writer {
private:
  FILE *file_;
  int64_t firts_;
  int64_t lsts_;
  uint32_t seq_;

public:
  Writer() : file_(nullptr), firts_(0), lsts_(0), seq_(0) {}
  Writer(const char *filename) : Writer() { this->Open(filename); }
  ~Writer() { this->Close(); }
  bool Open(const char *filename) {
    file_ = fopen(filename, "wb+");
    return file_ != NULL;
  }
  void Close();
  int WriteVideo(int64_t ts, bool iskey, char *data, size_t len);

private:
  size_t WriteBoxFtypMoov(std::vector<nalu::Value> &nalus) {
    std::string sps = std::string(nalus[0].data + 4, nalus[0].size - 4);
    std::string pps = std::string(nalus[1].data + 4, nalus[1].size - 4);
    libmp4::box::stbl stbl;
    const char *compat = libmp4::box::stbl_sps_decode(sps, stbl.stsd);
    int w = stbl.stsd.avc1.width, h = stbl.stsd.avc1.height;
    if (strcmp(compat, "avc1") == 0) {
      stbl.str = stbl.stsd.Marshal(sps, pps);
    } else {
      std::string vps = std::string(nalus[2].data - 4, nalus[1].size + 4);
      stbl.str = stbl.stsd.Marshal(sps, pps, vps);
    }
    libmp4::box::ftyp pftyp;
    pftyp.compat3 = libmp4::Le32Type(compat);
    box::moov moov = {0};
    moov.mvhd.duration = 0;
    moov.trakv.tkhd.duration = 0;
    moov.trakv.mdia.mdhd.duration = 0;
    moov.trakv.tkhd.width = w;
    moov.trakv.tkhd.height = h;
    int slen = stbl.Marshal();
    fwrite(pftyp.Marshal(), sizeof(pftyp), 1, file_);
    fwrite(moov.Marshal(slen), sizeof(box::moov), 1, file_);
    return fwrite(stbl.str.c_str(), slen, 1, file_);
  }
};

inline void Writer::Close() {
  if (file_) {
    // 更新时长
    u32 durOffset = sizeof(libmp4::box::ftyp) + 32;
    u32 dur = libmp4::Htobe32(lsts_ - firts_);
    fseek(file_, durOffset, SEEK_SET);
    fwrite(&dur, sizeof(u32), 1, file_);
    fclose(file_);
    file_ = NULL;
  }
}

inline int Writer::WriteVideo(int64_t ts, bool iskey, char *data, size_t len) {
  if (file_ == NULL) {
    return -1;
  }
  std::vector<nalu::Value> nalus;
  char *ptr = nalu::Split(data, len, nalus);
  if (iskey && firts_ == 0) {
    this->WriteBoxFtypMoov(nalus);
    firts_ = ts;
    lsts_ = ts;
  }
  if (firts_ == 0) {
    return 0;
  }
  // 写入moof
  box::moof moof;
  moof.mdatv.size = len;
  moof.traf.tfdt.decode_time = libmp4::Htobe32(ts - firts_);
  moof.traf.trun.sample.duration = libmp4::Htobe32(ts - lsts_);
  moof.traf.trun.sample.size = libmp4::Htobe32(len);
  if (iskey) {
    moof.traf.trun.sample.flags = libmp4::Htobe32(0x02000000);
  } else {
    moof.traf.trun.sample.flags = libmp4::Htobe32(0x00010000);
  }
  lsts_ = ts;
  fwrite(moof.Marshal(1, seq_++), sizeof(moof), 1, file_);
  // 写入mdat
  u32 slen = libmp4::Htobe32(len - 4);
  fwrite(&slen, sizeof(u32), 1, file_);
  return fwrite(ptr + 4, len - 4, 1, file_);
}
} // namespace libfmp4
