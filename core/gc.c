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

#define GC_RAW_KIND     1
#define GC_STRUCT_KIND  2
#define GC_OBJECT_KIND  3
#define GC_FORWARD_KIND 4

typedef struct _KlGcHdr KlGcHdr;
typedef struct _KlGcWrap KlGcWrap;

/* gc object header */
struct _KlGcHdr {
    int kind;
    int size;
    uintptr ptr;
};

struct _KlGcWrap {
    void *ptr;
};

/* semi-copy space */
static int space_size;
static char *space_ptr;

static char *from_space;
static char *to_space;
static char *free_ptr;

static int gc_running = 0;

/* all roots */
void *gcroots;

static KlGcHdr *__new__(int size)
{
    char *new_ptr = null;
    int tried = 0;
    int objsize = sizeof(KlGcHdr) + ALIGN_PTR(size);

    for (;;) {
        new_ptr = free_ptr + objsize;
        if (new_ptr <= (from_space + space_size)) break;
        int waste = space_size - (free_ptr - from_space);
        if (gc_running || tried) {
            debug("alloc size: %d failed(left: %d)", objsize, waste);
            panic("too small managed memory(total: %d)", space_size);
        }
        debug("alloc size: %d failed(left: %d)", objsize, waste);
        tried = 1;
        kl_gc();
    }

    debug("%d bytes allocated", objsize);
    KlGcHdr *hdr = (KlGcHdr *)free_ptr;
    hdr->size = ALIGN_PTR(size);
    // 8 bytes alignment
    assert(!((uintptr)hdr & 7));
    free_ptr = new_ptr;
    return hdr;
}

/**
 * +-------------+
 * |  gc header  |
 * +-------------+
 * |  raw data   |
 * +-------------+
 *
 */
void *kl_gc_alloc_raw(int size)
{
    debug("try to alloc raw: %d", size);

    KlGcHdr *hdr = __new__(size);
    hdr->kind = GC_RAW_KIND;
    hdr->ptr = 0;

    // 8 bytes alignment
    assert(!((uintptr)(hdr + 1) & 7));
    return (void *)(hdr + 1);
}

/**
 * +----------------+
 * |  gc header     | =====> +----------+
 * +----------------+        |  kind    |
 * |                |        |  size    |
 * |  struct data   |        |  objmap  |
 * |                |        +----------+
 * +----------------+
 */
void *kl_gc_alloc_struct(int size, int *objmap)
{
    debug("try to alloc struct: %d", size);

    KlGcHdr *hdr = __new__(size);
    hdr->kind = GC_STRUCT_KIND;
    hdr->ptr = (uintptr)objmap;

    // 8 bytes alignment
    assert(!((uintptr)(hdr + 1) & 7));
    return (void *)(hdr + 1);
}

static int wrap_objmap[] = {
    1,
    offsetof(KlWrapper, data),
};

void *kl_gc_alloc_wrap(void)
{
    return kl_gc_alloc_struct(sizeof(KlWrapper), wrap_objmap);
}

/**
 * +----------------+
 * |  gc header     |
 * +----------------+
 * |                |
 * |  object data   |
 * |                |
 * +----------------+
 */
void *kl_gc_alloc(int num)
{
    debug("try to alloc object: %d", num);

    int size = num * sizeof(KlValue);
    KlGcHdr *hdr = __new__(size);
    hdr->kind = GC_OBJECT_KIND;
    hdr->size = num;

    // 8 bytes alignment
    assert(!((uintptr)(hdr + 1) & 7));
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
            return (void *)newraw;
        }
        case GC_STRUCT_KIND: {
            int *objmap = (int *)hdr->ptr;
            void *newobj = kl_gc_alloc_struct(hdr->size, objmap);
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
        case GC_OBJECT_KIND: {
            int num_objs = hdr->size;
            void *newobj = kl_gc_alloc(num_objs);
            memcpy(newobj, ptr, num_objs * sizeof(KlValue));
            hdr->kind = GC_FORWARD_KIND;
            hdr->ptr = (uintptr)newobj;

            KlValue *elem = (KlValue *)newobj;
            KlFuncTbl *vtbl;
            for (int i = 0; i < num_objs; i++) {
                vtbl = elem->vtbl;
                if (vtbl && vtbl->type->gc) {
                    elem->obj = copy(elem->obj);
                }
                elem++;
            }
            return newobj;
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
    gc_running = 1;

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

    gc_running = 0;
    debug("=== gc-mem: %d totoal, %ld used, %ld avail", space_size,
          (free_ptr - from_space), space_size - (free_ptr - from_space));
    debug("=== gc finished ===");
}

#ifdef __cplusplus
}
#endif
