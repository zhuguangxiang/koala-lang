/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "util/bitvector.h"

#ifdef __cplusplus
extern "C" {
#endif

static void test_bitvector(void)
{
    BitVector bvec;
    bitvector_init(&bvec, 3);
    assert(bvec.size == 3);
    assert(bvec.words == 1);

    bitvector_set(&bvec, 0);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 0);
    assert(bitvector_get(&bvec, 2) == 0);

    bitvector_set(&bvec, 1);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 1);
    assert(bitvector_get(&bvec, 2) == 0);

    bitvector_set(&bvec, 2);
    assert(bvec.bits[0] == 7);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 1);
    assert(bitvector_get(&bvec, 2) == 1);

    bitvector_clear(&bvec, 1);
    assert(bvec.bits[0] == 5);

    bitvector_clear_all(&bvec);
    assert(bvec.bits[0] == 0);

    bitvector_set_all(&bvec);
    assert(bvec.bits[0] == 7);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 1);
    assert(bitvector_get(&bvec, 2) == 1);

    bitvector_fini(&bvec);
}

static void test_bitvector2(void)
{
    BitVector bvec;
    bitvector_init(&bvec, 65);
    assert(bvec.size == 65);
    assert(bvec.words == 2);

    bitvector_set(&bvec, 0);
    bitvector_set(&bvec, 1);
    bitvector_set(&bvec, 2);
    assert(bvec.bits[0] == 7);
    assert(bvec.bits[1] == 0);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 1);
    assert(bitvector_get(&bvec, 2) == 1);

    bitvector_clear(&bvec, 1);
    assert(bvec.bits[0] == 5);
    assert(bvec.bits[1] == 0);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 0);
    assert(bitvector_get(&bvec, 2) == 1);

    bitvector_set(&bvec, 64);
    assert(bvec.bits[0] == 5);
    assert(bvec.bits[1] == 1);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 0);
    assert(bitvector_get(&bvec, 2) == 1);
    assert(bitvector_get(&bvec, 64) == 1);

    bitvector_clear_all(&bvec);
    assert(bvec.bits[0] == 0);
    assert(bvec.bits[1] == 0);

    bitvector_set_all(&bvec);
    assert(bvec.bits[0] == (BitWord)-1);
    assert(bvec.bits[1] == 1);
    assert(bitvector_get(&bvec, 0) == 1);
    assert(bitvector_get(&bvec, 1) == 1);
    assert(bitvector_get(&bvec, 2) == 1);
    assert(bitvector_get(&bvec, 64) == 1);
    bitvector_fini(&bvec);
}

int main(int argc, char *argv[])
{
    test_bitvector();
    test_bitvector2();
    return 0;
}

#ifdef __cplusplus
}
#endif
