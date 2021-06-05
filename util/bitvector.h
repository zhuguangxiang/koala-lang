/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_BIT_VECTOR_H_
#define _KOALA_BIT_VECTOR_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t BitWord;

#define BITWORD_SIZE (sizeof(BitWord) * 8)

#define BITWORD_SLOT(idx) ((idx) / BITWORD_SIZE)
#define BITWORD_MASK(idx) (1 << ((idx) % BITWORD_SIZE))

typedef struct _BitVector {
    /* bitmap */
    BitWord *bits;
    /* size in bits */
    int size;
    /* size in words, round_up(size / WORD_BIT_SIZE) */
    int words;
} BitVector;

/* Initialize a bit vector with size in bits */
void bitvector_init(BitVector *bvec, int nbits);

/* Finalize a bit vector */
void bitvector_fini(BitVector *bvec);

/* Set 'idx' bit postion to 1 */
void bitvector_set(BitVector *bvec, int idx);

/* Get 'idx' bit value(0 or 1) */
int bitvector_get(BitVector *bvec, int idx);

#define bitvector_test(bvec, idx) bitvector_get(bvec, idx)

/* Set 'idx' bit postion to 0 */
void bitvector_clear(BitVector *bvec, int idx);

/* Set all bits to 1 */
void bitvector_set_all(BitVector *bvec);

/* Set all bits to 0 */
void bitvector_clear_all(BitVector *bvec);

void bitvector_show(BitVector *bvec);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BIT_VECTOR_H_ */
