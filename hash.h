/*
 * hash.h - hashing functions
 */
#ifndef _KOALA_HASH_H_
#define _KOALA_HASH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

uint64_t hash_uint64(uint64_t val, int bits);
uint32_t hash_uint32(uint32_t val, int bits);

uint32_t hash_nstring(const char *str, int len);
uint32_t hash_string(const char *str);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASH_H_ */
