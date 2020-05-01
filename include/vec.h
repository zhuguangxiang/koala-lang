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

#ifndef _KOALA_VEC_H_
#define _KOALA_VEC_H_

#include <stdlib.h>
#include "log.h"
#include "memory.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vec {
  // Pointer to the stored memory
  char *elems;
  // Size of each element in bytes
  int objsize;
  // Used length in number of elements
  int length;
  // Capacity in number of elements
  int capacity;
} Vec;

/* Initialize a vector with element size and capacity. */
int vec_init_capacity(Vec *self, int objsize, int capacity);

/* Initialize a vector with element size, by default capacity. */
static inline int vec_init(Vec *self, int objsize)
{
  vec_init_capacity(self, objsize, 0);
}

/* Destroy a vector */
void vec_fini(Vec *self);

/* Remove all elements from the vector */
int vec_clear(Vec *self);

/*
 * Concatenate a vector's items into the end of another vector.
 * self is changed, other is unchanged.
 */
int vec_join(Vec *self, Vec *other);

/* Get the vector length. */
#define vec_len(self) ((self) != NULL ? (self)->length : 0)

/* Check the vector is empty or not */
#define vec_empty(self) (vec_len(self) == 0)

/* Get the vector's stored memory */
#define vec_toarr(self) ((self) != NULL ? (void *)(self)->elems : NULL)

/* Store the pointer's content at an index. Index bound is checked. */
int vec_set(Vec *self, int index, void *obj);

/* Get the data's pointer at an index. Index bound is checked. */
int vec_get(Vec *self, int index, void *obj);

/*
 * Insert the pointer's content into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vec_insert(Vec *self, int index, void *obj);

/*
 * Remove the element at the index and shrink the vector by one.
 * This is relatively expensive operation.
 */
int vec_remove(Vec *self, int index, void *obj);

/* Swap the two elements */
void vec_swap(Vec *self, int idx1, int idx2);

/* Reverse order of the vector */
void vec_reverse(Vec *self, int index, int len);

/* Copy # of elements into new vector */
Vec vec_copy(Vec *self, int len);

/* Find element */
int vec_find(Vec *self, void *obj, int (*cmp)(const void *, const void *));

/* Find element in reverse order */
int vec_find_rev(Vec *self, void *obj, int (*cmp)(const void *, const void *));

/* Add an element at the end of the vector */
#define vec_push_back(self, obj) \
  vec_set(self, vec_len(self), obj)

/* Add an element at the front of the vector */
#define vec_push_front(self, obj) \
  vec_insert(self, 0, obj)

/*
 * Remove an element at the end of the vector.
 * When used with 'push_back', the vector can be used as a stack.
 */
#define vec_pop_back(self, obj) \
  vec_remove(self, vec_len(self) - 1, (void *)obj)

/* Remove an element at the front of the vector. */
#define vec_pop_front(self, obj) \
  vec_remove(self, 0, (void *)obj)

/* Get the last element in the vector, but not remove it. */
#define vec_top_back(self, obj) \
  vec_get(self, vec_len(self) - 1, obj)

/* Get the front element in the vector, but not remove it. */
#define vec_top_front(self, obj) \
  vec_get(self, 0, obj)

/* Add an array of elements at the end of the vector */
int vec_push_arr(Vec *self, void *arr, int len);

/* Sort a vector in-place. */
#define vec_sort(self, cmp) \
  qsort((self)->elems, (self)->length, (self)->objsize, cmp)

/* Get pointer of element in Vector */
static inline void *vec_offset(Vec *self, int index)
{
  return self->elems + self->objsize * index;
}

/* Vector foreach */
#define vec_foreach(ptr, i, vec) \
  if ((vec)->length > 0) \
    for ((i) = 0; \
         (i) < (vec)->length && (((ptr) = vec_offset(vec, i)), 1); \
         ++(i))

/* Vector foreach reverse */
#define vec_foreach_rev(ptr, i, vec) \
  if ((vec)->length > 0) \
    for ((i) = (vec)->length - 1; \
         (i) >= 0 && (((ptr) = vec_offset(vec, i)), 1); \
         --(i))

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VEC_H_ */
