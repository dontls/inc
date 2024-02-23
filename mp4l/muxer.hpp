#pragma once

#include "mp4box.hpp"
#include <cstddef>
#include <string.h>
#include <vector>
namespace mp4 {

// mp4
// ftyp+mdat(size+type,size+data,size+data...)+moov
// fmp4
// ftyp+moov+(moof+mdat,moof+mdat,....)

class Box {
 private:
  int64_t firts_;
  int64_t lsts_;
  uint32_t count_;
  uint32_t mdatoff_;
  box::moov moov_;
  box::stbl stbl_;

 public:
  Box(/* args */)
      : firts_(0), lsts_(0), count_(0), mdatoff_(sizeof(box::ftyp) + 8) {}
  ~Box() {}
  std::string Ftyp(std::vector<std::string>& nalus);
  void VideoTrak(uint32_t size, int64_t ts, bool iskey);
  std::string MdatHeader();
  std::string MoovData();
};

/**
 * @brief
 *
 * @param nalus
 * @return std::string
 */
inline std::string Box::Ftyp(std::vector<std::string>& nalus) {
  BYTE* ptr = (BYTE*)nalus[0].c_str();
  int len = nalus[0].length();
  int fps_ = 0;
  box::trak& trakv = moov_.trakv;
  box::ftyp typ;
  if (ptr[0] == 0x67) {
    typ.compat3 = MP4_FOURCC('a', 'v', 'c', '1');
    avc::decode_sps(ptr, len, stbl_.stsd.avc1.width, stbl_.stsd.avc1.height,
                    fps_);
    stbl_.str = stbl_.stsd.Marshal(nalus[0], nalus[1]);
  } else {
    typ.compat3 = MP4_FOURCC('h', 'v', 'c', '1');
    hevc::decode_sps(ptr, len, stbl_.stsd.avc1.width, stbl_.stsd.avc1.height,
                     fps_);
    stbl_.str = stbl_.stsd.Marshal(nalus[0], nalus[1], nalus[2]);
  }
  return typ.Marshal();
}

/**
 * @brief
 *
 * @param size 长度
 * @param ts 时间戳，ms
 */
inline void Box::VideoTrak(uint32_t size, int64_t ts, bool iskey) {
  if (firts_ == 0) {
    firts_ = ts;
    lsts_ = ts;
  }
  count_++;
  if (iskey) {
    stbl_.stssv.Put(count_);
  }
  stbl_.sttsv.Put(1);
  stbl_.sttsv.Put(ts - lsts_);
  stbl_.stszv.Put(size);
  stbl_.stcov.Put(mdatoff_);
  mdatoff_ += size;
  lsts_ = ts;
}

/**
 * @brief
 *
 * @return std::string
 */
inline std::string Box::MdatHeader() {
  box::mdat dat;
  dat.size = htobe32(mdatoff_ - sizeof(box::ftyp));
  dat.type = MP4_FOURCC('m', 'd', 'a', 't');
  return std::string((char*)&dat, 8);
}

/**
 * @brief
 *
 * @return std::string
 */

inline std::string Box::MoovData() {
  std::string& s1 = stbl_.to_string();
  uint32_t dur = htobe32(lsts_ - firts_);
  moov_.mvhd.duration = dur;
  moov_.trakv.tkhd.duration = dur;
  moov_.trakv.mdia.mdhd.duration = dur;
  moov_.trakv.tkhd.width = stbl_.stsd.avc1.width;
  moov_.trakv.tkhd.height = stbl_.stsd.avc1.height;
  moov_.Marshal(s1.length());
  std::string s((char*)&moov_, sizeof(box::moov));
  s.append(s1);
  log_printf("moov size = %lu\n", s.length());
  return s;
}

inline std::vector<std::string> split_nalu(std::string s, std::string& s1) {
  std::vector<std::string> nalus;
  for (;;) {
    std::string::size_type pos = s.find(s1, 4);
    if (pos == std::string::npos) {
      break;
    }
    nalus.push_back(s.substr(4, pos));
    s = s.substr(pos);
  }
  nalus.push_back(s.substr(4));
  return nalus;
}

class Muxer {
 private:
  FILE* file_;
  Box* box_;
  std::string nalustr_;

 public:
  Muxer() : file_(NULL), box_(NULL) {
    char hdr[5] = {0x00, 0x00, 0x00, 0x01};
    nalustr_ = std::string(hdr, 4);
  }

  ~Muxer() { this->Close(); }

  bool Open(const char* filename) {
    file_ = fopen(filename, "wb+");
    return file_ != NULL;
  }

  void Close();
  /**
   * ts毫秒
   */
  int WriteVideo(int64_t ts, bool iskey, char* data, size_t len);
};

inline void Muxer::Close() {
  if (file_ == NULL) {
    return;
  }
  if (box_) {
    std::string s = box_->MoovData();
    fwrite(s.c_str(), s.length(), 1, file_);
    fseek(file_, sizeof(box::ftyp), SEEK_SET);
    std::string v = box_->MdatHeader();
    fwrite(v.c_str(), v.length(), 1, file_);
    delete box_;
    box_ = NULL;
  }
  fclose(file_);
  file_ = NULL;
}

inline int Muxer::WriteVideo(int64_t ts, bool iskey, char* data, size_t len) {
  if (file_ == NULL) {
    return -1;
  }
  std::string s(data, len);
  std::string::size_type pos = s.rfind(nalustr_);
  if (iskey && box_ == NULL) {
    box_ = new Box();
    std::vector<std::string> nalus = split_nalu(s.substr(0, pos), nalustr_);
    std::string v = box_->Ftyp(nalus);
    fwrite(v.c_str(), v.length(), 1, file_);
    std::string dat = box_->MdatHeader();
    fwrite(dat.c_str(), dat.length(), 1, file_);
  }
  if (box_ == NULL) {
    return 0;
  }
  int size = len - pos;
  box_->VideoTrak(size, ts, iskey);
  uint32_t tsize = htobe32(size - 4);
  fwrite(&tsize, sizeof(uint32_t), 1, file_);
  fwrite(data + pos + 4, size - 4, 1, file_);
  // printf("fwrite size %d\n", size);
  return size;
}
}  // namespace mp4