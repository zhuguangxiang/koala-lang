/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "bitset.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROW_SIZE 8

#define BIT_ROW(idx) ((idx) / ROW_SIZE)
#define BIT_COL(idx) (1 << ((idx) % ROW_SIZE))

BitSet *bits_new(int nbits)
{
    int rows = (nbits + ROW_SIZE - 1) / ROW_SIZE;
    BitSet *bs = mm_alloc(sizeof(*bs) + rows);
    bs->size = nbits;
    bs->rows = rows;
    return bs;
}

void bits_free(BitSet *bs)
{
    mm_free(bs);
}

void bits_set(BitSet *bs, int idx)
{
    assert(idx >= 0 && idx < bs->size);
    bs->bits[BIT_ROW(idx)] |= BIT_COL(idx);
}

int bits_test(BitSet *bs, int idx)
{
    assert(idx >= 0 && idx < bs->size);
    return (bs->bits[BIT_ROW(idx)] & BIT_COL(idx)) ? 1 : 0;
}

void bits_clear(BitSet *bs, int idx)
{
    assert(idx >= 0 && idx < bs->size);
    bs->bits[BIT_ROW(idx)] &= ~BIT_COL(idx);
}

void bits_set_all(BitSet *bs)
{
    memset(bs->bits, 0xFF, bs->rows);
}

void bits_clear_all(BitSet *bs)
{
    memset(bs->bits, 0, bs->rows);
}

void bits_show(BitSet *bs, FILE *fp)
{
    for (int i = 0; i < bs->rows; i++) {
        if (i == 0)
            fprintf(fp, "%x", bs->bits[i]);
        else
            fprintf(fp, " %x", bs->bits[i]);
    }
}

#ifdef __cplusplus
}
#endif
