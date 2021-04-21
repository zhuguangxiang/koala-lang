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

#include "bitvector.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

void BitVecInit(BitVectorRef bvec, int nbits)
{
    int nwords = (nbits + BITWORD_SIZE - 1) / BITWORD_SIZE;
    bvec->bits = MemAlloc(nwords * sizeof(BitWord));
    bvec->size = nbits;
    bvec->words = nwords;
}

void BitVecFini(BitVectorRef bvec)
{
    MemFree(bvec->bits);
}

void BitVecSet(BitVectorRef bvec, int idx)
{
    assert(idx >= 0 && idx < bvec->size);
    bvec->bits[BITWORD_SLOT(idx)] |= BITWORD_MASK(idx);
}

void BitVecClear(BitVectorRef bvec, int idx)
{
    assert(idx >= 0 && idx < bvec->size);
    bvec->bits[BITWORD_SLOT(idx)] &= ~BITWORD_MASK(idx);
}

void BitVecSetAll(BitVectorRef bvec)
{
    int nbytes = bvec->words * sizeof(BitWord);
    memset(bvec->bits, 0xFF, nbytes);

    int row = bvec->size / BITWORD_SIZE;
    int col = bvec->size % BITWORD_SIZE;
    bvec->bits[row] &= ((1 << col) - 1);
    for (int i = row + 1; i < bvec->words; i++) {
        bvec->bits[i] = 0;
    }
}

void BitVecClearAll(BitVectorRef bvec)
{
    int nbytes = bvec->words * sizeof(BitWord);
    memset(bvec->bits, 0, nbytes);
}

#ifdef __cplusplus
}
#endif
