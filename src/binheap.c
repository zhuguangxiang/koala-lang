/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "binheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEAP_MIN_SIZE 32

int binheap_init(binheap_t *heap, int size, binheap_cmp_t cmp)
{
    if (size < HEAP_MIN_SIZE) size = HEAP_MIN_SIZE;
    int mm_size = sizeof(binheap_entry_t *) * (size + 1);
    binheap_entry_t **entries = malloc(mm_size);
    if (!entries) return -1;

    heap->cmp = cmp;
    /* index 0 is not used in heap */
    heap->used = 0;
    heap->cap = size;
    heap->entries = entries;
    return 0;
}

void binheap_fini(binheap_t *heap)
{
    free(heap->entries);
}

/*
 * root is at index 1, valid indices 1...n
 * https://en.wikipedia.org/wiki/Binary_heap
 */
#define PARENT(i) ((i) >> 1)
#define LCHILD(i) ((i) << 1)
#define RCHILD(i) (((i) << 1) + 1)

static void __heap_up(binheap_t *heap, binheap_entry_t *e)
{
    binheap_entry_t *p;
    unsigned int eidx, pidx;

    eidx = e->idx;

    do {
        /* stop if the entry is root */
        if (eidx == 1) break;

        /* if parent is `greater` than child, stop */
        pidx = PARENT(eidx);
        p = heap->entries[pidx];
        if (heap->cmp(p, e)) break;

        /* down parent to `eidx` position */
        p->idx = eidx;
        heap->entries[eidx] = p;

        /* `edix` up to `pidx` */
        eidx = pidx;
    } while (1);

    /* update 'e' in last stop position */
    heap->entries[eidx] = e;
    e->idx = eidx;
}

static void __heap_down(binheap_t *heap, binheap_entry_t *e)
{
    unsigned int eidx, lidx, ridx, swapidx;

    eidx = e->idx;

    do {
        /* stop if the entry is leaf */
        lidx = LCHILD(eidx);
        if (lidx > heap->used) break;

        /* get the 'greater` one from right and left child */
        ridx = RCHILD(eidx);
        if (ridx <= heap->used &&
            heap->cmp(heap->entries[ridx], heap->entries[lidx]))
            swapidx = ridx;
        else
            swapidx = lidx;

        /* if parent is `greater` than children, stop */
        if (heap->cmp(e, heap->entries[swapidx])) break;

        /* up `greater` child to `edix` position */
        heap->entries[swapidx]->idx = eidx;
        heap->entries[eidx] = heap->entries[swapidx];

        /* `edix` down to `swapidx` */
        eidx = swapidx;
    } while (1);

    /* update 'e' in last stop position */
    heap->entries[eidx] = e;
    e->idx = eidx;
}

/*
 * enlarge the array by heap->cap multiply 2
 */
static int __heap_grow(binheap_t *heap, unsigned int size)
{
    if (size <= heap->cap) return 0;
    binheap_entry_t **new_entries;
    int new_cap = heap->cap << 1;
    // printf("heap grow %d -> %d\n", heap->cap + 1, new_cap + 1);
    new_entries = malloc(sizeof(binheap_entry_t *) * (new_cap + 1));
    if (!new_entries) return -1;
    int old_mm_size = sizeof(binheap_entry_t *) * (heap->cap + 1);
    memcpy(new_entries, heap->entries, old_mm_size);
    free(heap->entries);
    heap->entries = new_entries;
    heap->cap = new_cap;
    return 0;
}

int binheap_insert(binheap_t *heap, binheap_entry_t *e)
{
    unsigned int used = heap->used + 1;
    if (__heap_grow(heap, used)) {
        // printf("error: grow binary heap failed.\n");
        return -1;
    }

    heap->used = used;
    heap->entries[used] = e;
    e->idx = used;

    __heap_up(heap, e);
    return 0;
}

binheap_entry_t *binheap_pop(binheap_t *heap)
{
    if (!heap->used) return NULL;
    binheap_entry_t *root = heap->entries[1];
    binheap_delete(heap, root);
    return root;
}

binheap_entry_t *binheap_top(binheap_t *heap)
{
    if (!heap->used) return NULL;
    return heap->entries[1];
}

int binheap_delete(binheap_t *heap, binheap_entry_t *e)
{
    unsigned int eidx = e->idx;

    if (eidx <= 0 || eidx > heap->used) {
        // printf("error: invalid entry.\n");
        return -1;
    }

    /* if the last entry is removed, nothing more need be done. */
    if (eidx == heap->used) {
        e->idx = 0;
        heap->used--;
        return 0;
    }

    /* move the last entry to the to be deleted `entry` position. */
    binheap_entry_t *last = heap->entries[heap->used];
    heap->entries[eidx] = last;
    last->idx = eidx;
    heap->used--;

    /*
     * the entry and its parent are in the proper orer,
     * or the entry is root, do heap down.
     */
    unsigned int pidx = PARENT(eidx);
    if (eidx == 1 || heap->cmp(heap->entries[pidx], last))
        __heap_down(heap, last);
    /*
     * the entry and its child are in the proper order, do heap up.
     */
    else
        __heap_up(heap, last);

    /* clear 'index' in the entry */
    e->idx = 0;
    return 0;
}

/* iterate heap */
binheap_entry_t *binheap_next(binheap_t *heap, binheap_entry_t *e)
{
    if (!e) return binheap_top(heap);
    unsigned int eidx = e->idx;
    if (eidx <= 0 || eidx >= heap->used) return NULL;
    return heap->entries[eidx + 1];
}

#ifdef __cplusplus
}
#endif
