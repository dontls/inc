#pragma once

#include "mp4box.hpp"
namespace libmp4 {
// mp4
// ftyp+mdat(size+type,size+data,size+data...)+moov
class Writer {
private:
  FILE *file_;
  box::mdat mdat_;
  Trak trakv_;
  // Trak traka_;
public:
  Writer() : file_(nullptr), mdat_{0} {}
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
  ftyp.compat3 = trakv_.MakeVideo(nalus);
  mdat_.type = Le32Type("mdat");
  // 未转换，sample写入计数
  mdat_.size = sizeof(box::ftyp) + sizeof(box::mdat);
  fwrite(ftyp.Marshal(), sizeof(ftyp), 1, file_);
  return fwrite(&mdat_, sizeof(mdat_), 1, file_);
}

inline size_t Writer::WriteBoxMoov() {
  box::moov moov = {0};
  int len = trakv_.Marshal();
  // int len1 = traka_.Marshal();
  moov.mvhd.duration = trakv_.Duration();
  fwrite(moov.Marshal(len), sizeof(box::moov), 1, file_);
  fwrite(trakv_.Value(), len, 1, file_);
  // fwrite(traka_.Value(), len1, 1, file_);
  //  mdat 头
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
  if (iskey && mdat_.type == 0) {
    WriteBoxFtyp(nalus);
  }
  if (mdat_.type == 0) {
    return 0;
  }
  // stbl信息
  trakv_.AppendSample(ts, mdat_.size, len);
  // mdat数据
  uint32_t slen = Htobe32(len - 4);
  fwrite(&slen, sizeof(uint32_t), 1, file_);
  return fwrite(ptr + 4, len - 4, 1, file_);
}
} // namespace libmp4
