/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_BITSET_H_
#define _KOALA_BITSET_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BitSet BitSet;

struct _BitSet {
    /* size in bits */
    int size;
    /* size in rows */
    int rows;
    /* bitmap */
    uint8 bits[0];
};

/* New a bitset */
BitSet *bits_new(int nbits);

/* Free a bitset */
void bits_free(BitSet *bs);

/* Set 'idx' bit postion to 1 */
void bits_set(BitSet *bs, int idx);

/* Test 'idx' bit value(0 or 1) */
int bits_test(BitSet *bs, int idx);

/* Set 'idx' bit postion to 0 */
void bits_clear(BitSet *bs, int idx);

/* Set all bits to 1 */
void bits_set_all(BitSet *bs);

/* Set all bits to 0 */
void bits_clear_all(BitSet *bs);

void bits_show(BitSet *bs, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BITSET_H_ */
