/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "memory.h"
#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_MINIMUM_CAPACITY 8

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

/* Declare an empty vector with name. */
#define VECTOR(name, itemsize) \
  struct vector name = {0, 0, itemsize, NULL}

/* Declare an empty vector stored pointers. */
#define VECTOR_PTR(name) VECTOR(name, sizeof(void *))

/* Initialize a vector with item size. */
static inline void vector_init(struct vector *self, int itemsize)
{
  memset(self, 0, sizeof(struct vector));
  self->itemsize = itemsize;
}

/* Free item callback function of vector_free */
typedef void (*vector_freefunc)(void *, void *);

/* Destroy a vector with item free function. 'freefn' is optional. */
void vector_free(struct vector *self, vector_freefunc freefunc, void *data);

/*
 * Remove all items from the vector, leaving the container with a size of 0.
 * The vector does not resize after removing the items. The memory is still
 * allocated for future uses.
 */
static inline void vector_clear(struct vector *self)
{
  memset(self->items, 0, self->capacity * self->itemsize);
  self->size = 0;
}

/*
 * Concatenate a vector's items into the end of another vector.
 * The source vector is unchanged.
 */
int vector_concat(struct vector *self, struct vector *other);

/*
 * Create a new vector from a range of an existing vector.
 * The source vector is unchanged.
 */
struct vector *vector_slice(struct vector *self, int start, int size);

/*
 * Copy the vector's items to a new vector.
 * The source vector is unchanged.
 */
#define vector_clone(self) \
  vector_slice(self, 0, (self)->size)

/* Shrink the vector's memory to it's size for saving memory. */
int vector_shrink_to_fit(struct vector *self);

/* Get the vector size. */
#define vector_size(self) \
  (self)->size

/* Get the vector capacity(allocated spaces). */
#define vector_capacity(self) \
  (self)->capacity

/* Store an item at an index. The previous item is returned va 'prev'. */
int vector_set(struct vector *self, int index, void *item, void *prev);

/* Get the item stored at an index. Index bound is checked. */
int vector_get(struct vector *self, int index, void *val);

/*
 * Insert an item into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_insert(struct vector *self, int index, void *item);

/*
 * Remove the item at the index and shrink the vector by one. The removed item
 * is stored va 'prev'. This is relatively expensive operation.
 */
int vector_remove(struct vector *self, int index, void *prev);

/* Add an item at the end of the vector. */
int vector_push_back(struct vector *self, void *item);

/*
 * Remove an item at the end of the vector.
 * When used with 'push_back', the vector can be used as a stack.
 */
int vector_pop_back(struct vector *self, void *val);

/* Get an item at the end of the vector, but not remove it. */
static inline int vector_top_back(struct vector *self, void *val)
{
  return vector_get(self, self->size - 1, val);
}

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

/* Convert vector to array with null-terminated item. */
void *vector_toarr(struct vector *self);

/*
 * Iterator callback function for vector iteration.
 * See iterator.h.
 */
void *vector_iter_next(struct iterator *iter);

/* Declare an iterator of the vector. Deletion is not safe. */
#define VECTOR_ITERATOR(name, vector) \
  ITERATOR(name, vector, vector_iter_next)

/*
 * Reverse iterator callback function for vector iteration.
 * See iterator.h.
 */
void *vector_iter_prev(struct iterator *iter);

/* Declare an reverse iterator of the vector. Deletion is not safe. */
#define VECTOR_REVERSE_ITERATOR(name, vector) \
  ITERATOR(name, vector, vector_iter_prev)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
