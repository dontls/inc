#pragma once
// byte.h
// 实现大小端存储读取
typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;

namespace libyte {

inline u8 *AppendU8(u8 *dest, u8 n) {
  dest[0] = n;
  return dest + 1;
}

inline u8 *AppendLeU16(u8 *dest, u16 n) {
  dest[0] = n & 0xff;
  dest[1] = (n >> 8) & 0xff;
  return dest + 2;
}

inline u8 *AppendLeU32(u8 *dest, u32 n) {
  dest[0] = n & 0xff;
  dest[1] = (n >> 8) & 0xff;
  dest[2] = (n >> 16) & 0xff;
  dest[3] = (n >> 24) & 0xff;
  return dest + 4;
}

inline u8 *AppendLeU64(u8 *dest, u64 n) {
  dest[0] = n & 0xff;
  dest[1] = (n >> 8) & 0xff;
  dest[2] = (n >> 16) & 0xff;
  dest[3] = (n >> 24) & 0xff;
  dest[4] = (n >> 32) & 0xff;
  dest[5] = (n >> 40) & 0xff;
  dest[6] = (n >> 48) & 0xff;
  dest[7] = (n >> 56) & 0xff;
  return dest + 4;
}

inline u8 *AppendBeU16(u8 *dest, u16 n) {
  dest[0] = (n >> 8) & 0xff;
  dest[1] = n & 0xff;
  return dest + 2;
}

inline u8 *AppendBeU32(u8 *dest, u32 n) {
  dest[0] = (n >> 24) & 0xff;
  dest[1] = (n >> 16) & 0xff;
  dest[2] = (n >> 8) & 0xff;
  dest[3] = n & 0xff;
  return dest + 4;
}

inline u8 *AppendBeU64(u8 *dest, u64 n) {
  dest[0] = (n >> 56) & 0xff;
  dest[1] = (n >> 48) & 0xff;
  dest[2] = (n >> 40) & 0xff;
  dest[3] = (n >> 32) & 0xff;
  dest[4] = (n >> 24) & 0xff;
  dest[5] = (n >> 16) & 0xff;
  dest[6] = (n >> 8) & 0xff;
  dest[7] = n & 0xff;
  return dest + 8;
}

inline u8 U8(u8 *b, s32 &i) {
  u8 r = b[i++];
  return r;
}
inline u16 LeU16(u8 *b, s32 &i) {
  u16 r = b[i++];
  r |= u16(b[i++]) << 8;
  return r;
}
inline u32 LeU32(u8 *b, s32 &i) {
  u32 r = b[i++];
  r |= u32(b[i++]) << 8;
  r |= u32(b[i++]) << 16;
  r |= u32(b[i++]) << 24;
  return r;
}
inline u64 LeU64(u8 *b, s32 &i) {
  u64 r = b[i++];
  r |= u64(b[i++]) << 8;
  r |= u64(b[i++]) << 16;
  r |= u64(b[i++]) << 24;
  r |= u64(b[i++]) << 32;
  r |= u64(b[i++]) << 40;
  r |= u64(b[i++]) << 48;
  r |= u64(b[i++]) << 56;
  return r;
}

inline u16 BeU16(u8 *b, s32 &i) {
  u16 r = u16(b[i++]) << 8;
  r |= u16(b[i++]);
  return r;
}

inline u32 BeU32(u8 *b, s32 &i) {
  u32 r = u32(b[i++]) << 24;
  r |= u32(b[i++]) << 16;
  r |= u32(b[i++]) << 8;
  r |= u32(b[i++]);
  return r;
}

inline u64 BeU64(u8 *b, s32 &i) {
  u64 r = u64(b[i++]) << 56;
  r |= u64(b[i++]) << 48;
  r |= u64(b[i++]) << 40;
  r |= u64(b[i++]) << 32;
  r |= u64(b[i++]) << 24;
  r |= u64(b[i++]) << 16;
  r |= u64(b[i++]) << 8;
  r |= u64(b[i++]);
  return r;
}

} // namespace libyte

#define GETVAR_8(p)                                                            \
  ({                                                                           \
    u8 r = (p)[0];                                                             \
    p += 1;                                                                    \
    r;                                                                         \
  })

#define GETVAR_LE16(p)                                                         \
  ({                                                                           \
    u16 r = ((u16)(p)[0] << 0) | ((u16)(p)[1] << 8);                           \
    p += 2;                                                                    \
    r;                                                                         \
  })

#define GETVAR_LE32(p)                                                         \
  ({                                                                           \
    u32 r = ((u32)(p)[0] << 0) | ((u32)(p)[1] << 8) | ((u32)(p)[2] << 16) |    \
            ((u32)(p)[3] << 24);                                               \
    p += 4;                                                                    \
    r;                                                                         \
  })

#define GETVAR_LE64(p)                                                         \
  ({                                                                           \
    u64 r = ((u64)(p)[0] << 0) | ((u64)(p)[1] << 8) | ((u64)(p)[2] << 16) |    \
            ((u64)(p)[3] << 24) | ((u64)(p)[4] << 32) | ((u64)(p)[5] << 40) |  \
            ((u64)(p)[6] << 48) | ((u64)(p)[7] << 56);                         \
    p += 8;                                                                    \
    r;                                                                         \
  })

#define GETVAR_BE16(p)                                                         \
  ({                                                                           \
    u16 r = ((u16)(p)[0] << 8) | ((u16)(p)[1] << 0);                           \
    p += 2;                                                                    \
    r;                                                                         \
  })

#define GETVAR_BE32(p)                                                         \
  ({                                                                           \
    u32 r = ((u32)(p)[0] << 24) | ((u32)(p)[1] << 16) | ((u32)(p)[2] << 8) |   \
            ((u32)(p)[3] << 0);                                                \
    p += 4;                                                                    \
    r                                                                          \
  })

#define GETAVR_BE64(p)                                                         \
  ({                                                                           \
    u64 r = ((u64)((u8)(p)[0]) << 56) | ((u64)((u8)(p)[1]) << 48) |            \
            ((u64)((u8)(p)[2]) << 40) | ((u64)((u8)(p)[3]) << 32) |            \
            ((u8)(p)[4] << 24) | ((u8)(p)[5] << 16) | ((u8)(p)[6] << 8) |      \
            ((u8)(p)[7] << 0);                                                 \
    p += 8;                                                                    \
    r;                                                                         \
  })