/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* final class Array<T> {} */

#define ARRAY_DEFAULT_SIZE 16

typedef struct _ArrayObj ArrayObj;

struct _ArrayObj {
    /* virt table */
    VTable *vtbl;
    /* type param bitmap */
    uint32 tp_map;
    /* item size */
    uint32 itemsize;
    /* number elements */
    uint32 count;
    /* available index */
    uint32 next;
    /* gc array */
    void *gcarr;
};

static TypeInfo array_type = {
    .name = "Array",
    .flags = TF_CLASS | TF_FINAL,
};

static int array_objmap[] = {
    1,
    offsetof(ArrayObj, gcarr),
};

objref array_new(uint32 tp_map)
{
    int itemsize = tp_size(tp_map, 0);
    int ref = tp_is_ref(tp_map, 0);
    ArrayObj *arr = gc_alloc(sizeof(*arr), array_objmap);
    GC_STACK(1);
    gc_push(&arr, 0);
    void *gcarr = gc_alloc_array(ARRAY_DEFAULT_SIZE, itemsize, ref);
    arr->vtbl = array_type.vtbl[0];
    arr->tp_map = tp_map;
    arr->itemsize = itemsize;
    arr->count = ARRAY_DEFAULT_SIZE;
    arr->gcarr = gcarr;
    gc_pop();
    return (objref)arr;
}

void array_reserve(objref self, int32 count)
{
    ArrayObj *arr = (ArrayObj *)self;
    if (count <= arr->next) return;

    GC_STACK(1);
    gc_push(&arr, 0);

    if (count > arr->count) {
        // auto-expand
        int num = arr->count;
        while (num < count) num = num * 2;

        int ref = tp_is_ref(arr->tp_map, 0);
        void *gcarr = gc_alloc_array(num, arr->itemsize, ref);
        memcpy(gcarr, arr->gcarr, arr->count * arr->itemsize);
        arr->count = num;
        arr->gcarr = gcarr;
    }
    arr->next = count;

    gc_pop();
}

void array_set(objref self, uint32 index, anyref val)
{
    ArrayObj *arr = (ArrayObj *)self;
    if (index > arr->next) {
        printf("panic: out of bound\n");
        abort();
    }

    int ref = tp_is_ref(arr->tp_map, 0);
    GC_STACK(2);
    gc_push(&arr, 0);
    if (ref) gc_push(&val, 1);

    if (arr->next >= arr->count) {
        // auto-expand
        int num = arr->count * 2; // double
        void *gcarr;
        gcarr = gc_alloc_array(num, arr->itemsize, ref);
        memcpy(gcarr, arr->gcarr, arr->count * arr->itemsize);
        arr->count = num;
        arr->gcarr = gcarr;
    }

    char *addr = (char *)arr->gcarr + index * arr->itemsize;
    memcpy(addr, &val, arr->itemsize);
    if (index == arr->next) ++arr->next;

    gc_pop();
}

anyref array_get(objref self, uint32 index)
{
    ArrayObj *arr = (ArrayObj *)self;
    if (index >= arr->next) {
        printf("panic: out of bound\n");
        abort();
    }

    char *addr = (char *)arr->gcarr + index * arr->itemsize;
    anyref val = 0;
    memcpy(&val, addr, arr->itemsize);
    return val;
}

void array_append(objref self, anyref val)
{
    ArrayObj *arr = (ArrayObj *)self;
    int ref = tp_is_ref(arr->tp_map, 0);
    GC_STACK(2);
    gc_push(&arr, 0);
    if (ref) gc_push(&val, 1);

    if (arr->next >= arr->count) {
        // auto-expand
        int num = arr->count * 2; // double
        void *gcarr;
        gcarr = gc_alloc_array(num, arr->itemsize, ref);
        memcpy(gcarr, arr->gcarr, arr->count * arr->itemsize);
        arr->count = num;
        arr->gcarr = gcarr;
    }

    char *addr = (char *)arr->gcarr + arr->next * arr->itemsize;
    memcpy(addr, &val, arr->itemsize);
    ++arr->next;

    gc_pop();
}

int32 array_length(objref self)
{
    ArrayObj *arr = (ArrayObj *)self;
    return (int32)arr->next;
}

void array_print(objref self)
{
    ArrayObj *arr = (ArrayObj *)self;
    printf("[");
    int tp_kind = tp_index(arr->tp_map, 0);
    switch (tp_kind) {
        case TP_REF_KIND: {
            int8 *gcarr = (int8 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%d", gcarr[i]);
                else
                    printf(", %d", gcarr[i]);
            }
            break;
        }
        case TP_I8_KIND: {
            int8 *gcarr = (int8 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%d", gcarr[i]);
                else
                    printf(", %d", gcarr[i]);
            }
            break;
        }
        case TP_I16_KIND: {
            int16 *gcarr = (int16 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%d", gcarr[i]);
                else
                    printf(", %d", gcarr[i]);
            }
            break;
        }
        case TP_I32_KIND: {
            int32 *gcarr = (int32 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%d", gcarr[i]);
                else
                    printf(", %d", gcarr[i]);
            }
            break;
        }
        case TP_I64_KIND: {
            int64 *gcarr = (int64 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%ld", gcarr[i]);
                else
                    printf(", %ld", gcarr[i]);
            }
            break;
        }
        case TP_F32_KIND: {
            float *gcarr = (float *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%f", gcarr[i]);
                else
                    printf(", %f", gcarr[i]);
            }
            break;
        }
        case TP_F64_KIND: {
            double *gcarr = (double *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%f", gcarr[i]);
                else
                    printf(", %f", gcarr[i]);
            }
            break;
        }
        case TP_BOOL_KIND: {
            int8 *gcarr = (int8 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0)
                    printf("%s", gcarr[i] ? "true" : "false");
                else
                    printf(", %s", gcarr[i] ? "true" : "false");
            }
            break;
        }
        case TP_CHAR_KIND: {
            int32 *gcarr = (int32 *)arr->gcarr;
            for (int i = 0; i < arr->next; i++) {
                if (i == 0) {
                    if (gcarr[i] < 128)
                        printf("'%c'", gcarr[i]);
                    else
                        printf("'%s'", (char *)&gcarr[i]);
                } else {
                    if (gcarr[i] < 128)
                        printf(", '%c'", gcarr[i]);
                    else
                        printf(", '%s'", (char *)&gcarr[i]);
                }
            }
            break;
        }
        default:
            assert(0);
            break;
    }
    printf("]\n");
}

void init_array_type(void)
{
    MethodDef array_methods[] = {
        /* clang-format off */
        METHOD_DEF("length", nil, "i32", array_length),
        /* clang-format on */
    };
    type_add_methdefs(&array_type, array_methods);
    type_ready(&array_type);
    type_show(&array_type);
    pkg_add_type("/", &array_type);
}

#ifdef __cplusplus
}
#endif
