/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "gc.h"
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

typedef enum _GcKind {
    GC_NONE_KIND,
    GC_OBJECT_KIND,
    GC_ARRAY_KIND,
    GC_FORWARD_KIND,
} GcKind;

typedef struct {
    uint32 isobj : 1;
    uint32 size : 31;
} GcArrayInfo;

/* gc header */
typedef struct _GcHeader {
    uint32 kind : 2;
    uint32 objsize : 30;
    union {
        void *forward;
        int *objmap;
        GcArrayInfo arrinfo;
    };
    GcFiniFunc fini;
} GcHeader, *GcHeaderRef;

/* semi-copy space */
static const int space_size = 200;
static char *from_space;
static char *to_space;
static char *free_ptr;

/* all roots */
void *gcroots;

/* objects with finalize func */
static Vector fini_objs[2];
static VectorRef from_fini_objs;
static VectorRef to_fini_objs;

static GcHeaderRef __new__(int size)
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

    GcHeaderRef hdr = (GcHeaderRef)free_ptr;
    // 8 bytes alignment
    assert(!((uintptr_t)hdr & 7));

    free_ptr = new_ptr;
    hdr->objsize = size;
    return hdr;
}

void *gc_alloc(int size, int *objmap, GcFiniFunc fini)
{
    assert(size > 0);
    GcHeaderRef hdr = __new__(size);
    hdr->kind = GC_OBJECT_KIND;
    hdr->objmap = objmap;
    hdr->fini = fini;
    if (fini) vector_push_back(from_fini_objs, &hdr);
    return (void *)(hdr + 1);
}

void *gc_alloc_array(int size, int isobj)
{
    printf("gc-debug: alloc array [%d], isobj: %d\n", size, isobj);
    GcHeaderRef hdr = __new__(size);
    hdr->kind = GC_ARRAY_KIND;
    hdr->arrinfo.isobj = isobj;
    hdr->arrinfo.size = size;
    return (void *)(hdr + 1);
}

void gc_expand_array(void **ptr, int size)
{
    void *arr = *ptr;
    GcHeaderRef hdr = (GcHeaderRef)arr - 1;
    int oldsize = hdr->arrinfo.size;
    int newsize = oldsize + size;
    int isobj = hdr->arrinfo.isobj;
    printf("gc-debug: expand array [%d] -> [%d] isobj: %d\n", oldsize, newsize,
           isobj);
    void *newarr = gc_alloc_array(newsize, isobj);
    memcpy(newarr, arr, oldsize);
    *ptr = newarr;
}

void gc_init(void)
{
    from_space = mm_alloc(space_size);
    to_space = mm_alloc(space_size);
    free_ptr = from_space;

    from_fini_objs = &fini_objs[0];
    to_fini_objs = &fini_objs[1];
    vector_init_ptr(from_fini_objs);
    vector_init_ptr(to_fini_objs);
}

void gc_fini(void)
{
    mm_free(from_space);
    mm_free(to_space);

    vector_fini(from_fini_objs);
    vector_fini(to_fini_objs);
}

static void *copy(void *ptr)
{
    if (!ptr) return NULL;

    GcHeaderRef hdr = (GcHeaderRef)ptr - 1;

    switch (hdr->kind) {
        case GC_FORWARD_KIND:
            printf("gc-debug: already copied\n");
            return hdr->forward;
        case GC_ARRAY_KIND: {
            GcArrayInfo arrinfo = hdr->arrinfo;
            void *newarr = gc_alloc_array(arrinfo.size, arrinfo.isobj);
            memcpy(newarr, ptr, arrinfo.size);
            hdr->forward = newarr;
            hdr->kind = GC_FORWARD_KIND;
            if (arrinfo.isobj) {
                int num_objs = arrinfo.size / PTR_SIZE;
                void **elems = (void **)newarr;
                for (int i = 0; i < num_objs; i++) elems[i] = copy(elems[i]);
            }
            return newarr;
        }
        case GC_OBJECT_KIND: {
            int *objmap = hdr->objmap;
            void *newobj = gc_alloc(hdr->objsize, objmap, hdr->fini);
            memcpy(newobj, ptr, hdr->objsize);
            hdr->forward = newobj;
            hdr->kind = GC_FORWARD_KIND;
            if (objmap) {
                int num_fields = objmap[0];
                int *offset = objmap + 1;
                for (int i = 0; i < num_fields; i++) {
                    void **field = (void **)(newobj + offset[i]);
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

    // exchange from_fini_objs and to_fini_objs
    VectorRef vec = from_fini_objs;
    from_fini_objs = to_fini_objs;
    to_fini_objs = vec;
    vector_clear(from_fini_objs);

    // foreach roots
    void **pptr;
    void **root = (void **)gcroots;
    while (root) {
        int nroots = (int)(uintptr_t)root[0];
        for (int i = 0; i < nroots; i++) {
            pptr = root[2 + i];
            if (!*pptr) continue;
            *pptr = copy(*pptr);
        }
        root = (void **)root[1];
    }

    // call object's fini func
    void **item;
    vector_foreach(item, to_fini_objs, {
        GcHeaderRef hdr = (GcHeaderRef)(*item);
        if (hdr->kind != GC_OBJECT_KIND) continue;
        printf("gc-debug: object is freed\n");
        if (hdr->fini) hdr->fini(hdr + 1);
    });

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

    printf("gc-debug: gc finished\n");
    printf("gc-debug: %ld available\n", space_size - (free_ptr - from_space));
}

void gc_stat(void)
{
    puts("------ GC Memory Usage ------");
    printf("%d bytes total\n", space_size);
    printf("%ld bytes used\n", free_ptr - from_space);
    puts("--------------------------");
}

#ifdef __cplusplus
}
#endif
