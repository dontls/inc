#pragma once

#include "mp4box.hpp"
#include <string>
namespace libmp4 {
// mp4
// ftyp+mdat(size+type,size+data,size+data...)+moov
class Writer {
private:
  FILE *file_;
  int64_t firts_;
  int64_t lsts_;
  box::moov moov_;
  box::mdat mdat_;
  box::stbl stbl_;

public:
  Writer() : file_(nullptr), firts_(0), lsts_(0), moov_{0}, mdat_{0} {}
  Writer(const char *filename) : Writer() { this->Open(filename); }
  ~Writer() { this->Close(); }
  bool Open(const char *filename) {
    file_ = fopen(filename, "wb+");
    return file_ != NULL;
  }
  void Close();
  int WriteVideo(int64_t ts, bool iskey, char *data, size_t len);

private:
  size_t WriteBoxFtyp(std::vector<nalu::Value> &nalus);
  size_t WriteBoxMoov();
};

inline size_t Writer::WriteBoxFtyp(std::vector<nalu::Value> &nalus) {
  box::ftyp ftyp;
  auto &tkhd = moov_.trakv.tkhd;
  box::stbl_decode(nalus, stbl_, tkhd.width, tkhd.height);
  ftyp.compat3 = stbl_.stsd.avc1.type;
  mdat_.type = Le32Type("mdat");
  // 未转换，sample写入计数
  mdat_.size = sizeof(box::ftyp) + sizeof(box::mdat);
  fwrite(ftyp.Marshal(), sizeof(ftyp), 1, file_);
  return fwrite(&mdat_, sizeof(mdat_), 1, file_);
}

inline size_t Writer::WriteBoxMoov() {
  moov_.mvhd.duration = Htobe32(lsts_ - firts_);
  moov_.trakv.tkhd.duration = moov_.mvhd.duration;
  moov_.trakv.mdia.mdhd.duration = moov_.mvhd.duration;
  int len = stbl_.Marshal();
  // moov
  fwrite(moov_.Marshal(len), sizeof(box::moov), 1, file_);
  fwrite(stbl_.Value(), len, 1, file_);
  // mdat 头
  fseek(file_, sizeof(box::ftyp), SEEK_SET);
  mdat_.size = Htobe32(mdat_.size - sizeof(box::ftyp));
  return fwrite(&mdat_, sizeof(mdat_), 1, file_);
}

inline void Writer::Close() {
  if (file_) {
    WriteBoxMoov();
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
    WriteBoxFtyp(nalus);
    firts_ = ts;
    lsts_ = ts;
  }
  if (firts_ == 0) {
    return 0;
  }
  // stbl信息
  stbl_.AppendSample(ts - lsts_, mdat_.size, len);
  lsts_ = ts;
  // mdat数据
  uint32_t slen = Htobe32(len - 4);
  fwrite(&slen, sizeof(uint32_t), 1, file_);
  return fwrite(ptr + 4, len - 4, 1, file_);
}
} // namespace libmp4
