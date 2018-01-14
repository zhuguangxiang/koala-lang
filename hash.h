/*
 * hash.h - hashing functions
 */
#ifndef _KOALA_HASH_H_
#define _KOALA_HASH_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64 hash_uint64(uint64 val, int bits);
uint32 hash_uint32(uint32 val, int bits);

uint32 hash_nstring(const char *str, int len);
uint32 hash_string(const char *str);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASH_H_ */
