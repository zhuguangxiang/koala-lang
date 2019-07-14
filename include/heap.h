/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Binary heap is a complete tree, so use often an array to implement it.
 *
 *                   1
 *                 /   \
 *                2     3
 *               / \   / \
 *              4   5 6   7
 *             / \  ...
 *
 * Heaps are suited to supporting a sorted list with add and delete,
 * but no lookup.
 *
 * A typical application is a priority queue, where items are added
 * at any point, and removed only from the front.
 *
 * This implementation uses an array with the elements in slots 1...N,
 * so this translates as: (e.g. max heap)
 * x(i) >= x(2i) and x(i) >= x(2i+1) for all i, 0 is not used.
 */


#ifndef _KOALA_HEAP_H_
#define _KOALA_HEAP_H_

#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

struct heap {
  /* allocated array size */
  int capacity;
  /* used array size */
  int size;
  /* compare function */
  int (*cmp)(void *, void *);
  /* items array */
  void **items;
};

/*
 * Initialize a binary heap. Depending on the 'cmp' function behavior,
 * it can be a min-heap or a max-heap.
 * The 'cmp' function with which to sort items in the heap.
 * It must return < 0, if a < b, 0 if a == b, and > 0 if a > b.
 */
int heap_init(struct heap *self, int (*cmp)(void *, void *));

/* Free the binary heap and its memory. The items in the heap are not freed. */
void heap_free(struct heap *self);

/*
 * Add a new item into the heap. The item will be sorted into correct position
 * according to the heap's 'cmp' function.
 */
int heap_push(struct heap *self, void *item);

/*
 * Remove the top item from the heap. The remainings are sorted into place with
 * the heap's 'cmp' function.
 */
void *heap_pop(struct heap *self);

/*
 * Iterator callback function for heap iteration.
 * See iterator.h
 */
void *heap_iter_next(struct iterator *iter);

/* Declare an iterator of the heap. */
#define HEAP_ITERATOR(name, heap) \
  ITERATOR(name, heap, heap_iter_next)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HEAP_H_ */
