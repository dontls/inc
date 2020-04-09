#pragma once

// byte.h
// 实现大小端存储读取

#ifndef __BYTE_H__
#define __BYTE_H__

#include <stddef.h>
#include <stdint.h>

typedef char               s8;
typedef unsigned char      u8;
typedef short              s16;
typedef unsigned short     u16;
typedef int                s32;
typedef unsigned int       u32;
typedef long long          s64;
typedef unsigned long long u64;

#define PUTVAR_8(p, dat)                                                       \
    ({                                                                         \
        (p)[0] = (u8)((dat) >> 0) & 0xff;                                      \
        p += 1;                                                                \
        p;                                                                     \
    })

#define PUTVAR_N8(p, dat, n)                                                   \
    ({                                                                         \
        for (int i = 0; i < n; i++) {                                          \
            p = PUT_8(p, dat);                                                 \
        }                                                                      \
        p;                                                                     \
    })

#define PUTVAR_LE16(p, dat)                                                    \
    ({                                                                         \
        ((p)[0] = ((u16)(dat) >> 0) & 0xff,                                    \
         (p)[1] = ((u16)(dat) >> 8) & 0xff);                                   \
        p += 2;                                                                \
        p;                                                                     \
    })

#define PUTVAR_LE32(p, dat)                                                    \
    ({                                                                         \
        ((p)[0] = (u8)((u32)(dat) >> 0) & 0xff,                                \
         (p)[1] = (u8)((u32)(dat) >> 8) & 0xff,                                \
         (p)[2] = (u8)((u32)(dat) >> 16) & 0xff,                               \
         (p)[3] = (u8)((u32)(dat) >> 24) & 0xff);                              \
        p += 4;                                                                \
        p;                                                                     \
    })

#define PUTVAR_LE64(p, dat)                                                    \
    ({                                                                         \
        ((p)[0] = (u8)((u64)(dat) >> 0) & 0xff,                                \
         (p)[1] = (u8)((u64)(dat) >> 8) & 0xff,                                \
         (p)[2] = (u8)((u64)(dat) >> 16) & 0xff,                               \
         (p)[3] = (u8)((u64)(dat) >> 24) & 0xff);                              \
        ((p)[4] = (u8)((u64)(dat) >> 32) & 0xff,                               \
         (p)[5] = (u8)((u64)(dat) >> 40) & 0xff,                               \
         (p)[6] = (u8)((u64)(dat) >> 48) & 0xff,                               \
         (p)[7] = (u8)((u64)(dat) >> 56) & 0xff);                              \
        p += 8;                                                                \
        p;                                                                     \
    })

#define PUTVAR_BE16(p, dat)                                                    \
    ({                                                                         \
        ((p)[0] = ((u16)(dat) >> 8) & 0xff,                                    \
         (p)[1] = ((u16)(dat) >> 0) & 0xff);                                   \
        p += 2;                                                                \
        p;                                                                     \
    })

#define PUTVAR_BE32(p, dat)                                                    \
    ({                                                                         \
        ((p)[0] = (u8)((u32)(dat) >> 24) & 0xff,                               \
         (p)[1] = (u8)((u32)(dat) >> 16) & 0xff,                               \
         (p)[2] = (u8)((u32)(dat) >> 8) & 0xff,                                \
         (p)[3] = (u8)((u32)(dat) >> 0) & 0xff);                               \
        p += 4;                                                                \
        p;                                                                     \
    })

#define PUTVAR_BE64(p, dat)                                                    \
    ({                                                                         \
        ((p)[0] = (u8)((u64)(dat) >> 56) & 0xff,                               \
         (p)[1] = (u8)((u64)(dat) >> 48) & 0xff,                               \
         (p)[2] = (u8)((u64)(dat) >> 40) & 0xff,                               \
         (p)[3] = (u8)((u64)(dat) >> 32) & 0xff);                              \
        ((p)[4] = (u8)((u64)(dat) >> 24) & 0xff,                               \
         (p)[5] = (u8)((u64)(dat) >> 16) & 0xff,                               \
         (p)[6] = (u8)((u64)(dat) >> 8) & 0xff,                                \
         (p)[7] = (u8)((u64)(dat) >> 0) & 0xff);                               \
        p += 8;                                                                \
        p;                                                                     \
    })

#define GETVAR_8(p)                                                            \
    ({                                                                         \
        u8 r = (p)[0];                                                         \
        p += 1;                                                                \
        r;                                                                     \
    })

#define GETVAR_LE16(p)                                                         \
    ({                                                                         \
        u16 r = ((u16)(p)[0] << 0) | ((u16)(p)[1] << 8);                       \
        p += 2;                                                                \
        r;                                                                     \
    })

#define GETVAR_LE32(p)                                                         \
    ({                                                                         \
        u32 r = ((u32)(p)[0] << 0) | ((u32)(p)[1] << 8) | ((u32)(p)[2] << 16)  \
                | ((u32)(p)[3] << 24);                                         \
        p += 4;                                                                \
        r;                                                                     \
    })

#define GETVAR_LE64(p)                                                         \
    ({                                                                         \
        u64 r = ((u64)(p)[0] << 0) | ((u64)(p)[1] << 8) | ((u64)(p)[2] << 16)  \
                | ((u64)(p)[3] << 24) | ((u64)(p)[4] << 32)                    \
                | ((u64)(p)[5] << 40) | ((u64)(p)[6] << 48)                    \
                | ((u64)(p)[7] << 56);                                         \
        p += 8;                                                                \
        r;                                                                     \
    })

#define GETVAR_BE16(p)                                                         \
    ({                                                                         \
        u16 r = ((u16)(p)[0] << 8) | ((u16)(p)[1] << 0);                       \
        p += 2;                                                                \
        r;                                                                     \
    })

#define GETVAR_BE32(p)                                                         \
    ({                                                                         \
        u32 r = ((u32)(p)[0] << 24) | ((u32)(p)[1] << 16) | ((u32)(p)[2] << 8) \
                | ((u32)(p)[3] << 0);                                          \
        p += 4;                                                                \
        r                                                                      \
    })

#define GETAVR_BE64(p)                                                         \
    ({                                                                         \
        u64 r = ((u64)((u8)(p)[0]) << 56) | ((u64)((u8)(p)[1]) << 48)          \
                | ((u64)((u8)(p)[2]) << 40) | ((u64)((u8)(p)[3]) << 32)        \
                | ((u8)(p)[4] << 24) | ((u8)(p)[5] << 16) | ((u8)(p)[6] << 8)  \
                | ((u8)(p)[7] << 0);                                           \
        p += 8;                                                                \
        r;                                                                     \
    })
#endif