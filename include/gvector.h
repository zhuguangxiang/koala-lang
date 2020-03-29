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

#ifndef _KOALA_GVECTOR_H_
#define _KOALA_GVECTOR_H_

#include "memory.h"
#include "iterator.h"
#include "common.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a dynamic array */
typedef struct gvector {
  /* used items */
  int size;
  /* total items */
  int capacity;
  /* item size */
  int isize;
  /* array of items */
  char *items;
} GVector;

/* Initialize a vector with item size. */
int gvector_init(GVector *self, int capacity, int isize);

/* Destroy a vector */
void gvector_fini(GVector *self);

/* Create a new vector */
GVector *gvector_new(int capacity, int isize);

/* Destroy a vector, and free vector memory */
void gvector_free(GVector *self);

/* Remove all items from the vector, and restore it as initial state. */
int gvector_clear(GVector *self);

/*
 * Concatenate a vector's items into the end of another vector.
 * The source vector is unchanged.
 */
int gvector_concat(GVector *self, GVector *other);

/*
 * Create a new vector from a range of an existing vector.
 * The source vector is unchanged.
 */
GVector *gvector_split(GVector *self, int start, int size);

/*
 * Copy the vector's items to a new vector.
 * The source vector is unchanged.
 */
#define gvector_copy(self) gvector_split(self, 0, (self)->size)

/* Get the vector size. */
#define gvector_size(self) ((self) != NULL ? (self)->size : 0)

/* Check the vector is empty or not */
#define gvector_empty(self) (gvector_size(self) == 0)

/* Get the vector's raw items */
#define gvector_toarr(self) ((self) != NULL ? (void *)(self)->items : NULL)

/* Store the pointer's content at an index */
int gvector_set(GVector *self, int index, void *item);

/* Get the data's pointer at an index. Index bound is checked. */
int __gvector_get(GVector *self, int index, void *item);

/* Return the index item as 'type'. Index bound is checked. */
#define gvector_get(self, index, item) \
  __gvector_get(self, index, (void *)item)

/*
 * Insert the pointer's content into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int gvector_insert(GVector *self, int index, void *item);

/*
 * Remove the item at the index and shrink the vector by one.
 * This is relatively expensive operation.
 */
int gvector_remove(GVector *self, int index, void *prev);

/* Add an item at the end of the vector and return 'slot' index */
#define gvector_push_back(self, item) ({                    \
  int _ret_ = gvector_set(self, gvector_size(self), item);  \
  (_ret_ != 0) ? gvector_size(self) - 1 : -1;               \
})

/* Add an item at the front of the vector */
#define gvector_push_front(self, item) gvector_insert(self, 0, item)

/*
 * Remove an item at the end of the vector.
 * When used with 'push_back', the vector can be used as a stack.
 */
#define gvector_pop_back(self, prev) \
  gvector_remove(self, gvector_size(self) - 1, (void *)prev)

/* Remove an item at the front of the vector. */
#define gvector_pop_front(self, prev) gvector_remove(self, 0, (void *)prev)

/* Get the last item in the vector as 'type', but not remove it. */
#define gvector_top_back(self, item) \
  gvector_get(self, gvector_size(self) - 1, item)

/* Get the front item in the vector as 'type', but not remove it. */
#define gvector_top_front(self, item) gvector_get(self, 0, item)

/* Add # items at the end of the vector */
int gvector_append_array(GVector *self, void *item, int size);

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
#define gvector_sort(self, cmp) \
  qsort((self)->items, (self)->size, (self)->isize, cmp)

/* Vector foreach */
#define gvector_foreach(item, vec) \
  for (int idx = 0; !gvector_get(vec, idx, &(item)); ++idx)

/* Vector foreach reversely */
#define gvector_foreach_reverse(item, vec) \
  for (int idx = gvector_size(vec) - 1; !gvector_get(vec, idx, &(item)); --idx)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GVECTOR_H_ */
