/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BITSET_H_
#define _KOALA_BITSET_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BitSet {
    /* size in bits */
    int size;
    /* size in rows */
    int rows;
    /* bitmap */
    uint8_t *bits;
} BitSet;

#define ROW_SIZE   (sizeof(uint8_t) * 8)
#define BIT_ROW(i) ((i) / ROW_SIZE)
#define BIT_COL(i) (1 << ((i) % ROW_SIZE))

/* initialize a bitset */
static inline void init_bitset(BitSet *bs, int nbits)
{
    int rows = (nbits + ROW_SIZE - 1) / ROW_SIZE;
    uint8_t *bits = mm_alloc(rows);
    bs->bits = bits;
    bs->rows = rows;
    bs->size = nbits;
}

/* finalize a bitset */
static inline void fini_bitset(BitSet *bs) { mm_free(bs->bits); }

/* new a bitset */
static inline BitSet *bitset_new(int nbits)
{
    BitSet *bitset = mm_alloc_obj_fast(bitset);
    init_bitset(bitset, nbits);
    return bitset;
}

/* free a bitset */
static inline void bitset_free(BitSet *bs)
{
    fini_bitset(bs);
    mm_free(bs);
}

/* set i-th bit */
static inline void bitset_set(BitSet *bs, int i)
{
    ASSERT(i >= 0 && i < bs->size);
    bs->bits[BIT_ROW(i)] |= BIT_COL(i);
}

/* test i-th bit value */
static inline int bitset_test(BitSet *bs, int i)
{
    ASSERT(i >= 0 && i < bs->size);
    return (bs->bits[BIT_ROW(i)] & BIT_COL(i)) ? 1 : 0;
}

/* set i-th bit */
static inline void bitset_clear(BitSet *bs, int i)
{
    ASSERT(i >= 0 && i < bs->size);
    bs->bits[BIT_ROW(i)] &= ~BIT_COL(i);
}

/* set all bits */
static inline void bitset_set_all(BitSet *bs) { memset(bs->bits, -1, bs->rows); }

/* clear all bits */
static inline void bitset_clear_all(BitSet *bs) { memset(bs->bits, 0, bs->rows); }

/* display the bitset */
void bitset_show(BitSet *bs, FILE *fp);

/* return first bit set and clear it */
int bitset_ffs_and_clear(BitSet *bs);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BITSET_H_ */
