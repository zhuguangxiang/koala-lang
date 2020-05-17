/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_SLICE_H_
#define _KOALA_SLICE_H_

#include <stdlib.h>
#include "log.h"
#include "memory.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct slice {
  // Pointer to inner slice
  void *ptr;
  // Starting position of elements
  int offset;
  // Used length in number of elements
  int length;
} Slice;

/* Initialize a slice with element size and capacity. */
int slice_init_capacity(Slice *self, int objsize, int capacity);

/* reserve # slots of this slice. */
int slice_reserve(Slice *self, int length);

/* Initialize a slice with element size, by default capacity. */
static inline int slice_init(Slice *self, int objsize)
{
  return slice_init_capacity(self, objsize, 0);
}

/* Get slice's slice */
int slice_slice(Slice *self, Slice *src, int offset, int len);

/* Get slice's slice from offset to end */
int slice_slice_to_end(Slice *self, Slice *src, int offset);

/* Copy slice's only, make sure 'dst' has enough space. */
int slice_copy(Slice *dst, Slice *src, int count);

/* Destroy a slice */
void slice_fini(Slice *self);

/* Remove all elements from the slice */
int slice_clear(Slice *self);

/*
 * Concatenate a slice's items into the end of another slice.
 * dst is changed, src is unchanged.
 */
int slice_extend(Slice *dst, Slice *src);

/* Add an array of elements at the end of the slice */
int slice_push_array(Slice *self, void *arr, int len);

/* Get the slice length. */
#define slice_len(self) ((self) != NULL ? (self)->length : 0)

/* Get the slice curent capacity. */
int slice_capacity(Slice *self);

/* Check the slice is empty or not */
#define slice_empty(self) (slice_len(self) == 0)

/* Get the slice's stored memory */
void *slice_ptr(Slice *self, int index);

/* Store the pointer's content at an index. Index bound is checked. */
int slice_set(Slice *self, int index, void *obj);

/* Get the data's pointer at an index. Index bound is checked. */
int slice_get(Slice *self, int index, void *obj);

/*
 * Insert the pointer's content into the in-bound of the slice.
 * This is relatively expensive operation.
 */
int slice_insert(Slice *self, int index, void *obj);

/*
 * Remove the element at the index and shrink the slice by one.
 * This is relatively expensive operation.
 */
int slice_remove(Slice *self, int index, void *obj);

/* Swap the two elements */
int slice_swap(Slice *self, int idx1, int idx2);

/* Reverse order of the slice */
void slice_reverse(Slice *self);

/* Find element */
int slice_index(Slice *self, void *obj, __compar_fn_t cmp);

/* Find element in reverse order */
int slice_last_index(Slice *self, void *obj, __compar_fn_t cmp);

/* Sort a slice in-place. */
void slice_sort(Slice *self, __compar_fn_t cmp);

/* Add an element at the end of the slice */
#define slice_push_back(self, obj) \
  slice_set(self, slice_len(self), obj)

/* Add an element at the front of the slice */
#define slice_push_front(self, obj) \
  slice_insert(self, 0, obj)

/*
 * Remove an element at the end of the slice.
 * When used with 'push_back', the slice can be used as a stack.
 */
#define slice_pop_back(self, obj) \
  slice_remove(self, slice_len(self) - 1, (void *)obj)

/* Remove an element at the front of the slice. */
#define slice_pop_front(self, obj) \
  slice_remove(self, 0, (void *)obj)

/* Get the last element in the slice, but not remove it. */
#define slice_top_back(self, obj) \
  slice_get(self, vec_len(self) - 1, obj)

/* Get the front element in the slice, but not remove it. */
#define slice_top_front(self, obj) \
  slice_get(self, 0, obj)

/* Slice foreach */
#define slice_foreach(ptr, i, slice) \
  if ((slice)->length > 0) \
    for ((i) = 0; \
         (i) < (slice)->length && (((ptr) = slice_ptr(slice, i)), 1); \
         ++(i))

/* Slice foreach reverse */
#define slice_foreach_rev(ptr, i, slice) \
  if ((slice)->length > 0) \
    for ((i) = (slice)->length - 1; \
         (i) >= 0 && (((ptr) = slice_ptr(slice, i)), 1); \
         --(i))

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SLICE_H_ */
