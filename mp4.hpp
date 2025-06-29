#pragma once

#include "mp4box.hpp"
namespace libfile {
// mp4
// ftyp+mdat(size+type,size+data,size+data...)+moov
class Mp4 {
private:
  FILE *file_;
  libmp4::box::mdat mdat_;
  libmp4::Trak trakv_;
  libmp4::Trak traka_;

public:
  Mp4() : file_(nullptr), mdat_{0} {}
  Mp4(const char *filename) : Mp4() { this->Open(filename); }
  ~Mp4() { this->Close(); }
  bool Open(const char *filename) {
    file_ = fopen(filename, "wb+");
    return file_ != NULL;
  }
  void Close();
  int WriteFrame(int64_t ts, uint8_t ftype, char *data, size_t len);

private:
  size_t WriteFtyp();
  size_t WriteMoov();
};

inline size_t Mp4::WriteFtyp() {
  libmp4::box::ftyp ftyp{};
  mdat_.type = libmp4::LE32TYPE("mdat");
  mdat_.size = sizeof(ftyp) + sizeof(mdat_);
  fwrite(ftyp.Marshal(), sizeof(ftyp), 1, file_);
  return fwrite(&mdat_, sizeof(mdat_), 1, file_);
}

inline size_t Mp4::WriteMoov() {
  libmp4::box::moov moov = {0};
  int len = trakv_.Marshal(&moov.mvhd.duration);
  int len1 = traka_.MakeAudio()->Marshal(nullptr);
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
    WriteMoov();
    fclose(file_);
    file_ = NULL;
  }
}

// ftype: 1:I, 2:p, 3:aac
inline int Mp4::WriteFrame(int64_t ts, uint8_t ftype, char *data, size_t len) {
  if (file_ == NULL) {
    return -1;
  }
  if (ftype < 3) {
    nalu::Units nalus;
    data = nalu::Split(data, len, nalus);
    if (mdat_.type == 0 && ftype == 1) {
      trakv_.MakeVideo(nalus);
      WriteFtyp();
    }
  }
  if (mdat_.type == 0) {
    return 0;
  }
  if (ftype == 3) {
    // stbl信息
    traka_.AppendSample(ts, mdat_.size, u32(len), false);
    return fwrite(data, len, 1, file_);
  }
  // stbl信息
  trakv_.AppendSample(ts, mdat_.size, u32(len + 4), ftype == 1);
  // mdat数据
  uint32_t slen = libmp4::HTOBE32(u32(len));
  fwrite((char *)&slen, sizeof(uint32_t), 1, file_);
  return fwrite(data, len, 1, file_);
}

} // namespace libfile
