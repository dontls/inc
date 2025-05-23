#pragma once

#include <stddef.h>
#include "types.h"
// 实现大小端存储读

namespace libyte {

struct Slice {
  char *data;
  size_t pos;

  Slice(char *s) : data(s), pos(0) {}
  Slice(char *s, size_t off) : data(s), pos(off) {}
  
  u8 *AppendU8(u8 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = n;
    pos += 1;
    return dest + 1;
  }

  u8 *AppendLeU16(u16 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = n & 0xff;
    dest[1] = (n >> 8) & 0xff;
    pos += 2;
    return dest + 2;
  }

  u8 *AppendBeU16(u16 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = (n >> 8) & 0xff;
    dest[1] = n & 0xff;
    pos += 2;
    return dest + 2;
  }

  u8 *AppendLeU32(u32 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = n & 0xff;
    dest[1] = (n >> 8) & 0xff;
    dest[2] = (n >> 16) & 0xff;
    dest[3] = (n >> 24) & 0xff;
    pos += 4;
    return dest + 4;
  }

  u8 *AppendBeU32(u32 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = (n >> 24) & 0xff;
    dest[1] = (n >> 16) & 0xff;
    dest[2] = (n >> 8) & 0xff;
    dest[3] = n & 0xff;
    pos += 4;
    return dest + 4;
  }

  u8 *AppendLeU64(u64 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = n & 0xff;
    dest[1] = (n >> 8) & 0xff;
    dest[2] = (n >> 16) & 0xff;
    dest[3] = (n >> 24) & 0xff;
    dest[4] = (n >> 32) & 0xff;
    dest[5] = (n >> 40) & 0xff;
    dest[6] = (n >> 48) & 0xff;
    dest[7] = (n >> 56) & 0xff;
    pos += 8;
    return dest + 8;
  }

  u8 *AppendBeU64(u64 n) {
    u8 *dest = (u8 *)(data + pos);
    dest[0] = (n >> 56) & 0xff;
    dest[1] = (n >> 48) & 0xff;
    dest[2] = (n >> 40) & 0xff;
    dest[3] = (n >> 32) & 0xff;
    dest[4] = (n >> 24) & 0xff;
    dest[5] = (n >> 16) & 0xff;
    dest[6] = (n >> 8) & 0xff;
    dest[7] = n & 0xff;
    pos += 8;
    return dest + 8;
  }

  u8 U8() {
    u8 r = data[pos++];
    return r;
  }

  u16 LeU16() {
    u16 r = data[pos++];
    r |= u16(data[pos++]) << 8;
    return r;
  }

  u32 LeU32() {
    u32 r = data[pos++];
    r |= u32(data[pos++]) << 8;
    r |= u32(data[pos++]) << 16;
    r |= u32(data[pos++]) << 24;
    return r;
  }
  u64 LeU64() {
    u64 r = data[pos++];
    r |= u64(data[pos++]) << 8;
    r |= u64(data[pos++]) << 16;
    r |= u64(data[pos++]) << 24;
    r |= u64(data[pos++]) << 32;
    r |= u64(data[pos++]) << 40;
    r |= u64(data[pos++]) << 48;
    r |= u64(data[pos++]) << 56;
    return r;
  }

  u16 BeU16() {
    u16 r = u16(data[pos++]) << 8;
    r |= u16(data[pos++]);
    return r;
  }

  u32 BeU32() {
    u32 r = u32(data[pos++]) << 24;
    r |= u32(data[pos++]) << 16;
    r |= u32(data[pos++]) << 8;
    r |= u32(data[pos++]);
    return r;
  }

  u64 BeU64() {
    u64 r = u64(data[pos++]) << 56;
    r |= u64(data[pos++]) << 48;
    r |= u64(data[pos++]) << 40;
    r |= u64(data[pos++]) << 32;
    r |= u64(data[pos++]) << 24;
    r |= u64(data[pos++]) << 16;
    r |= u64(data[pos++]) << 8;
    r |= u64(data[pos++]);
    return r;
  }
};

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
