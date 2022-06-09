/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_BINHEAP_H_
#define _KOALA_BINHEAP_H_

/*
 * binary heap is a complete tree, so use often an array to implement it
 * such as:
 *                   1
 *                 /   \
 *                2     3
 *               / \   / \
 *              4  5  6   7
 *             / \ ... ...
 *            ......
 * Heaps are suited to supporting a sorted list with add and delete,
 * but no lookup.
 * A common application is a (priority) queue, where elements
 * are added at any point, and removed from the front.
 * A heap is a rooted binary tree of elements with an associated comparision
 * function between elements satisfying the property that:
 * element P >= element C(or < )
 * where C(child) is a neighbor of P(parent), and P is nearer the root than C.
 *
 * This implementation uses an array with the elements in slots 1...N, so this
 * translates as:(max heap)
 * x(i) >= x(2i) and x(i) >= x(2i+1) for all i(where this is within the array)
 *
 * index 0 is not used.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * `heap_entry_t` stores the index value of an array. Using binary_heap_t,
 * you must include heap_entry_t in your structure and give a compare function.
 */
typedef struct _BinHeapEntry {
    unsigned int idx;
} BinHeapEntry;

/*
 * `cmp` function must return 1, if the parent P and his child C are in the
 * proper order relative to each other in the binary heap.
 * To do that, in a min-heap `cmp` must return 1 when P <= C, and in a
 * max-heap it must return 1 inversely P >= C.
 */
typedef int (*BinHeapCmpFunc)(BinHeapEntry *p, BinHeapEntry *c);

/* binary heap structure, root is at index 1, valid indices 1 through n */
typedef struct _BinHeap {
    /* compare function */
    BinHeapCmpFunc cmp;
    /* used must not greater than size */
    unsigned int used;
    /* `entries` array capacity */
    unsigned int cap;
    /* `entries` array pointer */
    BinHeapEntry **entries;
} BinHeap;

/* initialize a binary heap with default size */
int binheap_init(BinHeap *heap, int size, BinHeapCmpFunc cmp);

/* finalize a binary heap */
void binheap_fini(BinHeap *heap);

/* insert an entry into the binary heap */
int binheap_insert(BinHeap *heap, BinHeapEntry *e);

/* get and delete the root entry from the binary heap */
BinHeapEntry *binheap_pop(BinHeap *heap);

/* get and NOT REMOVE the root entry from the binary heap */
BinHeapEntry *binheap_top(BinHeap *heap);

/* delete the specified entry from the binary heap */
int binheap_delete(BinHeap *heap, BinHeapEntry *e);

/* iterate heap, if it's first time, `e` is null. */
BinHeapEntry *binheap_next(BinHeap *heap, BinHeapEntry *e);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BINHEAP_H_ */
