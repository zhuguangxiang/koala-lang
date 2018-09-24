
#include "hash.h"

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

uint64 hash_uint64(uint64 val, int bits)
{
  uint64 hash = val;

  /*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
  uint64 n = hash;
  n <<= 18;
  hash -= n;
  n <<= 33;
  hash -= n;
  n <<= 3;
  hash += n;
  n <<= 3;
  hash -= n;
  n <<= 4;
  hash += n;
  n <<= 2;
  hash += n;

  /* High bits are more random, so use them. */
  return hash >> (64 - bits);
}

uint32 hash_uint32(uint32 val, int bits)
{
  /* On some cpus multiply is faster, on others gcc will do shifts */
  uint32 hash = val * GOLDEN_RATIO_PRIME_32;

  /* High bits are more random, so use them. */
  return hash >> (32 - bits);
}

/* BKDR */
uint32 hash_string(const char *str)
{
  uint32 seed = 131; /* 31, 131, 1313, 13131, 131313, etc */
  uint32 val = 0;

  while (*str) {
    val = val * seed + (*str++);
  }

  return val;
}

uint32 hash_nstring(const char *str, int len)
{
  uint32 seed = 131; /* 31, 131, 1313, 13131, 131313, etc */
  uint32 val = 0;

  while ((*str) && (len)) {
    val = val * seed + (*str++);
    --len;
  }

  return val;
}
