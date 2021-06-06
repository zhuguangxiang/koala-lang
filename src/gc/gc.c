/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"

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
    /* C struct or koala object */
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
        /* object(struct) map */
        objmap_t *objmap;
    };
} GcHeader;

static const int space_size = 340;
static char *from_space;
static char *to_space;
static char *free_ptr;
GcTrace *gcroots;
static Vector fini_objs[2];
static Vector *from_fini_objs;
static Vector *to_fini_objs;

typedef struct obj_fini_info {
    void *obj;
    gc_fini_func fini;
} obj_fini_info_t;

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
    assert(!((uintptr)hdr & 7));

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

static inline void *__alloc_object(int size, void *objmap)
{
    GcHeader *hdr = __new__(size);
    hdr->kind = GC_OBJECT_KIND;
    hdr->objmap = objmap;
    return (void *)(hdr + 1);
}

static inline void set_obj_fini_func(void *obj, gc_fini_func func)
{
    obj_fini_info_t info = { obj, func };
    VectorPushBack(from_fini_objs, &info);
}

void *gc_alloc_object(int size, void *objmap, gc_fini_func fini)
{
    assert(size > 0);
    void *obj = __alloc_object(size, objmap);
    if (fini) {
        set_obj_fini_func(obj, fini);
    }
    return obj;
}

GcArray *gc_alloc_array(int len, int own_buf, int isobj)
{
    GcArray *arr = nil;
    gc_push(&arr);

    printf("gc-debug: alloc array [%d]\n", len);

    int size = sizeof(GcArray);
    if (own_buf) size += len * sizeof(uintptr);

    GcHeader *hdr = __new__(size);
    hdr->kind = GC_ARRAY_KIND;
    arr = (GcArray *)(hdr + 1);
    arr->len = len;
    arr->isobj = isobj;

    void *data;
    if (own_buf)
        data = arr->data;
    else {
        hdr = __new__(len * sizeof(uintptr));
        data = (void *)(hdr + 1);
    }

    arr->ptr = data;

    gc_pop();

    return arr;
}

GcArray *gc_expand_array(GcArray *arr, int newlen)
{
    if (arr->ptr != arr->data) {
        GcHeader *hdr = __new__(newlen * sizeof(uintptr));
        memcpy(hdr + 1, arr->ptr, arr->len * sizeof(uintptr));
        arr->ptr = hdr + 1;
        arr->len = newlen;
        return arr;
    } else {
        GcArray *newarr = gc_alloc_array(newlen, 1, arr->isobj);
        memcpy(newarr->ptr, arr->ptr, arr->len * sizeof(uintptr));
        return newarr;
    }
}

void gc_init(void)
{
    from_space = MemAlloc(space_size);
    to_space = MemAlloc(space_size);
    free_ptr = from_space;

    from_fini_objs = &fini_objs[0];
    to_fini_objs = &fini_objs[1];
    VectorInit(from_fini_objs, sizeof(obj_fini_info_t));
    VectorInit(to_fini_objs, sizeof(obj_fini_info_t));
}

void gc_fini(void)
{
    MemFree(from_space);
    MemFree(to_space);
    VectorFini(from_fini_objs);
    VectorFini(to_fini_objs);
}

static void *copy(void *ptr)
{
    if (!ptr) return nil;

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
                memcpy(newarr->ptr, arr->ptr, arr->len * sizeof(uintptr));
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
        case GC_OBJECT_KIND: {
            objmap_t *map = hdr->objmap;
            void *newobj = __alloc_object(hdr->objsize, map);
            memcpy(newobj, ptr, hdr->objsize);
            hdr->forward = newobj;
            hdr->kind = GC_FORWARD_KIND;
            if (map) {
                for (int i = 0; i < map->num; i++) {
                    void **field = (void **)(newobj + map->offset[i]);
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

    // update fini_objs
    Vector *tmp_vec = from_fini_objs;
    from_fini_objs = to_fini_objs;
    to_fini_objs = tmp_vec;
    VectorClear(from_fini_objs);

    obj_fini_info_t *fi;
    GcHeader *hdr;
    VectorForEach(fi, to_fini_objs)
    {
        hdr = (GcHeader *)fi->obj - 1;
        if (hdr->kind != GC_FORWARD_KIND) {
            printf("call object fini func\n");
            fi->fini(fi->obj);
        } else {
            printf("update fini_objs\n");
            set_obj_fini_func(hdr->forward, fi->fini);
        }
    }

    // call finalized objects
    printf("gc-debug: gc finished\n");
}

#ifdef __cplusplus
}
#endif
