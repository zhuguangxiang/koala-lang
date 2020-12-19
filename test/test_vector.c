/*===-- test_vector.c ---------------------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* Test vector in `vector.h` and `vector.c`                                   *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vector.h"
#include <assert.h>

void test_vector(void)
{
    Vector *vec = vector_new(sizeof(int));

    int val;

    val = 100;
    vector_push_back(vec, &val);

    val = 200;
    vector_push_back(vec, &val);

    val = 300;
    vector_push_back(vec, &val);

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
