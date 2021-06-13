/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "bitvector.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

void bitvector_init(BitVector *bvec, int nbits)
{
    int nwords = (nbits + BITWORD_SIZE - 1) / BITWORD_SIZE;
    bvec->bits = mm_alloc(nwords * sizeof(BitWord));
    bvec->size = nbits;
    bvec->words = nwords;
}

void bitvector_fini(BitVector *bvec)
{
    mm_free(bvec->bits);
}

void bitvector_set(BitVector *bvec, int idx)
{
    assert(idx >= 0 && idx < bvec->size);
    bvec->bits[BITWORD_SLOT(idx)] |= BITWORD_MASK(idx);
}

int bitvector_get(BitVector *bvec, int idx)
{
    assert(idx >= 0 && idx < bvec->size);
    return (bvec->bits[BITWORD_SLOT(idx)] >> ((idx) % BITWORD_SIZE)) & 1;
}

void bitvector_clear(BitVector *bvec, int idx)
{
    assert(idx >= 0 && idx < bvec->size);
    bvec->bits[BITWORD_SLOT(idx)] &= ~BITWORD_MASK(idx);
}

void bitvector_set_all(BitVector *bvec)
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

void bitvector_clear_all(BitVector *bvec)
{
    int nbytes = bvec->words * sizeof(BitWord);
    memset(bvec->bits, 0, nbytes);
}

void bitvector_show(BitVector *bvec)
{
    for (int i = 0; i < bvec->words; i++) {
        if (i == 0)
            printf("%lx ", bvec->bits[i]);
        else
            printf("%lx", bvec->bits[i]);
    }
    printf("\n");
}

#ifdef __cplusplus
}
#endif
