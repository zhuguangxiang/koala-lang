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

#ifdef __cplusplus
extern "C" {
#endif

static void test_bitvector(void)
{
    BitVector bvec;
    BitVecInit(&bvec, 3);
    assert(bvec.size == 3);
    assert(bvec.words == 1);

    BitVecSet(&bvec, 0);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 0);
    assert(BitVecGet(&bvec, 2) == 0);

    BitVecSet(&bvec, 1);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 1);
    assert(BitVecGet(&bvec, 2) == 0);

    BitVecSet(&bvec, 2);
    assert(bvec.bits[0] == 7);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 1);
    assert(BitVecGet(&bvec, 2) == 1);

    BitVecClear(&bvec, 1);
    assert(bvec.bits[0] == 5);

    BitVecClearAll(&bvec);
    assert(bvec.bits[0] == 0);

    BitVecSetAll(&bvec);
    assert(bvec.bits[0] == 7);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 1);
    assert(BitVecGet(&bvec, 2) == 1);

    BitVecFini(&bvec);
}

static void test_bitvector2(void)
{
    BitVector bvec;
    BitVecInit(&bvec, 65);
    assert(bvec.size == 65);
    assert(bvec.words == 2);

    BitVecSet(&bvec, 0);
    BitVecSet(&bvec, 1);
    BitVecSet(&bvec, 2);
    assert(bvec.bits[0] == 7);
    assert(bvec.bits[1] == 0);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 1);
    assert(BitVecGet(&bvec, 2) == 1);

    BitVecClear(&bvec, 1);
    assert(bvec.bits[0] == 5);
    assert(bvec.bits[1] == 0);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 0);
    assert(BitVecGet(&bvec, 2) == 1);

    BitVecSet(&bvec, 64);
    assert(bvec.bits[0] == 5);
    assert(bvec.bits[1] == 1);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 0);
    assert(BitVecGet(&bvec, 2) == 1);
    assert(BitVecGet(&bvec, 64) == 1);

    BitVecClearAll(&bvec);
    assert(bvec.bits[0] == 0);
    assert(bvec.bits[1] == 0);

    BitVecSetAll(&bvec);
    assert(bvec.bits[0] == (BitWord)-1);
    assert(bvec.bits[1] == 1);
    assert(BitVecGet(&bvec, 0) == 1);
    assert(BitVecGet(&bvec, 1) == 1);
    assert(BitVecGet(&bvec, 2) == 1);
    assert(BitVecGet(&bvec, 64) == 1);
    BitVecFini(&bvec);
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
