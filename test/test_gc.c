/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "runtime/gc.h"
#include "util/log.h"
#include "util/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* memory at least 340 bytes */

struct Bar {
    int value;
};

struct Foo {
    int value;
    struct Bar *bar;
};

int Foo_objmap[2] = {
    1,
    offsetof(struct Foo, bar),
};

void foo_fini_func(void *obj)
{
    struct Foo *foo = obj;
    printf("Foo %d fini called\n", foo->value);
}

void bar_fini_func(void *obj)
{
    struct Bar *bar = obj;
    printf("Bar %d fini called\n", bar->value);
}

void test_object_gc(void)
{
    struct Bar *old;
    struct Bar *bar = null;
    KL_GC_STACK(1);
    kl_gc_push(&bar, 0);

    bar = kl_gc_alloc(sizeof(struct Bar), null);
    debug("bar:%p", bar);
    old = bar;
    bar->value = 200;

    kl_gc();
    assert(bar->value == 200);
    assert(bar != old);
    debug("bar:%p", bar);

    bar = kl_gc_alloc(sizeof(struct Bar), null);
    bar->value = 100;
    debug("bar:%p", bar);

    {
        struct Foo *foo = null;
        KL_GC_STACK(1);
        kl_gc_push(&foo, 0);
        foo = kl_gc_alloc(sizeof(struct Foo), Foo_objmap);
        foo->value = 100;
        foo->bar = bar;
        kl_gc();
        assert(foo->value == 100);
        assert(foo->bar == bar);
        kl_gc_pop();
    }

    assert(bar->value == 100);
    debug("bar:%p", bar);

    kl_gc_pop();
}

struct TestSlice {
    int offset;
    int length;
    void *array;
};

int TestSliceObjmap[] = {
    1,
    offsetof(struct TestSlice, array),
};

struct TestArray {
    int length;
    int cap;
    void *raw;
};

int TestArrayObjmap[] = {
    1,
    offsetof(struct TestArray, raw),
};

void test_slice_array(void)
{
    struct TestArray *arr =
        kl_gc_alloc(sizeof(struct TestArray), TestArrayObjmap);
    int *raw = kl_gc_alloc_raw(sizeof(int) * 10);
    arr->length = 0;
    arr->cap = 10;
    arr->raw = raw;
    for (int i = 0; i < 10; i++) {
        raw[i] = 100 + i;
    }
    struct TestSlice *s1 =
        kl_gc_alloc(sizeof(struct TestSlice), TestSliceObjmap);
    s1->offset = 5;
    s1->length = 5;
    s1->array = arr;

    KL_GC_STACK(3);
    kl_gc_push(&s1, 0);
    kl_gc_push(&arr, 1);
    kl_gc_push(&raw, 2);

    debug("s1:%p,arr:%p,raw:%p", s1, arr, raw);

    kl_gc();

    arr = s1->array;
    raw = arr->raw;
    for (int i = 0; i < 10; i++) {
        assert(raw[i] == 100 + i);
    }

    debug("s1:%p,arr:%p,raw:%p", s1, arr, raw);
    kl_gc_pop();
}

#if 0
void test_array_gc(void)
{
    void *arr = null;
    GC_STACK(1);
    gc_push(&arr, 0);

    arr = gc_alloc_array(64, 1, 0);
    strcpy(arr, "hello, world");

    gc();
    assert(!strcmp(arr, "hello, world"));

    gc_pop();
}

void test_array_gc2(void)
{
    char **arr = null;
    GC_STACK(1);
    gc_push(&arr, 0);

    arr = (char **)gc_alloc_array(4, 8, 1);
    printf("--------first string-----------\n");
    char *s = gc_alloc_array(48, 1, 0);
    arr[0] = s;
    strcpy(s, "hello, world");
    printf("--------second string-----------\n");
    s = gc_alloc_array(48, 1, 0);
    arr[1] = s;
    strcpy(s, "hello, koala");

    printf("--------string gc-----------\n");
    gc();
    printf("--------string gc end-----------\n");

    assert(!strcmp(arr[0], "hello, world"));
    assert(!strcmp(arr[1], "hello, koala"));

    gc_pop();
}
#endif

int main(int argc, char *argv[])
{
    kl_gc_init(200);

    mm_stat();

    test_object_gc();
    test_slice_array();
    // test_array_gc();
    // test_array_gc2();
    printf("----end----\n");
    kl_gc();

    kl_gc_fini();

    mm_stat();

    return 0;
}

#ifdef __cplusplus
}
#endif
