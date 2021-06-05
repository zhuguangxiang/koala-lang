/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_HASH_H_
#define _KOALA_HASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ready-to-use hash functions for strings, using the FNV-1 algorithm.
 * (see http://www.isthe.com/chongo/tech/comp/fnv).
 * `str_hash` takes 0-terminated strings.
 * `mem_hash` operates on arbitrary-length memory.
 */
unsigned int str_hash(const char *buf);
unsigned int mem_hash(const void *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASH_H_ */
