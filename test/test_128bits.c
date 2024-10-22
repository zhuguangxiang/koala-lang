/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Value {
    union {
        void *ptr;
        int tag;
    };
    __int64_t val;
} Value;

Value add(Value a, Value b)
{
    Value r;
    r.val = (a.val >> 1) + (b.val >> 1);
    r.val = (r.val << 1) | 1;
    return r;
}

__int64_t add2(__int64_t a, __int64_t b)
{
    __int64_t r;
    r = (a >> 1) + (b >> 1);
    r = (r << 1) | 1;
    return r;
}

int main(int argc, char *argv[])
{
    struct Value v = { .val = (100 << 1) | 1 };
    v = add(v, v);
    printf("%ld\n", v.val >> 1);
    __int64_t i = add2((100 << 1) | 1, (100 << 1) | 1);
    printf("%ld\n", i >> 1);
    return 0;
}

#ifdef __cplusplus
}
#endif
