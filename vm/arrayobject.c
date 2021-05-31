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

typedef struct _ArrayObject {
    OBJECT_HEAD
    void *gcarr;
    uint32 num;
    uint32 itemsize;
    uint32 next : 31;
    uint32 ref : 1;
} ArrayObject, *ArrayObjectRef;

static TypeObjectRef array_type;
static int __objmap__[] = {
    1,
    offsetof(ArrayObject, gcarr),
};

ObjectRef array_new(int32 itemsize, int8 ref)
{
    ArrayObjectRef arr = gc_alloc(sizeof(ArrayObject), __objmap__);
    GC_STACK(1);
    gc_push1(&arr);
    arr->gcarr = gc_alloc_array(ARRAY_SIZE, itemsize, ref);
    arr->num = ARRAY_SIZE;
    arr->itemsize = itemsize;
    arr->ref = ref;
    gc_pop();
    return (ObjectRef)arr;
}

void array_reserve(ObjectRef self, int32 count)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    if (count <= arr->next) return;

    GC_STACK(1);
    gc_push1(&arr);

    if (count > arr->num) {
        // auto-expand
        int num = arr->num * 2;
        while (num < count) num = num * 2;

        void *gcarr = gc_alloc_array(num, arr->itemsize, arr->ref);
        memcpy(gcarr, arr->gcarr, arr->num * arr->itemsize);
        arr->num = num;
        arr->gcarr = gcarr;
    }
    arr->next = count;

    gc_pop();
}

void array___set_item__(ObjectRef self, uint32 index, uintptr_t val)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    if (index > arr->next) {
        printf("panic: out of bound\n");
        abort();
    }

    GC_STACK(2);
    if (arr->ref)
        gc_push2(&arr, &val);
    else
        gc_push1(&arr);

    if (arr->next >= arr->num) {
        // auto-expand
        int num = arr->num * 2; // double
        void *gcarr = gc_alloc_array(num, arr->itemsize, arr->ref);
        memcpy(gcarr, arr->gcarr, arr->num * arr->itemsize);
        arr->num = num;
        arr->gcarr = gcarr;
    }
    char *addr = (char *)arr->gcarr + index * arr->itemsize;
    memcpy(addr, &val, arr->itemsize);
    if (index == arr->next) ++arr->next;

    gc_pop();
}

uintptr_t array___get_item__(ObjectRef self, uint32 index)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    if (index >= arr->next) {
        printf("panic: out of bound\n");
        abort();
    }

    char *addr = (char *)arr->gcarr + index * arr->itemsize;
    uintptr_t val = 0;
    memcpy(&val, addr, arr->itemsize);
    return val;
}

void array_append(ObjectRef self, uintptr_t val)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    GC_STACK(2);
    if (arr->ref)
        gc_push2(&arr, &val);
    else
        gc_push1(&arr);

    if (arr->next >= arr->num) {
        // auto-expand
        int num = arr->num * 2; // double
        void *gcarr = gc_alloc_array(num, arr->itemsize, arr->ref);
        memcpy(gcarr, arr->gcarr, arr->num * arr->itemsize);
        arr->num = num;
        arr->gcarr = gcarr;
    }
    char *addr = (char *)arr->gcarr + arr->next * arr->itemsize;
    memcpy(addr, &val, arr->itemsize);
    ++arr->next;

    gc_pop();
}

int32 array_length(ObjectRef self)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    return (int32)arr->next;
}

void array_print(ObjectRef self)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
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
