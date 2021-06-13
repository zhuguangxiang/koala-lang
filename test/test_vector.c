/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_vector(void)
{
    Vector *vec = vector_create(sizeof(int));

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

    assert(vector_empty(vec));

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

    assert(vector_empty(vec));

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
