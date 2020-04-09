// 简单写日志接口
#ifndef __LOGGER_H__
#define __LOGGER_H__


#define DEBUG_FILE "./log.txt"
#define DEBUG_BUFFER_MAX 4096
#define DEBUG_FILE_MAX_SIZE 1024 * 1024

#define logSize(logFile)         \
    ({                           \
        struct stat statbuf;     \
        stat(logFile, &statbuf); \
        statbuf.st_size;         \
    })

#define logWriteFormat(format, ...)                                \
    {                                                              \
        int size = logSize(DEBUG_FILE);                            \
        if (size > DEBUG_FILE_MAX_SIZE) {                          \
            remove(DEBUG_FILE);                                    \
        }                                                          \
        char buffer[DEBUG_BUFFER_MAX + 1] = { 0 };                 \
        snprintf(buffer, DEBUG_BUFFER_MAX, format, ##__VA_ARGS__); \
        FILE* fd = fopen(DEBUG_FILE, "a");                         \
        if (fd != NULL) {                                          \
            fwrite(buffer, strlen(buffer), 1, fd);                 \
            fflush(fd);                                            \
            fclose(fd);                                            \
        }                                                          \
    }

#define logWriteHex(text, hex, len)                       \
    {                                                     \
        int size = logSize(DEBUG_FILE);                   \
        if (size > DEBUG_FILE_MAX_SIZE) {                 \
            remove(DEBUG_FILE);                           \
        }                                                 \
        int  pos = 0;                                     \
        char buffer[DEBUG_BUFFER_MAX + 1] = { 0 };        \
        sprintf(buffer, "%s\n", text);                    \
        pos += (strlen(text) + 1);                        \
        for (int i = 1; i <= len; i++) {                  \
            sprintf(&buffer[pos], "0x%02x ", hex[i - 1]); \
            pos += 5;                                     \
            if ((i % 16) == 0) {                          \
                buffer[pos++] = '\n';                     \
            }                                             \
        }                                                 \
        buffer[pos++] = '\n';                             \
        FILE* fd = fopen(DEBUG_FILE, "a");                \
        if (fd != NULL) {                                 \
            fwrite(buffer, pos, 1, fd);                   \
            fflush(fd);                                   \
            fclose(fd);                                   \
        }                                                 \
    }

#define LogText logWriteFormat
#define LogHex logWriteHex

#endif