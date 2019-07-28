/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <assert.h>
#include <string.h>
#include "vector.h"

static inline void *__vector_offset(struct vector *self, int size)
{
  assert(self->items != NULL);
  return (void *)((char *)self->items + self->itemsize * size);
}

static int __vector_maybe_expand(struct vector *self, int extrasize)
{
  int total = self->size + extrasize;
  if (total <= self->capacity)
    return 0;

  int capacity;
  if (self->capacity != 0)
    capacity = self->capacity << 2;
  else
    capacity = VECTOR_MINIMUM_CAPACITY;

  void *items = kmalloc(capacity * self->itemsize);
  if (items == NULL)
    return -1;
  if (self->items != NULL) {
    memcpy(items, self->items, self->size * self->itemsize);
    kfree(self->items);
  }
  self->items = items;
  self->capacity = capacity;
  return 0;
}

void vector_free(struct vector *self, vector_freefunc freefunc, void *data)
{
  if (freefunc != NULL) {
    VECTOR_ITERATOR(iter, self);
    void *item;
    iter_for_each(&iter, item)
      freefunc(item, data);
  }
  kfree(self->items);
  self->size = 0;
  self->capacity = 0;
  self->items = NULL;
}

int vector_concat(struct vector *self, struct vector *other)
{
  if (__vector_maybe_expand(self, other->size))
    return -1;

  void *offset = __vector_offset(self, self->size);
  memcpy(offset, other->items, other->size * other->itemsize);
  self->size += other->size;
  return 0;
}

int vector_set(struct vector *self, int index, void *item, void *val)
{
  if (index < 0 || index > self->size)
    return -1;

  vector_get(self, index, val);

  if (__vector_maybe_expand(self, 1))
    return -1;

  void *offset = __vector_offset(self, index);
  memcpy(offset, item, self->itemsize);
  ++self->size;
  return 0;
}

int vector_get(struct vector *self, int index, void *val)
{
  if (index < 0 || index > self->size - 1)
    return -1;
  void *offset = __vector_offset(self, index);
  memcpy(val, offset, self->itemsize);
  return 0;
}

int vector_push_back(struct vector *self, void *item)
{
  if (__vector_maybe_expand(self, 1))
    return -1;

  void *offset = __vector_offset(self, self->size);
  memcpy(offset, item, self->itemsize);
  ++self->size;
  return 0;
}

int vector_pop_back(struct vector *self, void *val)
{
  if (self->size <= 0)
    return -1;

  void *offset = __vector_offset(self, self->size - 1);
  memcpy(val, offset, self->itemsize);
  --self->size;
  return 0;
}

void *vector_toarr(struct vector *self)
{
  int size = self->itemsize * (self->size + 1);
  void *arr = kmalloc(size);
  memcpy(arr, self->items, size);
  return arr;
}

void *vector_iter_next(struct iterator *iter)
{
  struct vector *vec = iter->iterable;
  if (!vec || vec->size <= 0)
    return NULL;

  if (iter->index < vec->size) {
    iter->item = __vector_offset(vec, iter->index);
    ++iter->index;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}

void *vector_iter_prev(struct iterator *iter)
{
  struct vector *vec = iter->iterable;
  if (!vec || vec->size <= 0)
    return NULL;

  if (iter->item == NULL)
    iter->index = vec->size - 1;

  if (iter->index >= 0) {
    iter->item = __vector_offset(vec, iter->index);
    iter->index--;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}
