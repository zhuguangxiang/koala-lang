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

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "memory.h"
#include "iterator.h"
#include "common.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a dynamic array */
typedef struct vector {
  /* used items */
  int size;
  /* total items */
  int capacity;
  /* item size */
  int isize;
  /* array of items */
  char *items;
} Vector;

/* Initialize a vector with item size. */
int vector_init(Vector *self, int capacity, int isize);

/* Destroy a vector */
void vector_fini(Vector *self);

/* Create a new vector */
Vector *vector_new(int capacity, int isize);

/* Destroy a vector, and free vector memory */
void vector_free(Vector *self);

/* Remove all items from the vector, and restore it as initial state. */
int vector_clear(Vector *self);

/*
 * Concatenate a vector's items into the end of another vector.
 * The source vector is unchanged.
 */
int vector_concat(Vector *self, Vector *other);

/*
 * Create a new vector from a range of an existing vector.
 * The source vector is unchanged.
 */
Vector *vector_slice(Vector *self, int start, int size);

/*
 * Copy the vector's items to a new vector.
 * The source vector is unchanged.
 */
#define vector_copy(self) vector_slice(self, 0, (self)->size)

/* Get the vector size. */
#define vector_size(self) ((self) != NULL ? (self)->size : 0)

/* Check the vector is empty or not */
#define vector_empty(self) (vector_size(self) == 0)

/* Get the vector's raw items */
#define vector_toarr(self) ((self) != NULL ? (void *)(self)->items : NULL)

/* Store the pointer's content at an index */
int vector_set(Vector *self, int index, void *item);

/* Get the data's pointer at an index. Index bound is checked. */
void *__vector_get(Vector *self, int index);

/* Return the index item as 'type'. Index bound is checked. */
#define vector_get(self, index, type) *(type *)__vector_get(self, index)

/*
 * Insert the pointer's content into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_insert(Vector *self, int index, void *item);

/*
 * Remove the item at the index and shrink the vector by one.
 * This is relatively expensive operation.
 */
int vector_remove(Vector *self, int index, void *prev);

/* Add an item at the end of the vector and return 'slot' index */
#define vector_push_back(self, item) ({                 \
  int ret = vector_set(self, vector_size(self), item);  \
  (ret != 0) ? vector_size(self) - 1 : -1;              \
})

/* Add an item at the front of the vector */
#define vector_push_front(self, item) vector_insert(self, 0, item)

/*
 * Remove an item at the end of the vector.
 * When used with 'push_back', the vector can be used as a stack.
 */
#define vector_pop_back(self, prev) \
  vector_remove(self, vector_size(self) - 1, (void *)prev)

/* Remove an item at the front of the vector. */
#define vector_pop_front(self, prev) vector_remove(self, 0, (void *)prev)

/* Get the last item in the vector as 'type', but not remove it. */
#define vector_top_back(self, type) \
  vector_get(self, vector_size(self) - 1, type)

/* Get the front item in the vector as 'type', but not remove it. */
#define vector_top_front(self, type) vector_get(self, 0, type)

/*
 * Sort a vector in-place.
 *
 * self - the vector to be sorted.
 * cmp  - The function used to compare two items.
 *        Returns -1, 0, or 1 if the item is less than, equal to,
 *        or greater than the other one.
 *
 * Returns nothing.
 *
 * Examples:
 *   int str_cmp(const void *v1, const void *v2) {
 *     const char *ch1 = *(const char **)v1;
 *     const char *ch2 = *(const char **)v2;
 *     return strcmp(ch1, ch2);
 *   }
 *   vector_sort(vec, str_cmp);
 */
#define vector_sort(self, cmp) \
  qsort((self)->items, (self)->size, (self)->isize, cmp)

/* Vector foreach */
#define vector_foreach(item, vector)                        \
  for (int idx = 0; idx < vector_size(vector) &&            \
      ({item = vector_get(vector, idx, typeof(item)); 1;}); \
      ++idx)

/* Vector foreach reversely */
#define vector_foreach_reverse(item, vector)                \
  for (int idx = vector_size(vector) - 1; idx >= 0 &&       \
      ({item = vector_get(vector, idx, typeof(item)); 1;}); \
      --idx)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
