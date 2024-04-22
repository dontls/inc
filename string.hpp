#ifndef __STRING_H__
#define __STRING_H__

#include <string>
#include <vector>
#include <sstream>

namespace libstring {
inline std::vector<std::string> Split(const std::string &s,
                                      const std::string &c) {
  std::vector<std::string> v;
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while (std::string::npos != pos2) {
    v.push_back(s.substr(pos1, pos2 - pos1));

    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if (pos1 != s.length())
    v.push_back(s.substr(pos1));
  return v;
}

inline uint32_t SplitToBit(const char *str, const char flag) {
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
} // namespace libstring

#endif
