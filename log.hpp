#pragma once

#if 1
#define LogDebug(format, ...)                                                  \
  printf("%s:%d " format " \n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LogInfo(format, ...)                                                   \
  printf("\033[35m%s:%d " format "\033[0m\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LogWarn(b, format, ...)                                                \
  if (b)                                                                       \
    printf("\033[33m%s:%d " format "\033[0m\n", __FILE__, __LINE__,            \
           ##__VA_ARGS__);                                                     \
  else                                                                         \
    LogDebug(format, ##__VA_ARGS__)

#define LogError(b, format, ...)                                               \
  if (b)                                                                       \
    printf("\033[31m%s:%d " format "\033[0m\n", __FILE__, __LINE__,            \
           ##__VA_ARGS__);                                                     \
  else                                                                         \
    LogDebug(format, ##__VA_ARGS__)
#else
#define LogDebug(format, ...)
#define LogInfo(format, ...)
#define LogWarn(format, ...)
#define LogError(format, ...)
#endif