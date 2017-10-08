
#include "hash.h"

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

uint64_t hash_uint64(uint64_t val, int bits)
{
  uint64_t hash = val;

  /*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
  uint64_t n = hash;
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

uint32_t hash_uint32(uint32_t val, int bits)
{
  /* On some cpus multiply is faster, on others gcc will do shifts */
  uint32_t hash = val * GOLDEN_RATIO_PRIME_32;

  /* High bits are more random, so use them. */
  return hash >> (32 - bits);
}

/* BKDR */
uint32_t hash_string(const char *str)
{
  uint32_t seed = 131; /* 31, 131, 1313, 13131, 131313, etc */
  uint32_t val = 0;

  while (*str) {
    val = val * seed + (*str++);
  }

  return val;
}

uint32_t hash_nstring(const char *str, int len)
{
  uint32_t seed = 131; /* 31, 131, 1313, 13131, 131313, etc */
  uint32_t val = 0;

  while ((*str) && (len)) {
    val = val * seed + (*str++);
    --len;
  }

  return val;
}
