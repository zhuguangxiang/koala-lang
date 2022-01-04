/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "kltypes.h"
#include "util/log.h"
#include "util/mm.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The gc algorithm:
 * semi-space copying
 * see: `The Garbage Collection Handbook`
 */

typedef struct _KlGcHdr KlGcHdr;

#define GC_RAW_KIND     0
#define GC_OBJECT_KIND  1
#define GC_ARRAY_KIND   2
#define GC_FORWARD_KIND 3

/* gc header */
struct _KlGcHdr {
    uint32 kind : 2;
    uint32 size : 30;
    uintptr ptr;
};

/* semi-copy space */
static int space_size;
static char *space_ptr;

static char *from_space;
static char *to_space;
static char *free_ptr;

/* all roots */
void *gcroots;

static KlGcHdr *__new__(int size)
{
    int objsize = ALIGN_PTR(sizeof(KlGcHdr) + size);
    char *new_ptr;
    int tried = 0;

Lrealloc:
    new_ptr = free_ptr + objsize;
    if (new_ptr > (from_space + space_size)) {
        int waste = space_size - (free_ptr - from_space);
        if (tried) {
            debug("alloc size:%d failed", objsize);
            debug("%d bytes wasted(total:%d)", waste, space_size);
            panic("too small managed memory");
        }
        debug("alloc size:%d failed", objsize);
        debug("%d bytes wasted(total:%d)", waste, space_size);
        tried = 1;
        kl_gc();
        goto Lrealloc;
    }

    debug("alloc: %d", objsize);

    KlGcHdr *hdr = (KlGcHdr *)free_ptr;
    // 8 bytes alignment
    assert(!((uintptr)hdr & 7));

    free_ptr = new_ptr;
    hdr->size = size;
    return hdr;
}

void *kl_gc_alloc(int size, int *objmap)
{
    assert(size > 0);
    debug("try to alloc object:%d", size);
    KlGcHdr *hdr = __new__(size);
    hdr->kind = GC_OBJECT_KIND;
    hdr->ptr = (uintptr)objmap;
    return (void *)(hdr + 1);
}

void *kl_gc_alloc_array(int num)
{
    int size = num * sizeof(KlValue);
    debug("try to alloc array:%d", num);
    KlGcHdr *hdr = __new__(size);
    hdr->kind = GC_ARRAY_KIND;
    return (void *)(hdr + 1);
}

void *kl_gc_alloc_raw(int size)
{
    debug("try to alloc raw:%d", size);
    KlGcHdr *hdr = __new__(size);
    hdr->kind = GC_RAW_KIND;
    return (void *)(hdr + 1);
}

void kl_gc_init(int size)
{
    space_size = size;
    from_space = mm_alloc(space_size);
    to_space = mm_alloc(space_size);
    free_ptr = from_space;
}

void kl_gc_fini(void)
{
    mm_free(from_space);
    mm_free(to_space);
}

static void *copy(void *ptr)
{
    if (!ptr) return null;

    KlGcHdr *hdr = (KlGcHdr *)ptr - 1;

    switch (hdr->kind) {
        case GC_RAW_KIND: {
            void *newraw = kl_gc_alloc_raw(hdr->size);
            memcpy(newraw, ptr, hdr->size);
            hdr->kind = GC_FORWARD_KIND;
            hdr->ptr = (uintptr)newraw;
            return newraw;
        }
        case GC_OBJECT_KIND: {
            int *objmap = (int *)hdr->ptr;
            void *newobj = kl_gc_alloc(hdr->size, objmap);
            memcpy(newobj, ptr, hdr->size);
            hdr->kind = GC_FORWARD_KIND;
            hdr->ptr = (uintptr)newobj;
            if (objmap) {
                int num = objmap[0];
                int *offset = objmap + 1;
                void **field;
                for (int i = 0; i < num; i++) {
                    field = (void **)(newobj + offset[i]);
                    *field = copy(*field);
                }
            }
            return newobj;
        }
        case GC_ARRAY_KIND: {
            int num_objs = hdr->size / sizeof(KlValue);
            void *newarr = kl_gc_alloc_array(num_objs);
            memcpy(newarr, ptr, hdr->size);
            hdr->kind = GC_FORWARD_KIND;
            hdr->ptr = (uintptr)newarr;
            KlValue **elems = (KlValue **)newarr;
            for (int i = 0; i < num_objs; i++) {
                // elems[i] = copy(elems[i]);
            }
            return newarr;
        }
        case GC_FORWARD_KIND: {
            debug("already copied");
            return (void *)hdr->ptr;
        }
        default: {
            panic("invalid gc header(kind %d?)", hdr->kind);
        }
    }
}

void kl_gc(void)
{
    debug("=== gc is starting ===");

    // TODO: wait other threads stopped?

    // swap space
    char *tmp = from_space;
    from_space = to_space;
    to_space = tmp;
    free_ptr = from_space;
    memset(from_space, 0, space_size);

    // exchange from_fini_objs and to_fini_objs
    /*
    VectorRef vec = from_fini_objs;
    from_fini_objs = to_fini_objs;
    to_fini_objs = vec;
    vector_clear(from_fini_objs);
    */

    // foreach roots
    void **pptr;
    void **root = (void **)gcroots;
    while (root) {
        int nroots = (int)(uintptr)root[0];
        for (int i = 0; i < nroots; i++) {
            pptr = root[2 + i];
            if (!pptr || !*pptr) continue;
            *pptr = copy(*pptr);
        }
        root = (void **)root[1];
    }

    // call object's fini func
    /*
    void **item;
    vector_foreach(item, to_fini_objs, {
        GcHeaderRef hdr = (GcHeaderRef)(*item);
        if (hdr->kind != GC_OBJECT_KIND) continue;
        printf("gc-debug: object is freed\n");
        if (hdr->fini) hdr->fini(hdr + 1);
    });
    */

    /*
    GcHeaderRef hdr = (GcHeaderRef)to_space;
    char *end = to_space + space_size;
    while ((char *)hdr < end && hdr->kind != GC_NONE_KIND) {
        if (hdr->kind == GC_OBJECT_KIND) {
            printf("gc-debug: object is freed\n");
            if (hdr->fini) hdr->fini(hdr + 1);
        }

        if (hdr->kind == GC_ARRAY_KIND) {
            printf("gc-debug: array is freed\n");
        }

        hdr = (GcHeaderRef)((char *)(hdr + 1) + hdr->objsize);
    }
    */

    debug("=== gc finished ===");
    debug("%d totoal, %ld used, %ld avail", space_size, (free_ptr - from_space),
          space_size - (free_ptr - from_space));
}

#ifdef __cplusplus
}
#endif
