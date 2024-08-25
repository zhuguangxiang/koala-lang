/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

void bitset_show(BitSet *bs, FILE *fp)
{
    for (int i = 0; i < bs->rows; i++) {
        if (i % 8 == 0)
            fprintf(fp, "\n%X", bs->bits[i]);
        else
            fprintf(fp, " %X", bs->bits[i]);
    }
    fprintf(fp, "\n\n");
}

int bitset_ffs_and_clear(BitSet *bs)
{
    for (int i = 0; i < bs->rows; i++) {
        int row = bs->bits[i];
        int j = FFS(row);
        if (j > 0) {
            int r = i * 8 + j - 1;
            bitset_clear(bs, r);
            return r;
        }
    }
    return -1;
}

#ifdef __cplusplus
}
#endif
