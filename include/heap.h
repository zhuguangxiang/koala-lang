/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _KOALA_HEAP_H_
#define _KOALA_HEAP_H_

#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
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
 *
 */
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
 *
 * self - The heap to be initialized.
 * cmp  - The function with which to sort items in the heap.
 *        It must return < 0, if a < b, 0 if a == b, and > 0 if a > b.
 *
 * Returns 0 or -1 if memory allocation failed.
 */
int heap_init(struct heap *self, int (*cmp)(void *, void *));

/*
 * Free the binary heap and its memory. The items in the heap are not freed.
 *
 * self - The heap to be freed.
 *
 * Returns nothing.
 */
void heap_free(struct heap *self);

/*
 * Add a new item into the heap. The item will be sorted into correct position
 * according to the heap's 'cmp' function.
 *
 * self - The heap into which to add the item.
 * item - The data item to store.
 *
 * Returns 0 if successful or -1 if failed.
 */
int heap_push(struct heap *self, void *item);

/*
 * Remove the top item from the heap. The remainings are sorted into place with
 * the heap's 'cmp' function.
 *
 * self - The heap from which to remove the top one.
 *
 * Returns the top item  or null if the heap is empty.
 */
void *heap_pop(struct heap *self);

/*
 * Iterator callback function for heap iteration.
 * See iterator.h
 */
int heap_iter_next(struct iterator *iter);

/*
 * Declare an iterator of the heap.
 *
 * name - The name of the heap iterator.
 * heap - The container to iterate.
 */
#define HEAP_ITERATOR(name, heap) \
  ITERATOR(name, heap, heap_iter_next)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HEAP_H_ */
