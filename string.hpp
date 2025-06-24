#ifndef __STRING_H__
#define __STRING_H__

#include <string>
#include <vector>
#include <sstream>
#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace libstr {

class Fmt {
private:
  /* data */
public:
  template <typename T> Fmt(const char *fmt, T v);
  const char *c_str() const { return buf_; }

  int length() const { return length_; }

private:
  char buf_[32] = {};
  int length_ = 0;
};

template <typename T> Fmt::Fmt(const char *fmt, T v) {
  length_ = snprintf(buf_, sizeof(buf_), fmt, v);
  assert(static_cast<size_t>(length_) < sizeof(buf_));
}

template Fmt::Fmt(const char *fmt, char);
template Fmt::Fmt(const char *fmt, short);
template Fmt::Fmt(const char *fmt, unsigned short);
template Fmt::Fmt(const char *fmt, int);
template Fmt::Fmt(const char *fmt, unsigned int);
template Fmt::Fmt(const char *fmt, long);
template Fmt::Fmt(const char *fmt, unsigned long);
template Fmt::Fmt(const char *fmt, long long);
template Fmt::Fmt(const char *fmt, unsigned long long);
template Fmt::Fmt(const char *fmt, float);
template Fmt::Fmt(const char *fmt, double);

typedef std::vector<std::string> Strings;

inline Strings Split(const std::string &s, const char *p) {
  Strings v;
  size_t pos = 0, end = 0;
  while ((end = s.find(p, pos)) != std::string::npos) {
    v.push_back(s.substr(pos, end - pos));
    pos = end + strlen(p);
  }
  v.push_back(s.substr(pos));
  return v;
}

inline uint32_t Split2Bit(const char *str, const char flag) {
  // 使用 std::stringstream 将字符串分割成单词
  std::stringstream ss(str);
  std::string token;
  uint32_t ret = 0;
  while (std::getline(ss, token, flag)) {
    // 使用 std::stoi 将单词转换为 int 类型
    int num = std::stoi(token);
    if (num > 0 && num < 33) {
      num -= 1;
      ret |= (1 << num);
    }
  }
  return ret;
}
} // namespace libstr

#endif
