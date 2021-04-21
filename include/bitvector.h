/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_BITVECTOR_H_
#define _KOALA_BITVECTOR_H_

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
} BitVector, *BitVectorRef;

/* Initialize a bit vector with size in bits */
void BitVecInit(BitVectorRef bvec, int nbits);

/* Finalize a bit vector */
void BitVecFini(BitVectorRef bvec);

/* Set 'idx' bit postion to 1 */
void BitVecSet(BitVectorRef bvec, int idx);

/* Set 'idx' bit postion to 0 */
void BitVecClear(BitVectorRef bvec, int idx);

/* Set all bits to 1 */
void BitVecSetAll(BitVectorRef bvec);

/* Set all bits to 0 */
void BitVecClearAll(BitVectorRef bvec);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BITVECTOR_H_ */
