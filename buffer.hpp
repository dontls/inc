#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <mutex>
#include <string>

namespace libyte {
class Buffer {
private:
  std::mutex lock_;
  char *ptr_;
  size_t total_;
  size_t offset_;

public:
  Buffer() : offset_(0) {
    total_ = 2048;
    ptr_ = new char[total_];
  }
  Buffer(size_t n) : offset_(0) {
    total_ = n;
    ptr_ = new char[total_];
  }
  ~Buffer() {
    if (ptr_) {
      delete[] ptr_;
      ptr_ = NULL;
    }
  }

  bool Empty() { return offset_ == 0; }

  size_t Cap() { return total_; }

  size_t Len() { return offset_; }

  char *Bytes() { return ptr_; }

  // 写入指定长度
  size_t Write(char *p, size_t n) {
    assert(NULL != p || 0 > 0);
    return tryWriteByRealloc(p, n);
  }

  // 写入单个字节
  size_t WriteByte(char c) { return Write(&c, 1); }
  //
  size_t WriteUint16(uint16_t n) { return Write((char *)&n, 2); }
  //
  size_t WriteUint32(uint32_t n) { return Write((char *)&n, 4); }
  //
  size_t WriteUint64(uint64_t n) { return Write((char *)&n, 8); }
  // 写入Buffer
  size_t Write(Buffer *b) { return Write(b->Bytes(), b->Len()); }

  // 写入字符串
  size_t WriteString(std::string &s) {
    if (s.length() <= 0) {
      return 0;
    }
    return Write((char *)s.c_str(), s.length());
  }

  // 读数据
  size_t Read(char *p, size_t n) {
    assert(NULL != p && n > 0);
    return tryRead(p, n);
  }

  // 重置
  size_t Reset(size_t offset = 0) {
    offset_ = offset;
    memset(ptr_, 0, total_);
    return offset_;
  }

  // 移除数据
  size_t Remove(size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    size_t s = n < offset_ ? n : offset_;
    offset_ -= s;
    ::memmove(ptr_, ptr_ + s, offset_);
    return s;
  }

  void Realloc(size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    offset_ = 0;
    if (total_ < n) {
      total_ = n;
      ptr_ = (char *)realloc(ptr_, total_);
    }
    ::memset(ptr_, 0, total_);
    assert(ptr_);
  }

private:
  size_t tryWriteByRealloc(char *p, size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    size_t s = offset_ + n;
    if (s > total_) {
      total_ += 2 * n;
      ptr_ = (char *)realloc(ptr_, total_);
      assert(ptr_);
    }
    ::memcpy(ptr_ + offset_, p, n);
    offset_ += n;
    return n;
  }

  size_t tryRead(char *p, size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    size_t readSize = n < offset_ ? n : offset_;
    ::memcpy(p, ptr_, readSize);
    offset_ -= readSize;
    ::memmove(ptr_, ptr_ + readSize, offset_);
    return readSize;
  }
};

} // namespace libyte
