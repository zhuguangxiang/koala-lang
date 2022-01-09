/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "runtime/gc.h"
#include "runtime/kltypes.h"
#include "util/log.h"
#include "util/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

// gcc test/test_gc.c runtime/gc.c  runtime/klstring.c util/mm.c -I./ -g

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

void test_struct_gc(void)
{
    struct Bar *old;
    struct Bar *bar = null;
    KL_GC_STACK(1);
    kl_gc_push(&bar, 0);

    bar = kl_gc_alloc_struct(sizeof(struct Bar), null);
    debug("bar:%p", bar);
    old = bar;
    bar->value = 200;

    kl_gc();
    assert(bar->value == 200);
    assert(bar != old);
    debug("bar:%p", bar);

    bar = kl_gc_alloc_struct(sizeof(struct Bar), null);
    bar->value = 100;
    debug("bar:%p", bar);

    {
        struct Foo *foo = null;
        KL_GC_STACK(1);
        kl_gc_push(&foo, 0);
        foo = kl_gc_alloc_struct(sizeof(struct Foo), Foo_objmap);
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

void test_slice_gc(void)
{
    struct TestArray *arr = null;
    struct TestSlice *s1 = null;
    int *raw = null;

    KL_GC_STACK(3);
    kl_gc_push(&s1, 0);
    kl_gc_push(&arr, 1);
    kl_gc_push(&raw, 2);

    arr = kl_gc_alloc_struct(sizeof(struct TestArray), TestArrayObjmap);
    raw = kl_gc_alloc_raw(sizeof(int) * 20);
    arr->length = 0;
    arr->cap = 10;
    arr->raw = raw;
    for (int i = 0; i < 20; i++) {
        raw[i] = 100 + i;
    }

    s1 = kl_gc_alloc_struct(sizeof(struct TestSlice), TestSliceObjmap);
    s1->offset = 5;
    s1->length = 5;
    s1->array = arr;

    debug("s1:%p,arr:%p,raw:%p", s1, arr, raw);

    kl_gc();

    arr = s1->array;
    raw = arr->raw;
    for (int i = 0; i < 20; i++) {
        assert(raw[i] == 100 + i);
    }

    debug("s1:%p,arr:%p,raw:%p", s1, arr, raw);
    kl_gc_pop();
}

struct String {
    char *data;
    int len;
    int start;
    int end;
};

int str_objmap[] = {
    1,
    offsetof(struct String, data),
};

void test_string_gc(void)
{
    struct String *s = null;
    void *data = null;
    KL_GC_STACK(2);
    kl_gc_push(&s, 0);
    kl_gc_push(&data, 1);

    printf("--------string test-----------\n");

    s = kl_gc_alloc_struct(sizeof(struct String), str_objmap);
    data = kl_gc_alloc_raw(30);
    s->data = data;
    assert(s->data);
    s->start = 0;
    s->end = 12;

    strcpy(s->data, "hello, koala");

    printf("--------string gc-----------\n");
    kl_gc();
    printf("--------string gc end-----------\n");

    assert(!strcmp(s->data, "hello, koala"));

    kl_gc_pop();
}

int kl_string_push(KlValue *val, int ch);
KlValue kl_string_new(char *str);
KlValue kl_string_substring(KlValue *val, int start, int len);

void test_klstring(void)
{
    KlValue v = kl_string_new("hello");
    KlString *s;

    kl_string_push(&v, 'a');
    s = v.obj;
    char *data = *s->data;
    printf("%p\n", s->data);
    assert(!strcmp(data, "helloa"));

    kl_string_push(&v, 'b');
    s = v.obj;
    data = *s->data;
    printf("%p\n", s->data);
    assert(!strcmp(data, "helloab"));

    kl_string_push(&v, 'c');
    s = v.obj;
    data = *s->data;
    printf("%p\n", s->data);
    assert(!strcmp(data, "helloabc"));

    KlValue v2 = kl_string_substring(&v, 2, 2);
    s = v2.obj;
    data = *s->data + s->start;
    assert(((KlString *)v.obj)->data == s->data);
    assert(!strcmp(data, "lloabc"));

    kl_string_push(&v2, 'e');
    s = v.obj;
    data = *s->data;
    assert(!strcmp(data, "helleabc"));
}

int main(int argc, char *argv[])
{
    kl_gc_init(160);

    mm_stat();

    test_struct_gc();
    test_slice_gc();
    test_string_gc();
    test_klstring();

    printf("------------------------\n");

    kl_gc();

    kl_gc_fini();

    mm_stat();

    return 0;
}

#ifdef __cplusplus
}
#endif
