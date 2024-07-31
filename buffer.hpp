#pragma once

#include <assert.h>
#include <string.h>

#include <mutex>
#include <string>
#include <vector>

namespace libyte {
class Buffer {
private:
  std::mutex lock_;
  std::vector<char> v_;

public:
  Buffer() {}
  Buffer(size_t n) { v_.reserve(n); }
  ~Buffer() {}
  // 空
  bool Empty() { return v_.empty(); }
  // 容量
  size_t Cap() { return v_.capacity(); }
  // 数据长度
  size_t Len() { return v_.size(); }
  // 数据
  char *Bytes() { return v_.data(); }
  // 写入指定长度
  size_t Write(char *p, size_t n) {
    assert(NULL != p || n > 0);
    return tryWrite(p, n);
  }
  // 写入
  size_t Write(const char *s) { return Write((char *)s, strlen(s)); }
  // 写入
  template <typename T> size_t Write(T b) {
    return Write((char *)&b, sizeof(T));
  }
  // 写入Buffer
  size_t Write(Buffer *b) { return Write(b->Bytes(), b->Len()); }
  // 写入字符串
  size_t Write(std::string &s) { return Write((char *)s.c_str(), s.length()); }
  // 读数据
  size_t Read(char *p, size_t n) {
    assert(NULL != p && n > 0);
    return tryRead(p, n);
  }
  // 重置
  void Reset(size_t offset = 0) { v_.resize(offset, 0); }
  // 移除数据
  size_t Remove(size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    n = n > Len() ? Len() : n;
    v_.erase(v_.begin(), v_.begin() + n);
    return n;
  }
  // 修改容量
  void Realloc(size_t n, bool force = false) {
    std::lock_guard<std::mutex> lock(lock_);
    if (force || Cap() < n) {
      v_.reserve(n);
    }
    v_.resize(0);
  }

private:
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  size_t tryWrite(char *p, size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    size_t s = Cap();
    if (Len() + n > s) {
      s += 2 * n;
      v_.reserve(s);
    }
    v_.insert(v_.end(), p, p + n);
    return n;
  }

  size_t tryRead(char *p, size_t n) {
    std::lock_guard<std::mutex> lock(lock_);
    n = n > Len() ? Len() : n;
    if (n > 0) {
      ::memcpy(p, Bytes(), n);
      v_.erase(v_.begin(), v_.begin() + n);
    }
    return n;
  }
};

} // namespace libyte
