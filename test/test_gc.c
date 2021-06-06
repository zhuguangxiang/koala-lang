/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "gc/gc.h"
#include "util/mm.h"

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
    struct Bar *bar = nil;
    GC_STACK(1);
    gc_push(&bar, 0);

    bar = gc_alloc(sizeof(struct Bar), nil);
    old = bar;
    bar->value = 200;

    gc();
    assert(bar->value == 200);
    assert(bar != old);

    bar = gc_alloc(sizeof(struct Bar), nil);
    bar->value = 100;

    {
        struct Foo *foo = nil;
        GC_STACK(1);
        gc_push(&foo, 0);
        foo = gc_alloc(sizeof(struct Foo), Foo_objmap);
        foo->value = 100;
        foo->bar = bar;
        gc();
        assert(foo->value == 100);
        assert(foo->bar == bar);
        gc_pop();
    }

    assert(bar->value == 100);

    gc_pop();
}

void test_array_gc(void)
{
    void *arr = nil;
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
    char **arr = nil;
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

int main(int argc, char *argv[])
{
    gc_init(200);

    mm_stat();

    test_object_gc();
    test_array_gc();
    test_array_gc2();
    printf("----end----\n");
    gc();

    gc_fini();

    mm_stat();

    return 0;
}
