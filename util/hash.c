/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#define FNV32_BASE  ((unsigned int)0x811c9dc5)
#define FNV32_PRIME ((unsigned int)0x01000193)

unsigned int str_hash(const char *str)
{
    unsigned int c, hash = FNV32_BASE;
    while ((c = (unsigned char)*str++)) hash = (hash * FNV32_PRIME) ^ c;
    return hash;
}

unsigned int mem_hash(const void *buf, int len)
{
    unsigned int hash = FNV32_BASE;
    unsigned char *ucbuf = (unsigned char *)buf;
    while (len--) {
        unsigned int c = *ucbuf++;
        hash = (hash * FNV32_PRIME) ^ c;
    }
    return hash;
}

#ifdef __cplusplus
}
#endif
