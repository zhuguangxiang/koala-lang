/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <string.h>
#include "vector.h"
#include "log.h"

static inline void *__vector_offset(Vector *self, int size)
{
  if (!self->items)
    panic("null pointer");
  return (void *)((char *)self->items + sizeof(void *) * size);
}

static int __vector_maybe_expand(Vector *self, int extrasize)
{
  int total = self->size + extrasize;
  if (total <= self->capacity)
    return 0;

  int capacity;
  if (self->capacity != 0)
    capacity = self->capacity << 2;
  else
    capacity = VECTOR_MINIMUM_CAPACITY;

  void *items = kmalloc(capacity * sizeof(void *));
  if (items == NULL)
    return -1;

  if (self->items != NULL) {
    memcpy(items, self->items, self->size * sizeof(void *));
    kfree(self->items);
  }
  self->items = items;
  self->capacity = capacity;
  return 0;
}

void vector_fini(Vector *self, freefunc freefunc, void *data)
{
  if (self == NULL)
    return;
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

int vector_concat(Vector *self, Vector *other)
{
  if (__vector_maybe_expand(self, other->size))
    return -1;

  void *offset = __vector_offset(self, self->size);
  memcpy(offset, other->items, other->size * sizeof(void *));
  self->size += other->size;
  return 0;
}

void *vector_set(Vector *self, int index, void *item)
{
  if (index < 0 || index > self->size)
    return NULL;

  if (__vector_maybe_expand(self, 1))
    return NULL;

  void *offset = __vector_offset(self, index);
  void *val = *(void **)offset;
  *(void **)offset = item;
  if (index == self->size) {
    ++self->size;
    val = NULL;
  }
  return val;
}

void *vector_get(Vector *self, int index)
{
  if (self == NULL)
    return NULL;
  if (index < 0 || index > self->size - 1)
    return NULL;
  void *offset = __vector_offset(self, index);
  return *(void **)offset;
}

int vector_push_back(Vector *self, void *item)
{
  if (__vector_maybe_expand(self, 1))
    return -1;

  void *offset = __vector_offset(self, self->size);
  *(void **)offset = item;
  ++self->size;
  return 0;
}

void *vector_pop_back(Vector *self)
{
  if (self == NULL || self->size <= 0)
    return NULL;

  void *offset = __vector_offset(self, self->size - 1);
  void *val = *(void **)offset;
  --self->size;
  return val;
}

void *vector_toarr(Vector *self)
{
  int size = sizeof(void *) * (self->size + 1);
  void *arr = kmalloc(size);
  memcpy(arr, self->items, size);
  return arr;
}

void *vector_iter_next(Iterator *iter)
{
  Vector *vec = iter->iterable;
  if (!vec || vec->size <= 0)
    return NULL;

  if (iter->index < vec->size) {
    void *offset = __vector_offset(vec, iter->index);
    iter->item = *(void **)offset;
    ++iter->index;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}

void *vector_iter_prev(Iterator *iter)
{
  Vector *vec = iter->iterable;
  if (!vec || vec->size <= 0)
    return NULL;

  if (iter->item == NULL)
    iter->index = vec->size - 1;

  if (iter->index >= 0) {
    void *offset = __vector_offset(vec, iter->index);
    iter->item = *(void **)offset;
    iter->index--;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}
