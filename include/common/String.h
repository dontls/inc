#ifndef __STRING_H__
#define __STRING_H__

#include <string>
#include <vector>

inline std::vector<std::string> string_split(const std::string& s, const std::string& c)
{
    std::vector<std::string> v;
    std::string::size_type   pos1, pos2;
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
#endif
