/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "gc/gc.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    pub final class Array<T> {
        pub func length() int32 {}
    }
*/

#define ARRAY_SIZE 8

typedef struct _ArrayObject ArrayObject;

struct _ArrayObject {
    GENERIC_OBJECT_HEAD
    uint32 num;
    uint32 itemsize;
    uint32 next;
    void *gcarr;
};

static TypeInfo array_type;
static int __objmap__[] = {
    1,
    offsetof(ArrayObject, gcarr),
};

Object *array_new(uint32 tp_map)
{
    int itemsize = tp_size(tp_map, 0);
    ArrayObject *arr = gc_alloc(sizeof(ArrayObject), __objmap__);
    GC_STACK(1);
    gc_push1(&arr);
    int isref = is_ref(tp_map, 0);
    void *gcarr = gc_alloc_array(ARRAY_SIZE, itemsize, isref);
    arr->gcarr = gcarr;
    arr->num = ARRAY_SIZE;
    arr->itemsize = itemsize;
    arr->tp_map = tp_map;
    gc_pop();
    return (Object *)arr;
}

void array_reserve(Object *self, int32 count)
{
    ArrayObject *arr = (ArrayObject *)self;
    if (count <= arr->next) return;
    GC_STACK(1);
    gc_push1(&arr);

    if (count > arr->num) {
        // auto-expand
        int num = arr->num * 2;
        while (num < count) num = num * 2;

        int isref = is_ref(arr->tp_map, 0);
        void *gcarr = gc_alloc_array(num, arr->itemsize, isref);
        memcpy(gcarr, arr->gcarr, arr->num * arr->itemsize);
        arr->num = num;
        arr->gcarr = gcarr;
    }
    arr->next = count;

    gc_pop();
}

void array_set(Object *self, uint32 index, uintptr val)
{
    ArrayObject *arr = (ArrayObject *)self;
    if (index > arr->next) {
        printf("panic: out of bound\n");
        abort();
    }

    int isref = is_ref(arr->tp_map, 0);
    GC_STACK(2);
    if (isref)
        gc_push2(&arr, &val);
    else
        gc_push1(&arr);

    if (arr->next >= arr->num) {
        // auto-expand
        int num = arr->num * 2; // double
        void *gcarr;
        gcarr = gc_alloc_array(num, arr->itemsize, isref);
        memcpy(gcarr, arr->gcarr, arr->num * arr->itemsize);
        arr->num = num;
        arr->gcarr = gcarr;
    }
    char *addr = (char *)arr->gcarr + index * arr->itemsize;
    memcpy(addr, &val, arr->itemsize);
    if (index == arr->next) ++arr->next;

    gc_pop();
}

uintptr array_get(Object *self, uint32 index)
{
    ArrayObject *arr = (ArrayObject *)self;
    if (index >= arr->next) {
        printf("panic: out of bound\n");
        abort();
    }

    char *addr = (char *)arr->gcarr + index * arr->itemsize;
    uintptr val = 0;
    memcpy(&val, addr, arr->itemsize);
    return val;
}

void array_append(Object *self, uintptr val)
{
    ArrayObject *arr = (ArrayObject *)self;
    int isref = is_ref(arr->tp_map, 0);
    GC_STACK(2);
    if (isref)
        gc_push2(&arr, &val);
    else
        gc_push1(&arr);

    if (arr->next >= arr->num) {
        // auto-expand
        int num = arr->num * 2; // double
        void *gcarr;
        gcarr = gc_alloc_array(num, arr->itemsize, isref);
        memcpy(gcarr, arr->gcarr, arr->num * arr->itemsize);
        arr->num = num;
        arr->gcarr = gcarr;
    }
    char *addr = (char *)arr->gcarr + arr->next * arr->itemsize;
    memcpy(addr, &val, arr->itemsize);
    ++arr->next;

    gc_pop();
}

int32 array_length(Object *self)
{
    ArrayObject *arr = (ArrayObject *)self;
    return (int32)arr->next;
}

void array_print(Object *self)
{
    ArrayObject *arr = (ArrayObject *)self;
    printf("[");
    if (arr->itemsize == 1) {
        int8 *gcarr = (int8 *)arr->gcarr;
        for (int i = 0; i < arr->next; i++) {
            if (i == 0)
                printf("%d", gcarr[i]);
            else
                printf(", %d", gcarr[i]);
        }
    }

    if (arr->itemsize == 4) {
        int32 *gcarr = (int32 *)arr->gcarr;
        for (int i = 0; i < arr->next; i++) {
            if (i == 0)
                printf("%d", gcarr[i]);
            else
                printf(", %d", gcarr[i]);
        }
    }
    printf("]\n");
}

static MethodDef array_methods[] = {
    { "length", NULL, "i32", array_length },
    { NULL },
};

#ifdef __cplusplus
}
#endif
