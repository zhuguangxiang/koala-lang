/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "util/bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

static void test_bitset(void)
{
    BitSet *bs = bits_new(3);
    assert(bs->size == 3);
    assert(bs->rows == 1);

    bits_set(bs, 0);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 0);
    assert(bits_test(bs, 2) == 0);

    bits_set(bs, 1);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 1);
    assert(bits_test(bs, 2) == 0);

    bits_set(bs, 2);
    assert(bs->bits[0] == 7);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 1);
    assert(bits_test(bs, 2) == 1);

    bits_clear(bs, 1);
    assert(bs->bits[0] == 5);

    bits_clear_all(bs);
    assert(bs->bits[0] == 0);

    bits_set_all(bs);
    assert(bs->bits[0] == (uint8)-1);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 1);
    assert(bits_test(bs, 2) == 1);

    bits_free(bs);
}

static void test_bitset2(void)
{
    BitSet *bs = bits_new(65);
    assert(bs->size == 65);
    assert(bs->rows == 9);

    bits_set(bs, 0);
    bits_set(bs, 1);
    bits_set(bs, 2);
    assert(bs->bits[0] == 7);
    assert(bs->bits[8] == 0);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 1);
    assert(bits_test(bs, 2) == 1);

    bits_clear(bs, 1);
    assert(bs->bits[0] == 5);
    assert(bs->bits[8] == 0);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 0);
    assert(bits_test(bs, 2) == 1);

    bits_set(bs, 64);
    assert(bs->bits[0] == 5);
    assert(bs->bits[8] == 1);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 0);
    assert(bits_test(bs, 2) == 1);
    assert(bits_test(bs, 64) == 1);

    bits_clear_all(bs);
    assert(bs->bits[0] == 0);
    assert(bs->bits[8] == 0);

    bits_set_all(bs);
    assert(bs->bits[0] == (uint8)-1);
    assert(bs->bits[8] == (uint8)-1);
    assert(bits_test(bs, 0) == 1);
    assert(bits_test(bs, 1) == 1);
    assert(bits_test(bs, 2) == 1);
    assert(bits_test(bs, 64) == 1);

    bits_free(bs);
}

int main(int argc, char *argv[])
{
    test_bitset();
    test_bitset2();
    return 0;
}

#ifdef __cplusplus
}
#endif
