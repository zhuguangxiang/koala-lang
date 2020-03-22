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

#include <string.h>
#include <inttypes.h>
#include "vector.h"
#include "log.h"

static inline void *__vector_offset(Vector *self, int size)
{
  expect(self->items != NULL);
  return (void *)((char *)self->items + sizeof(void *) * size);
}

static int __vector_maybe_expand(Vector *self, int extrasize)
{
  int total = self->size + extrasize;
  if (total <= self->capacity)
    return 0;

  int capacity;
  if (self->capacity != 0)
    capacity = self->capacity << 1;
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

void vector_init_capacity(Vector *self, int capacity)
{
  self->capacity = capacity;
  self->size = capacity;
  self->items = kmalloc(capacity * sizeof(void *));
}

void vector_fini(Vector *self)
{
  if (self == NULL)
    return;
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

int vector_append_int(Vector *self, int val)
{
  vector_push_back(self, (void *)(intptr_t)val);
  return self->size - 1;
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
