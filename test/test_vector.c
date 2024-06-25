/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_vector(void)
{
    Vector *vec = vector_create();

    int *val;

    val = malloc(sizeof(int));
    *val = 100;
    vector_push_back(vec, val);

    val = malloc(sizeof(int));
    *val = 200;
    vector_push_back(vec, val);

    val = malloc(sizeof(int));
    *val = 300;
    vector_push_back(vec, val);

    int *v;
    vector_foreach(v, vec) {
        printf("v = %d\n", *v);
    }

    vector_foreach_reverse(v, vec) {
        printf("v = %d\n", *v);
    }

    ASSERT(vector_size(vec) == 3);
    ASSERT(vector_capacity(vec) == 4);

    val = vector_pop_back(vec);
    ASSERT(*val == 300);
    free(val);

    val = vector_pop_back(vec);
    ASSERT(*val == 200);
    free(val);

    val = vector_pop_back(vec);
    ASSERT(*val == 100);
    free(val);

    ASSERT(vector_empty(vec));

    val = malloc(sizeof(int));
    *val = 1000;
    vector_insert(vec, 0, val);

    val = malloc(sizeof(int));
    *val = 2000;
    vector_insert(vec, 0, val);

    val = malloc(sizeof(int));
    *val = 3000;
    vector_insert(vec, 0, val);

    val = malloc(sizeof(int));
    *val = 4000;
    vector_insert(vec, 0, val);

    val = malloc(sizeof(int));
    *val = 5000;
    vector_push_front(vec, val);

    vector_foreach(v, vec) {
        printf("v = %d\n", *v);
    }

    val = vector_top_back(vec);
    ASSERT(*val == 1000);

    Vector *vec2 = vector_create();

    vector_concat(vec2, vec);

    vector_foreach(v, vec2) {
        printf("v = %d\n", *v);
    }

    val = vector_pop_front(vec);
    ASSERT(*val == 5000);
    free(val);

    val = vector_pop_front(vec);
    ASSERT(*val == 4000);
    free(val);

    val = vector_pop_front(vec);
    ASSERT(*val == 3000);
    free(val);

    val = vector_pop_front(vec);
    ASSERT(*val == 2000);
    free(val);

    val = vector_pop_front(vec);
    ASSERT(*val == 1000);
    free(val);

    ASSERT(vector_empty(vec));

    vector_destroy(vec);

    ASSERT(vector_size(vec2) == 5);
    ASSERT(vector_capacity(vec2) == 8);

    vector_destroy(vec2);
}

int main(int argc, char *argv[])
{
    test_vector();
    return 0;
}

#ifdef __cplusplus
}
#endif
