/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_BIN_HEAP_H_
#define _KOALA_BIN_HEAP_H_

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
typedef struct binheap_entry {
    unsigned int idx;
} binheap_entry_t;

/*
 * `cmp` function must return 1, if the parent P and his child C are in the
 * proper order relative to each other in the binary heap.
 * To do that, in a min-heap `cmp` must return 1 when P <= C, and in a
 * max-heap it must return 1 inversely P >= C.
 */
typedef int (*binheap_cmp_t)(binheap_entry_t *p, binheap_entry_t *c);

/* binary heap structure, root is at index 1, valid indices 1 through n */
typedef struct binheap {
    /* compare function */
    binheap_cmp_t cmp;
    /* used must not greater than size */
    unsigned int used;
    /* `entries` array capacity */
    unsigned int cap;
    /* `entries` array pointer */
    binheap_entry_t **entries;
} binheap_t;

/* initialize a binary heap with default size */
int binheap_init(binheap_t *heap, int size, binheap_cmp_t cmp);

/* finalize a binary heap */
void binheap_fini(binheap_t *heap);

/* insert an entry into the binary heap */
int binheap_insert(binheap_t *heap, binheap_entry_t *e);

/* get and delete the root entry from the binary heap */
binheap_entry_t *binheap_pop(binheap_t *heap);

/* get and NOT REMOVE the root entry from the binary heap */
binheap_entry_t *binheap_top(binheap_t *heap);

/* delete the specified entry from the binary heap */
int binheap_delete(binheap_t *heap, binheap_entry_t *e);

/* iterate heap, if it's first time, `e` is null. */
binheap_entry_t *binheap_next(binheap_t *heap, binheap_entry_t *e);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BIN_HEAP_H_ */
