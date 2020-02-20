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

#define VECTOR_MINIMUM_CAPACITY 8
#define VECTOR_DOUBLE_GROWTH_CAPACITY 1024
#define VECTOR_GROWTH_FACTOR  (5 / 4)

int vector_init(Vector *self, int capacity, int isize)
{
  if (self == NULL) return -1;
  self->size = 0;
  self->capacity = MAX(VECTOR_MINIMUM_CAPACITY, capacity);
  self->isize = isize;
  self->items = kmalloc(self->capacity * isize);
  return (self->items != NULL) ? 0 : -1;
}

void vector_fini(Vector *self)
{
  if (self == NULL) return;
  kfree(self->items);
  self->size = 0;
  self->capacity = 0;
  self->isize = 0;
}

Vector *vector_new(int capacity, int isize)
{
  Vector *self = kmalloc(sizeof(Vector));
  int ret = vector_init(self, capacity, isize);
  if (ret) {
    kfree(self);
    return NULL;
  }
  return self;
}

void vector_free(Vector *self)
{
  vector_fini(self);
  kfree(self);
}

int vector_clear(Vector *self)
{
  if (self == NULL)
    return -1;

  self->size = 0;
  if (self->capacity > VECTOR_MINIMUM_CAPACITY)
    self->capacity = VECTOR_MINIMUM_CAPACITY;
  kfree(self->items);
  self->items = kmalloc(self->capacity * self->isize);
  if (self->items == NULL)
    return -1;
  return 0;
}

static inline char *__vector_offset(Vector *self, int index)
{
  return self->items + self->isize * index;
}

static int __vector_maybe_expand(Vector *self, int extrasize)
{
  int total = self->size + extrasize;
  if (total <= self->capacity)
    return 0;

  int capacity = self->capacity;
  if (capacity == 0)
    capacity = VECTOR_MINIMUM_CAPACITY;

  while (capacity < total) {
    if (capacity < VECTOR_DOUBLE_GROWTH_CAPACITY) {
      capacity = capacity << 1;
    } else {
      capacity = capacity * VECTOR_GROWTH_FACTOR;
    }
  }

  void *items = kmalloc(capacity * self->isize);
  if (items == NULL)
    return -1;

  if (self->items != NULL) {
    memcpy(items, self->items, self->size * self->isize);
    kfree(self->items);
  }
  self->items = items;
  self->capacity = capacity;
  return 0;
}

static void __vector_move_right(Vector *self, int index)
{
  /* the location to start to move */
  char *offset = __vector_offset(self, index);

  /* how many bytes to be moved to the right */
  size_t nbytes = (self->size - index) * self->isize;

  /* NOTES: do not use memcpy */
  memmove(offset + self->isize, offset, nbytes);
}

static void __vector_move_left(Vector *self, int index)
{
  /* the location to start to move */
  void *offset = __vector_offset(self, index);

  /* how many bytes to be moved to the left */
  size_t nbytes = (self->size - index - 1) * self->isize;

  /* NOTES: do not use memcpy */
  memmove(offset, offset + self->isize, nbytes);
}

int vector_concat(Vector *self, Vector *other)
{
  if (self == NULL || other == NULL)
    return -1;

  if (self->isize != other->isize)
    return -1;

  if (__vector_maybe_expand(self, other->size))
    return -1;

  void *offset = __vector_offset(self, self->size);
  memcpy(offset, other->items, other->size * other->isize);
  self->size += other->size;
  return 0;
}

Vector *vector_slice(Vector *self, int start, int size)
{
  if (self == NULL)
    return NULL;

  if (start < 0 || start >= self->size)
    return NULL;
  if (start + size > self->size)
    return NULL;

  Vector *vec = vector_new(size, self->isize);
  if (vec == NULL)
    return NULL;
  void *offset = __vector_offset(self, start);
  memcpy(vec->items, offset, size * self->isize);
  vec->size = size;
  return vec;
}

int vector_set(Vector *self, int index, void *item)
{
  if (self == NULL)
    return -1;

  if (index < 0 || index > self->size)
    return -1;

  if (__vector_maybe_expand(self, 1))
    return -1;

  void *offset = __vector_offset(self, index);
  memcpy(offset, item, self->isize);
  if (index == self->size) {
    ++self->size;
  }
  return 0;
}

void *__vector_get(Vector *self, int index)
{
  if (self == NULL)
    return NULL;

  if (index < 0 || index > self->size - 1)
    return NULL;

  return (void *)__vector_offset(self, index);
}

int vector_insert(Vector *self, int index, void *item)
{
  if (self == NULL)
    return -1;

  if (index < 0 || index > self->size)
    return -1;

  if (__vector_maybe_expand(self, 1))
    return -1;

  /* move other items to the right */
  __vector_move_right(self, index);

  /* insert the item */
  void *offset = __vector_offset(self, index);
  memcpy(offset, item, self->isize);
  ++self->size;
  return 0;
}

int vector_remove(Vector *self, int index, void *prev)
{
  if (self == NULL)
    return -1;

  if (index < 0 || index >= self->size)
    return -1;

    /* the location to start to move */
  void *offset = __vector_offset(self, index);

  /* save old value to 'prev' */
  if (prev != NULL) memcpy(prev, offset, self->isize);

  /* remove the index item */
  __vector_move_left(self, index);
  --self->size;
  return 0;
}
