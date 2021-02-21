/*
 * This file is part of the koala project, under the MIT License.
 * Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
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
typedef int (*binheap_cmp_func)(BinHeapEntry *p, BinHeapEntry *c);

/* binary heap structure, root is at index 1, valid indices 1 through n */
typedef struct _BinHeap {
    /* compare function */
    binheap_cmp_func cmp;
    /* used must not greater than size */
    unsigned int used;
    /* `entries` array capacity */
    unsigned int cap;
    /* `entries` array pointer */
    BinHeapEntry **entries;
} BinHeap;

typedef BinHeap *BinHeapRef;

/* initialize a binary heap with default size */
int binheap_init(BinHeapRef heap, int size, binheap_cmp_func cmp);

/* finalize a binary heap */
void binheap_fini(BinHeapRef heap);

/* insert an entry into the binary heap */
int binheap_insert(BinHeapRef heap, BinHeapEntry *e);

/* get and delete the root entry from the binary heap */
BinHeapEntry *binheap_pop(BinHeapRef heap);

/* get and NOT REMOVE the root entry from the binary heap */
BinHeapEntry *binheap_top(BinHeapRef heap);

/* delete the specified entry from the binary heap */
int binheap_delete(BinHeapRef heap, BinHeapEntry *e);

/* iterate heap, if it's first time, `e` is null. */
BinHeapEntry *binheap_next(BinHeapRef heap, BinHeapEntry *e);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BINHEAP_H_ */
