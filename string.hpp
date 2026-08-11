#ifndef __STRING_H__
#define __STRING_H__

#include <string>
#include <vector>
#include <sstream>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cstdarg>

namespace libstring {

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

// 参考golang实现
inline std::string Sprintf(const char *fmt, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  if (n < 0) {
    return "";
  }
  if (static_cast<size_t>(n) >= sizeof(buffer)) {
    std::string s(n + 1, '\0');
    va_start(args, fmt);
    vsnprintf(&s[0], s.size(), fmt, args);
    va_end(args);
    return s;
  }
  return std::string(buffer, n);
}

} // namespace libstring

#endif
