#pragma once

#include "mp4box.hpp"

namespace libfile {

static u32 _version_flag(u32 v, u32 f) {
  v = (v << 24) | f;
  return v;
}
namespace box {
#pragma pack(1)
struct mvex {
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
  void *Marshal();
};

inline void *mvex::Marshal() {
  size = libmp4::HTOBE32_SIZEOF(mvex);
  type = libmp4::LE32TYPE("mvex");
  trex.size = libmp4::HTOBE32_SIZEOF(trex);
  trex.type = libmp4::LE32TYPE("trex");
  trex.verflags = libmp4::HTOBE32(_version_flag(0, 0));
  trex.default_sample_descriptionIndex = libmp4::HTOBE32(1);
  trex.track_id = libmp4::HTOBE32(1);
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
      u32 trackId; // 写入时赋值
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
  void Marshal();
};

inline void moof::Marshal() {
  size = libmp4::HTOBE32(sizeof(moof) - sizeof(libmp4::box::mdat));
  type = libmp4::LE32TYPE("moof");

  mfhd.size = libmp4::HTOBE32_SIZEOF(mfhd);
  mfhd.type = libmp4::LE32TYPE("mfhd");
  mfhd.verflags = libmp4::HTOBE32(_version_flag(0, 0));
  // mfhd.sequenceNumber = libmp4::HTOBE32(seq);

  traf.size = libmp4::HTOBE32_SIZEOF(traf);
  traf.type = libmp4::LE32TYPE("traf");

  traf.tfhd.size = libmp4::HTOBE32_SIZEOF(traf.tfhd);
  traf.tfhd.type = libmp4::LE32TYPE("tfhd");
  traf.tfhd.verflags = libmp4::HTOBE32(_version_flag(0, 0x020000));
  // traf.tfhd.trackId = libmp4::HTOBE32(trackId);

  traf.tfdt.size = libmp4::HTOBE32_SIZEOF(traf.tfdt);
  traf.tfdt.type = libmp4::LE32TYPE("tfdt");
  traf.tfdt.verflags = libmp4::HTOBE32(_version_flag(0, 0));
  // traf.tfdt.decode_time = libmp4::HTOBE32(traf.tfdt.decode_time);

  traf.trun.size = libmp4::HTOBE32_SIZEOF(traf.trun);
  traf.trun.type = libmp4::LE32TYPE("trun");
  traf.trun.verflags = libmp4::HTOBE32(_version_flag(1, 0xf01));
  traf.trun.data_offset = libmp4::HTOBE32_SIZEOF(moof); // moof+mdat_header

  traf.trun.sample_count = libmp4::HTOBE32(1);
  traf.trun.sample.time_offset = 0;

  mdatv.type = libmp4::LE32TYPE("mdat");
}
} // namespace box
#pragma pack()
// fmp4
// ftyp+moov+(moof+mdat,moof+mdat,....)
class FMp4 {
private:
  FILE *file_;
  int64_t firts_, lsts_;
  uint32_t seq_;
  box::moof moof_;

public:
  FMp4() : file_(nullptr), firts_(0), lsts_(0), seq_(0) {}
  FMp4(const char *filename) : FMp4() { this->Open(filename); }
  ~FMp4() { this->Close(); }
  bool Open(const char *filename) {
    file_ = fopen(filename, "wb+");
    return file_ != NULL;
  }
  void Close();
  int Write(int64_t ts, uint8_t ftype, char *data, int len);

private:
  size_t WriteBoxFtypMoov(nalu::Vector &nalus) {
    libmp4::box::ftyp ftyp = {0};
    libmp4::box::moov moov = {0};
    box::mvex mvex = {0};
    libmp4::Trak trakv, traka;
    ftyp.compat3 = trakv.MakeVideo(nalus);
    traka.MakeAudio(nullptr);
    int vlen = trakv.Marshal(0);
    int alen = traka.Marshal(0);
    fwrite(ftyp.Marshal(), sizeof(ftyp), 1, file_);
    fwrite(moov.Marshal(vlen + sizeof(box::mvex), alen), sizeof(moov), 1,
           file_);
    fwrite(mvex.Marshal(), sizeof(box::mvex), 1, file_);
    fwrite(trakv.Value(), vlen, 1, file_);
    return fwrite(traka.Value(), alen, 1, file_);
  }
};

inline void FMp4::Close() {
  if (!file_) {
    return;
  }
  // 更新时长
  u32 durOffset = sizeof(libmp4::box::ftyp) + 32;
  u32 dur = libmp4::HTOBE32(lsts_ - firts_);
  fseek(file_, durOffset, SEEK_SET);
  fwrite(&dur, sizeof(u32), 1, file_);
  fclose(file_);
  file_ = nullptr;
}

// video
inline int FMp4::Write(int64_t ts, uint8_t ftype, char *data, int len) {
  if (file_ == nullptr) {
    return -1;
  }
  nalu::Vector nalus;
  char *ptr = nalu::Split(data, len, nalus);
  if (firts_ == 0 && ftype == 1) {
    this->WriteBoxFtypMoov(nalus);
    moof_.Marshal();
    firts_ = lsts_ = ts;
  }
  if (firts_ == 0) {
    return 0;
  }
  // 写入moof
  moof_.mfhd.sequenceNumber = libmp4::HTOBE32(seq_++);
  moof_.mdatv.size = libmp4::HTOBE32(len + 4 + sizeof(libmp4::box::mdat));
  moof_.traf.tfdt.decode_time = libmp4::HTOBE32(ts - firts_);
  moof_.traf.trun.sample.duration = libmp4::HTOBE32(ts - lsts_);
  moof_.traf.trun.sample.size = libmp4::HTOBE32(len + 4);
  if (ftype < 3) {
    moof_.traf.trun.sample.flags =
        libmp4::HTOBE32(ftype == 1 ? 0x02000000 : 0x00010000);
    moof_.traf.tfhd.trackId = libmp4::HTOBE32(1);
  } else {
    moof_.traf.tfhd.trackId = libmp4::HTOBE32(2);
    moof_.traf.trun.sample.flags = 0;
  }
  lsts_ = ts;
  fwrite(&moof_, sizeof(box::moof), 1, file_);
  // 写入mdat
  u32 slen = libmp4::HTOBE32(len);
  fwrite(&slen, sizeof(u32), 1, file_);
  return int(fwrite(ptr, len, 1, file_));
}
} // namespace libfile
