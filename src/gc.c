/*===-- gc.c - Koala Garbage Collection ---------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements the garbage collection algorithm.                     *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "common.h"
#include "mm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The gc algorithm:
 * 1. semi-space copying
 * 2. see: `The Garbage Collection Handbook`
 */

/* gc object kind */
enum gckind {
    /* raw memory, string */
    GC_RAW_KIND = 1,
    /* array memory, slice shared */
    GC_ARRAY_KIND,
    /* extend by C, no type */
    GC_STRUCT_KIND,
    /* normal koala object */
    GC_OBJECT_KIND,
    /* object is handled */
    GC_FORWARD_KIND,
};

/* all kind gc header */
typedef struct GcHeader {
    uint32_t objsize : 29;
    uint32_t kind : 3;
    union {
        /* object is handled */
        void *forward;
        /* normal object map */
        objmap_t *objmap;
    };
} GcHeader;

static const int space_size = 340;
static char *from_space;
static char *to_space;
static char *free_ptr;

static GcHeader *__new__(int size)
{
    size = ALIGN_PTR(size);
    int objsize = sizeof(GcHeader) + size;
    char *new_ptr;
    int tried = 0;

Lrealloc:
    new_ptr = free_ptr + objsize;
    if (new_ptr > (from_space + space_size)) {
        int waste = space_size - (free_ptr - from_space);
        if (tried) {
            printf("gc-debug: alloc size:%d failed\n", objsize);
            printf("gc-debug: %d bytes wasted(total:%d)\n", waste, space_size);
            printf("gc-error: too small managed memory\n");
            abort();
        }
        printf("gc-debug: alloc size:%d failed\n", objsize);
        printf("gc-debug: %d bytes wasted(total:%d)\n", waste, space_size);
        tried = 1;
        gc();
        goto Lrealloc;
    }

    printf("gc-debug: alloc size:%d\n", objsize);

    GcHeader *hdr = (GcHeader *)free_ptr;
    // 8 bytes alignment
    assert(!((uintptr_t)hdr & 7));

    free_ptr = new_ptr;
    hdr->objsize = size;
    hdr->kind = GC_RAW_KIND;
    return hdr;
}

void *gc_alloc(int size)
{
    assert(size > 0);
    GcHeader *hdr = __new__(size);
    return (void *)(hdr + 1);
}

void *__gc_alloc_struct(objmap_t *objmap, int size)
{
    assert(size > 0);
    GcHeader *hdr = __new__(size);
    hdr->kind = GC_STRUCT_KIND;
    hdr->objmap = objmap;
    return (void *)(hdr + 1);
}

GcArray *gc_alloc_array(int len, int own_buf, int isobj)
{
    GcArray *arr = NULL;
    gc_push(&arr);

    printf("gc-debug: alloc array [%d]\n", len);

    int size = sizeof(GcArray);
    if (own_buf) size += len * sizeof(uintptr_t);

    GcHeader *hdr = __new__(size);
    hdr->kind = GC_ARRAY_KIND;
    arr = (GcArray *)(hdr + 1);
    arr->len = len;
    arr->isobj = isobj;

    void *data;
    if (own_buf)
        data = arr->data;
    else {
        hdr = __new__(len * sizeof(uintptr_t));
        data = (void *)(hdr + 1);
    }

    arr->ptr = data;

    gc_pop();

    return arr;
}

GcArray *gc_expand_array(GcArray *arr, int newlen)
{

    if (arr->ptr != arr->data) {
        GcHeader *hdr = __new__(newlen * sizeof(uintptr_t));
        memcpy(hdr + 1, arr->ptr, arr->len * sizeof(uintptr_t));
        arr->ptr = hdr + 1;
        arr->len = newlen;
        return arr;
    }
    else {
        GcArray *newarr = gc_alloc_array(newlen, 1, arr->isobj);
        memcpy(newarr->ptr, arr->ptr, arr->len * sizeof(uintptr_t));
        return newarr;
    }
}

GcObject *gc_alloc_obj(int size, void *type, void *vtbl, objmap_t *map)
{
    size += sizeof(GcObject);
    GcHeader *hdr = __new__(size);
    hdr->kind = GC_OBJECT_KIND;
    GcObject *obj = (GcObject *)(hdr + 1);
    obj->type = type;
    obj->type = vtbl;
    return obj;
}

void gc_init(void)
{
    from_space = mm_alloc(space_size);
    to_space = mm_alloc(space_size);
    free_ptr = from_space;
}

void gc_fini(void)
{
    mm_free(from_space);
    mm_free(to_space);
}

GcTrace *gcroots;

static void *copy(void *ptr)
{
    if (!ptr) return NULL;

    GcHeader *hdr = (GcHeader *)ptr - 1;

    switch (hdr->kind) {
        case GC_FORWARD_KIND:
            return hdr->forward;
        case GC_RAW_KIND: {
            void *newobj = gc_alloc(hdr->objsize);
            memcpy(newobj, ptr, hdr->objsize);
            hdr->forward = newobj;
            hdr->kind = GC_FORWARD_KIND;
            return newobj;
        }
        case GC_ARRAY_KIND: {
            int copied = 0;
            GcArray *arr = ptr;
            int own_buf = (arr->ptr == (void *)arr->data);
            GcArray *newarr = gc_alloc_array(arr->len, own_buf, arr->isobj);
            if (arr->ptr) {
                memcpy(newarr->ptr, arr->ptr, arr->len * sizeof(uintptr_t));
                copied = 1;
            }
            hdr->forward = newarr;
            hdr->kind = GC_FORWARD_KIND;
            if (newarr->isobj && copied) {
                void **elems = (void **)newarr->ptr;
                for (int i = 0; i < newarr->len; i++) elems[i] = copy(elems[i]);
            }
            return newarr;
        }
        case GC_STRUCT_KIND: {
            objmap_t *map = hdr->objmap;
            void *newstruct = gc_alloc_struct(map, hdr->objsize);
            memcpy(newstruct, ptr, hdr->objsize);
            hdr->forward = newstruct;
            hdr->kind = GC_FORWARD_KIND;
            if (map) {
                for (int i = 0; i < map->num; i++) {
                    void **field = (void **)(newstruct + map->offset[i]);
                    *field = copy(*field);
                }
            }
            return newstruct;
        }
        case GC_OBJECT_KIND: {
            objmap_t *map = hdr->objmap;
            int objsize = hdr->objsize;
            GcObject *obj = ptr;
            GcObject *newobj = gc_alloc_obj(objsize, obj->type, obj->vtbl, map);
            // copy all includes GcObject
            memcpy(newobj, obj, objsize);
            hdr->forward = newobj;
            hdr->kind = GC_FORWARD_KIND;
            if (map) {
                void *newptr = newobj + 1;
                for (int i = 0; i < map->num; i++) {
                    void **field = (void **)(newptr + map->offset[i]);
                    *field = copy(*field);
                }
            }
            return newobj;
        }
        default: {
            printf("gc-error: invalid gcheader(kind %d?)\n", hdr->kind);
            abort();
        }
    }
}

void gc(void)
{
    printf("gc-debug: start to gc\n");

    // wait other threads stopped.
    // TODO

    // swap space
    char *tmp = from_space;
    from_space = to_space;
    to_space = tmp;
    free_ptr = from_space;
    memset(from_space, 0, space_size);

    // foreach roots
    void **pptr;
    GcTrace *root = gcroots;
    while (root) {
        for (int i = 0; i < root->nroots; i++) {
            pptr = ((void **)(root + 1))[i];
            if (!*pptr) continue;
            *pptr = copy(*pptr);
        }
        root = root->prev;
    }

    printf("gc-debug: gc finished\n");
}

#ifdef __cplusplus
}
#endif
