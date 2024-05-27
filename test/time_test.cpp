#include "../time.hpp"
#include <stdio.h>

int main(int argc, char const *argv[]) {
  std::string ntime = libtime::Format();
  std::time_t ts = libtime::Format2Unix(ntime.c_str());
  printf("%s %ld\n", ntime.c_str(), ts);
  long long startTime = libtime::UnixMilli();
  if (!libtime::Since(startTime, 90)) {
    printf("%lld\n", startTime);
  }
  sleep(2);
  if (libtime::Since(startTime, 5000)) {
    printf("5s超时\n");
  } else {
    printf("-------------\n");
  }
  return 0;
}
// #include <iostream>
// #include <chrono>
// #include <sstream>
// #include <iomanip>
// int main() {
//   std::tm tm = {};
//   std::string dateString = "2024-05-27 12:34:56";
//   std::istringstream ss(dateString);
//   ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

//   if (ss.fail()) {
//     std::cout << "Failed to parse the date string." << std::endl;
//     return 1;
//   }

//   // 转换为时间点
//   std::chrono::system_clock::time_point tp =
//       std::chrono::system_clock::from_time_t(std::mktime(&tm));

//   // 获取本地时区的偏移
//   std::time_t localTime = std::chrono::system_clock::to_time_t(tp);
//   std::tm *localTm = std::localtime(&localTime);
//   std::chrono::minutes offset = std::chrono::minutes(localTm->tm_gmtoff /
//   60);
//   // 输出时间戳
//   std::cout << "Timestamp: " << tp.time_since_epoch().count() << " seconds"
//             << std::endl;
//   // 应用时区偏移
//   tp -= offset;

//   // 计算时间戳
//   std::chrono::seconds timestamp =
//       std::chrono::time_point_cast<std::chrono::seconds>(tp).time_since_epoch();

//   // 输出时间戳
//   std::cout << "Timestamp: " << timestamp.count() << " seconds" <<
//   std::endl;

//   return 0;
// }