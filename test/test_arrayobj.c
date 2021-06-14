/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core/core.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

int32 call_array_length(uintptr arr)
{
    int slot = type_get_func_slot(__GET_TYPE(arr), "length");
    FuncNode *fn = object_get_func(arr, slot);
    return ((int32(*)(uintptr))fn->ptr)(arr);
}

void test_array1(void)
{
    uintptr arr = array_new(TP_1(TP_I8_KIND));
    GC_STACK(1);
    gc_push(&arr, 0);
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
    assert(call_array_length(arr) == 12);

    char val = array_get(arr, 1);
    assert(val == 'e');
    array_set(arr, 12, 'H');
    len = array_length(arr);
    assert(len == 13);
    assert(call_array_length(arr) == 13);
    array_print(arr);

    array_reserve(arr, 15);
    array_set(arr, 14, 'K');
    array_print(arr);

    array_reserve(arr, 18);
    array_set(arr, 16, '_');
    array_print(arr);

    gc_pop();
}

void test_array2(void)
{
    uintptr arr = array_new(TP_1(TP_REF_KIND));
    GC_STACK(1);
    gc_push(&arr, 0);

    {
        GC_STACK(4);
        uintptr subarr1 = 0;
        uintptr subarr2 = 0;
        uintptr subarr3 = 0;
        uintptr subarr4 = 0;
        gc_push(&subarr1, 0);
        gc_push(&subarr2, 1);
        gc_push(&subarr3, 2);
        gc_push(&subarr4, 3);

        subarr1 = array_new(TP_1(TP_I32_KIND));
        array_append(subarr1, 100);
        array_append(subarr1, 101);
        array_append(subarr1, 102);
        array_append(subarr1, 103);
        array_append(arr, (uintptr)subarr1);
        gc();

        subarr2 = array_new(TP_1(TP_I32_KIND));
        array_append(subarr2, 200);
        array_append(subarr2, 201);
        array_append(subarr2, 202);
        array_append(subarr2, 203);
        array_append(arr, (uintptr)subarr2);
        gc();

        subarr3 = array_new(TP_1(TP_I32_KIND));
        array_append(subarr3, 300);
        array_append(subarr3, 301);
        array_append(subarr3, 302);
        array_append(subarr3, 303);
        array_append(arr, (uintptr)subarr3);
        gc();

        subarr4 = array_new(TP_1(TP_I32_KIND));
        array_append(subarr4, 400);
        array_append(subarr4, 401);
        array_append(subarr4, 402);
        array_append(subarr4, 403);
        array_append(arr, (uintptr)subarr4);
        gc();

        array_print(subarr1);
        array_print(subarr2);
        array_print(subarr3);
        array_print(subarr4);

        gc_pop();
    }

    assert(call_array_length(arr) == 4);

    gc_pop();
}

void test_array3(void)
{
    uintptr arr = array_new(TP_1(TP_CHAR_KIND));
    GC_STACK(1);
    gc_push(&arr, 0);

    unsigned char *s = (unsigned char *)"汉";
    int32 wch = (s[0] << 0) + (s[1] << 8) + (s[2] << 16);
    array_append(arr, wch);

    s = (unsigned char *)"字";
    wch = (s[0] << 0) + (s[1] << 8) + (s[2] << 16);
    array_append(arr, wch);

    wch = 'a';
    array_append(arr, wch);

    wch = 'b';
    array_append(arr, wch);

    array_print(arr);

    assert(call_array_length(arr) == 4);

    gc_pop();
}

int main(int argc, char *argv[])
{
    gc_init(768);
    init_core();

    test_array1();
    gc();
    test_array2();
    gc();

    test_array3();

    gc_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif
