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

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "memory.h"
#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_MINIMUM_CAPACITY 2

/* a dynamic array */
struct vector {
  /* total slots */
  int capacity;
  /* used slots count */
  int size;
  /* item size */
  int itemsize;
  /* array of items */
  void *items;
};

/*
 * Declare a empty vector with name.
 *
 * name - The name of the vector.
 */
#define VECTOR(name, itemsize) \
  struct vector name = {0, 0, itemsize, NULL}

/*
 * Initialize a vector.
 *
 * self     - The vector to be initialized.
 * itemsize - The size of each item stored in the vector.
 *
 * Returns nothing.
 */
static inline
void vector_init(struct vector *self, int itemsize)
{
  memset(self, 0, sizeof(struct vector));
  self->itemsize = itemsize;
}

/*
 * Finalize a vector with item free function. 'free' is optional.
 *
 * self     - The Vector to finalize.
 * itemfree - The function to call for every item.
 *
 * Returns nothing.
 */
void vector_fini(struct vector *self, void (*itemfree)(void *));

/*
 * Create a new vector. Memory allocation does not happen.
 * It is delayed to insert first item.
 *
 * itemsize - The size of each item.
 *
 * Returns a new vector or null if memory allocation failed.
 */
static inline
struct vector *vector_create(int itemsize)
{
  struct vector *vec = mem_alloc(sizeof(struct vector));
  if (vec == NULL)
    return NULL;
  vec->itemsize = itemsize;
  return vec;
}

/*
 * Destroy a vector with item free function. 'free' is optional.
 *
 * self     - The Vector to destroy.
 * itemfree - The function to call for every item.
 *
 * Returns nothing.
 */
static inline
void vector_destroy(struct vector *self, void (*itemfree)(void *))
{
  vector_fini(self, itemfree);
  mem_free(self);
}

/*
 * Remove all items from the vector, leaving the container with a size of 0.
 * The vector does not resize after removing the items. The memory is still
 * allocated for future uses.
 *
 * self - The vector to clear.
 *
 * Returns nothing.
 */
static inline
void vector_clear(struct vector *self)
{
  memset(self->items, 0, self->capacity * self->itemsize);
  self->size = 0;
}

/*
 * Concatenate a vector's items into the end of another vector.
 * The source vector is unchanged.
 *
 * self  - The destination vector.
 * other - The source vector.
 */
int vector_concat(struct vector *self, struct vector *other);

/*
 * Create a new vector from a range of an existing vector.
 * The source vector is unchanged.
 *
 * self  - The vector from which to slice items.
 * start - The index to begin the range.
 * size  - The number of items to copy.
 *
 * Returns the vector's subset(slice) or null if memory allocation failed.
 */
struct vector *vector_slice(struct vector *self, int start, int size);

/*
 * Copy the vector's items to a new vector.
 * The source vector is unchanged.
 *
 * self - The vector to copy.
 *
 * Returns a new vector or null if memory allocation failed.
 */
#define vector_clone(self) \
  vector_slice(self, 0, (self)->size)

/*
 * Shrink the vector's memory to it's size for saving memory.
 *
 * self - The vector to shrink.
 *
 * Returns 0 or -1 if memory allocation failed.
 */
int vector_shrink_to_fit(struct vector *self);

/*
 * Get the vector size.
 *
 * self - The vector to get its size.
 *
 * Returns vector size.
 */
#define vector_size(self) \
  (self)->size

/*
 * Get the vector capacity(allocated spaces).
 *
 * self - The vector to get its capacity.
 *
 * Returns vector capacity.
 */
#define vector_capacity(self) \
  (self)->capacity

/*
 * Store an item at an index. The previous item is returned va 'prev'.
 *
 * self  - The vector to hold the item.
 * index - The position at which to store new item.
 * item  - The data to store in the vector.
 * val   - The previous data returned, optional.
 *
 * Returns -1: failed, 0 successful.
 */
int vector_set(struct vector *self, int index, void *item, void *val);

/*
 * Get the item stored at an index. Index bound is checked.
 *
 * self  - The vector from which to get the item.
 * index - The zero-based item index.
 * val   - The data to store.
 *
 * Returns 0: successful, -1: failed if the index is out of bounds.
 */
int vector_get(struct vector *self, int index, void *val);

/*
 * Insert an item into the in-bound of the vector.
 * This is relatively expensive operation.
 *
 * self  - The Vector to store the item.
 * index - The position at which to insert the item.
 * item  - The data to store in the vector.
 *
 * Returns -1 if memory allocation failed.
 */
int vector_insert(struct vector *self, int index, void *item);

/*
 * Remove the item at the index and shrink the vector by one.
 * This is relatively expensive operation.
 *
 * self  - The vector to remove an item at the index.
 * index - The position to remove item from the vector.
 * val   - The data to store.
 *
 * Returns 0: successful, -1: failed if the index is out of bounds.
 */
int vector_remove(struct vector *self, int index, void *val);

/*
 * Add an item at the end of the vector.
 *
 * self - The vector to store the new item.
 * item - The data to be appended to the vector.
 *
 * Returns -1 if memory allocation failed.
 */
int vector_push_back(struct vector *self, void *item);

/*
 * Remove an item at the end of the vector.
 * When used with 'push_back', the vector can be used as a stack.
 *
 * self - The vector to pop.
 * val   - The data to store.
 *
 * Returns 0: successful, -1: failed if the vector is empty.
 */
int vector_pop_back(struct vector *self, void *val);

/*
 * Sort a vector in-place.
 *
 * self
 * compare - The function used to compare two items.
 *           Returns -1, 0, or 1 if the item is less than, equal to,
 *           or greater than the other one.
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
#define vector_sort(self, compare) \
  qsort((self)->items, (self)->size, compare)

/*
 * Iterator callback function.
 * See iterator.h.
 */
int vector_iter_next(struct iterator *iter);

/*
 * Declare an iterator of the vector. Deletion is safe.
 *
 * name   - The name of the vector iterator
 * vector - The container to iterate.
 */
#define VECTOR_ITERATOR(name, vector) \
  ITERATOR(name, vector, vector_iter_next)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
