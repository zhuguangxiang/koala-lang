/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "vm/object.h"

struct Foo {
    int v1;
    int v2;
} foo = { 100, 200 };

int main(int argc, char *argv[])
{
    Value v = VALUE_INIT(&foo, 100);
    assert(VALUE_GET_PTR(v) == (uintptr_t)&foo);
    assert(!VALUE_IS_REF(v));
    VALUE_SET_REF(v);
    assert(VALUE_IS_REF(v));
    assert(VALUE_GET_PTR(v) == (uintptr_t)&foo);
    assert(VALUE_GET_VAL(v) == 100);
    return 0;
}
