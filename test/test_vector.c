/*
 * This file is part of the koala project, under the MIT License.
 * Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_vector(void)
{
    VectorRef vec = vector_new(sizeof(int));

    int val;

    val = 100;
    vector_push_back(vec, &val);

    val = 200;
    vector_push_back(vec, &val);

    val = 300;
    vector_push_back(vec, &val);

    int *v;
    vector_foreach(v, vec, printf("v = %d\n", *v));
    vector_foreach_reverse(v, vec, printf("v = %d\n", *v));

    assert(vector_size(vec) == 3);
    assert(vector_capacity(vec) == 8);

    vector_pop_back(vec, &val);
    assert(val == 300);

    vector_pop_back(vec, &val);
    assert(val == 200);

    vector_pop_back(vec, &val);
    assert(val == 100);

    assert(!vector_size(vec));

    val = 1000;
    vector_insert(vec, 0, &val);

    val = 2000;
    vector_insert(vec, 0, &val);

    val = 3000;
    vector_insert(vec, 0, &val);

    vector_foreach(v, vec, printf("v = %d\n", *v));

    vector_pop_front(vec, &val);
    assert(val == 3000);

    vector_pop_front(vec, &val);
    assert(val == 2000);

    vector_pop_front(vec, &val);
    assert(val == 1000);

    assert(!vector_size(vec));

    vector_destroy(vec);
}

int main(int argc, char *argv[])
{
    test_vector();
    return 0;
}

#ifdef __cplusplus
}
#endif
