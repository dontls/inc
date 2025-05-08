#pragma once

#include "mp4box.hpp"
namespace libfile {
// mp4
// ftyp+mdat(size+type,size+data,size+data...)+moov
class Mp4 {
private:
  FILE *file_;
  int64_t firts_, lsts_;
  libmp4::box::mdat mdat_;
  libmp4::Trak trakv_;
  libmp4::Trak traka_;

public:
  Mp4() : file_(nullptr), firts_(0), lsts_(0), mdat_{0} {}
  Mp4(const char *filename) : Mp4() { this->Open(filename); }
  ~Mp4() { this->Close(); }
  bool Open(const char *filename) {
    file_ = fopen(filename, "wb+");
    return file_ != NULL;
  }
  void Close();
  int Write(int64_t ts, uint8_t ftype, char *data, int len);

private:
  size_t WriteBoxFtyp(nalu::Vector &nalus);
  size_t WriteBoxMoov();
};

inline size_t Mp4::WriteBoxFtyp(nalu::Vector &nalus) {
  libmp4::box::ftyp ftyp;
  ftyp.compat3 = trakv_.MakeVideo(nalus);
  traka_.MakeAudio(nullptr);
  mdat_.type = libmp4::LE32TYPE("mdat");
  // 未转换，sample写入计数
  mdat_.size = sizeof(libmp4::box::ftyp) + sizeof(libmp4::box::mdat);
  fwrite(ftyp.Marshal(), sizeof(ftyp), 1, file_);
  return fwrite(&mdat_, sizeof(mdat_), 1, file_);
}

inline size_t Mp4::WriteBoxMoov() {
  libmp4::box::moov moov = {0};
  u32 beDur = libmp4::HTOBE32(lsts_ - firts_);
  int len = trakv_.Marshal(beDur);
  int len1 = traka_.Marshal(beDur);
  moov.mvhd.duration = beDur;
  fwrite(moov.Marshal(len, len1), sizeof(libmp4::box::moov), 1, file_);
  fwrite(trakv_.Value(), len, 1, file_);
  fwrite(traka_.Value(), len1, 1, file_);
  //  mdat 头
  fseek(file_, sizeof(libmp4::box::ftyp), SEEK_SET);
  mdat_.size = libmp4::HTOBE32(mdat_.size - sizeof(libmp4::box::ftyp));
  return fwrite(&mdat_, sizeof(mdat_), 1, file_);
}

inline void Mp4::Close() {
  if (file_) {
    WriteBoxMoov();
    fclose(file_);
    file_ = NULL;
  }
}

// ftype: 1:I, 2:p, 3:aac
inline int Mp4::Write(int64_t ts, uint8_t ftype, char *data, int len) {
  if (file_ == NULL) {
    return -1;
  }
  nalu::Vector nalus;
  char *ptr = nalu::Split(data, len, nalus);
  if (firts_ == 0 && ftype == 1) {
    WriteBoxFtyp(nalus);
    firts_ = lsts_ = ts;
  }
  if (firts_ == 0) {
    return 0;
  }
  libmp4::Trak *trak = ftype < 3 ? &trakv_ : &traka_;
  // stbl信息
  trak->AppendSample(ts - lsts_, mdat_.size, u32(len + 4));
  lsts_ = ts;
  // mdat数据
  uint32_t slen = libmp4::HTOBE32(u32(len));
  fwrite(&slen, sizeof(uint32_t), 1, file_);
  return int(fwrite(ptr, len, 1, file_));
}

} // namespace libfile
