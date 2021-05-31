/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "gc/gc.h"
#include "vm/arrayobject.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_array1(void)
{
    ObjectRef arr = array_new(1, 0);
    GC_STACK(1);
    gc_push1(&arr);
    array_append(arr, 'h');
    array_append(arr, 'e');
    array_append(arr, 'l');
    array_append(arr, 'l');
    array_append(arr, 'o');
    array_append(arr, ' ');
    array_append(arr, 'w');
    array_append(arr, 'o');
    array_append(arr, 'r');
    array_append(arr, 'l');
    array_append(arr, 'd');
    array_append(arr, '!');
    int32 len = array_length(arr);
    assert(len == 12);
    char val = array___get_item__(arr, 1);
    assert(val == 'e');
    array___set_item__(arr, 12, 'H');
    len = array_length(arr);
    assert(len == 13);
    array_print(arr);

    array_reserve(arr, 15);
    array___set_item__(arr, 14, 'K');
    array_print(arr);

    array_reserve(arr, 18);
    array___set_item__(arr, 16, '_');
    array_print(arr);

    gc_pop();
}

void test_array2(void)
{
    ObjectRef arr = array_new(8, 1);
    GC_STACK(1);
    gc_push1(&arr);

    {
        GC_STACK(4);
        ObjectRef subarr1 = NULL;
        ObjectRef subarr2 = NULL;
        ObjectRef subarr3 = NULL;
        ObjectRef subarr4 = NULL;
        gc_push4(&subarr1, &subarr2, &subarr3, &subarr4);

        subarr1 = array_new(4, 0);
        array_append(subarr1, 100);
        array_append(subarr1, 101);
        array_append(subarr1, 102);
        array_append(subarr1, 103);
        array_append(arr, (uintptr_t)subarr1);
        gc();

        subarr2 = array_new(4, 0);
        array_append(subarr2, 200);
        array_append(subarr2, 201);
        array_append(subarr2, 202);
        array_append(subarr2, 203);
        array_append(arr, (uintptr_t)subarr2);
        gc();

        subarr3 = array_new(4, 0);
        array_append(subarr3, 300);
        array_append(subarr3, 301);
        array_append(subarr3, 302);
        array_append(subarr3, 303);
        array_append(arr, (uintptr_t)subarr3);
        gc();

        subarr4 = array_new(4, 0);
        array_append(subarr4, 400);
        array_append(subarr4, 401);
        array_append(subarr4, 402);
        array_append(subarr4, 403);
        array_append(arr, (uintptr_t)subarr4);
        gc();

        array_print(subarr1);
        array_print(subarr2);
        array_print(subarr3);
        array_print(subarr4);

        gc_pop();
    }

    gc_pop();
}

int main(int argc, char *argv[])
{
    gc_init(512);
    test_array1();
    gc();
    test_array2();
    gc();
    gc_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif
