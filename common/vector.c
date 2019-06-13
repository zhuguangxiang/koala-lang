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

#include <assert.h>
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

  void *items = mem_alloc(capacity * self->itemsize);
  if (items == NULL)
    return -1;
  if (self->items != NULL) {
    memcpy(items, self->items, self->size * self->itemsize);
    mem_free(self->items);
  }
  self->items = items;
  self->capacity = capacity;
  return 0;
}

void vector_free(struct vector *self, vector_free_fn free_fn, void *data)
{
  if (free_fn != NULL) {
    VECTOR_ITERATOR(iter, self);
    void *item;
    iter_for_each(&iter, item)
      free_fn(item, data);
  }
  mem_free(self->items);
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

int vector_iter_next(struct iterator *iter)
{
  struct vector *vec = iter->iterable;
  if (vec->size <= 0)
    return 0;

  iter->current = __vector_offset(vec, iter->index);
  iter->index++;
  return (iter->index > vec->size) ? 0 : 1;
}

int vector_iter_prev(struct iterator *iter)
{
  struct vector *vec = iter->iterable;
  if (vec->size <= 0)
    return 0;

  if (iter->current == NULL)
    iter->index = vec->size - 1;
  iter->current = __vector_offset(vec, iter->index);
  iter->index--;
  return (iter->index < 0) ? 0 : 1;
}
