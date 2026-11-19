/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_GC_H_
#define _KOALA_GC_H_

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Single-threaded, stop-the-world, tri-color mark & sweep. */

/* max memory is KOALA_GC_MAX (e.g. 512M, 2G), 1G by default; exceeding it panics */
void gc_init(void);
void gc_fini(void);

/* normal object, starts with OBJECT_HEAD */
void *gc_alloc(int size);
/* perm object, starts with OBJECT_HEAD, never freed; a root one (root != 0) is traced by its
   type's _mark on every collection */
void *gc_alloc_perm(int size, int root);
/* TValue array of n elements, each value is marked */
TValue *gc_alloc_values(int n);
/* byte array, contains no references */
void *gc_alloc_bytes(int size);
/* perm byte array, never freed */
void *gc_alloc_perm_bytes(int size);
/* perm TValue array, never scanned or freed: its values must be perm too */
TValue *gc_alloc_perm_values(int n);

// Allocate a GC-managed object of the same size as the type pointed to by `ptr`
#define gc_alloc_obj(ptr) gc_alloc(sizeof(*(ptr)))

// Allocate a perm GC-managed object of the same size as the type pointed to by `ptr`
#define gc_alloc_perm_obj(ptr, root) gc_alloc_perm(sizeof(*(ptr)), (root))

/* free a normal block now, without any callback; the caller releases its private data and
   makes sure nothing else refers to it */
void gc_free(void *ptr);

/* called from roots and TypeObject._mark */
void gc_mark(void *ptr);
void gc_mark_value(TValue *val);

/* full collection, roots come from every coroutine of the current scheduler */
size_t gc_collect(void);

/* allocations minus deallocations since the last collection, as python */
int64_t gc_get_count(void);
/* blocks on the normal list */
size_t gc_normal_count(void);
/* blocks on the perm list */
size_t gc_perm_count(void);

/* one collection; sizes include block headers */
typedef struct _GCCycleStats {
    size_t scanned;
    size_t freed;
    size_t freed_size;
    size_t alive;
    size_t alive_size;
    /* blocks with age over the threshold, not collected */
    size_t old;
    double elapsed_ms;
} GCCycleStats;

/* accumulated over all collections, and the last one */
typedef struct _GCStats {
    size_t collections;
    size_t collected;
    size_t collected_size;
    GCCycleStats last;
} GCStats;

void gc_get_stats(GCStats *stats);

/* block kinds */
#define GC_KIND_OBJECT 1 /* starts with OBJECT_HEAD, scanned by _mark */
#define GC_KIND_VALUES 2 /* TValue array, each value is marked */
#define GC_KIND_BYTES  3 /* byte array, no references */

/* observers, called by the collector; NULL members are skipped */
typedef struct _GCHooks {
    /* a block is about to be freed by a collection */
    void (*on_free)(void *ptr, int kind, size_t size);
    /* a collection is done */
    void (*on_collect)(const GCStats *stats);
} GCHooks;

/* NULL removes the hooks */
void gc_set_hooks(const GCHooks *hooks);

/* bytes held by gc blocks, headers included */
size_t gc_used(void);
/* KOALA_GC_MAX, 1G by default */
size_t gc_max(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
