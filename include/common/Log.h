#ifndef __LOG_H__
#define __LOG_H__

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DBG_FG_BLACK "\033[30m"
#define DBG_FG_RED "\033[31m"
#define DBG_FG_GREEN "\033[32m"
#define DBG_FG_YELLOW "\033[33m"
#define DBG_FG_BLUE "\033[34m"
#define DBG_FG_VIOLET "\033[35m"
#define DBG_FG_VIRDIAN "\033[36m"
#define DBG_FG_WHITE "\033[37m"

#define DBG_BG_BLACK "\033[40m"
#define DBG_BG_RED "\033[41m"
#define DBG_BG_GREEN "\033[42m"
#define DBG_BG_YELLOW "\033[43m"
#define DBG_BG_BLUE "\033[44m"
#define DBG_BG_VIOLET "\033[45m"
#define DBG_BG_VIRDIAN "\033[46m"
#define DBG_BG_WHITE "\033[47m"

#define DBG_END "\033[0m"

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
        int  pos                          = 0;            \
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
        FILE* fd      = fopen(DEBUG_FILE, "a");           \
        if (fd != NULL) {                                 \
            fwrite(buffer, pos, 1, fd);                   \
            fflush(fd);                                   \
            fclose(fd);                                   \
        }                                                 \
    }

#define LogText logWriteFormat
#define LogHex logWriteHex

#define printf(format, ...)                                         \
    {                                                               \
        printf("%s:%d " format, __FILE__, __LINE__, ##__VA_ARGS__); \
    }
#define printf_error(format, ...)                         \
    {                                                     \
        printf(DBG_FG_RED format DBG_END, ##__VA_ARGS__); \
    }
#define printf_warning(format, ...)                          \
    {                                                        \
        printf(DBG_FG_YELLOW format DBG_END, ##__VA_ARGS__); \
    }

#endif