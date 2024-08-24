/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_bitset(void)
{
    BitSet bitset;
    init_bitset(&bitset, 256);
    bitset_set_all(&bitset);
    bitset_show(&bitset, stdout);

    int i = bitset_ffs_and_clear(&bitset);
    ASSERT(i == 0);

    i = bitset_ffs_and_clear(&bitset);
    ASSERT(i == 1);
    bitset_show(&bitset, stdout);

    bitset_set(&bitset, 0);
    bitset_show(&bitset, stdout);
}

int main(int argc, char *argv[])
{
    test_bitset();
    printf("done\n");
    return 0;
}

#ifdef __cplusplus
}
#endif
