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

#include "vec.h"

#define VEC_MINIMUM_CAPACITY 8
#define VEC_DOUBLE_GROWTH_CAPACITY 1024
#define VEC_GROWTH_FACTOR  (5 / 4)

int vec_init_capacity(Vec *self, int objsize, int capacity)
{
  if (self == NULL) return -1;
  self->length = 0;
  self->capacity = MAX(VEC_MINIMUM_CAPACITY, capacity);
  self->objsize = objsize;
  self->elems = kmalloc(self->capacity * objsize);
  return (self->elems != NULL) ? 0 : -1;
}

/* Destroy a vector */
void vec_fini(Vec *self)
{
  if (self == NULL) return;
  vec_clear(self);
  self->capacity = 0;
  self->objsize = 0;
  kfree(self->elems);
}

/* Remove all elements from the vector */
int vec_clear(Vec *self)
{
  if (self == NULL) return -1;
  self->length = 0;
  return 0;
}

static int __vec_maybe_expand(Vec *self, int extrasize)
{
  int total = self->length + extrasize;
  if (total <= self->capacity) return 0;

  int capacity = self->capacity;
  while (capacity < total) {
    if (capacity < VEC_DOUBLE_GROWTH_CAPACITY) {
      capacity = capacity << 1;
    } else {
      capacity = capacity * VEC_GROWTH_FACTOR;
    }
  }

  void *new_elems = kmalloc(capacity * self->objsize);
  if (new_elems == NULL) return -1;

  if (self->elems != NULL) {
    memcpy(new_elems, self->elems, self->length * self->objsize);
    kfree(self->elems);
  }
  self->elems = new_elems;
  self->capacity = capacity;
  return 0;
}

/*
 * Concatenate a vector's items into the end of another vector.
 * self is changed, other is unchanged.
 */
int vec_join(Vec *self, Vec *other)
{
  if (self == NULL || other == NULL) return -1;

  if (self->objsize != other->objsize) return -1;

  if (__vec_maybe_expand(self, other->length)) return -1;

  void *offset = vec_offset(self, self->length);
  memcpy(offset, other->elems, other->length * other->objsize);
  self->length += other->length;
  return 0;
}

/* Store the pointer's content at an index. Index bound is checked. */
int vec_set(Vec *self, int index, void *obj)
{
  if (self == NULL) return -1;

  if (index < 0 || index > self->length) return -1;

  if (__vec_maybe_expand(self, 1)) return -1;

  void *offset = vec_offset(self, index);
  memcpy(offset, obj, self->objsize);
  if (index == self->length) {
    ++self->length;
  }
  return 0;
}

/* Get the data's pointer at an index. Index bound is checked. */
int vec_get(Vec *self, int index, void *obj)
{
  if (self == NULL) return -1;

  if (index < 0 || index > self->length - 1) return -1;

  void *offset = vec_offset(self, index);
  memcpy(obj, offset, self->objsize);
  return 0;
}

static void __vec_move_right(Vec *self, int index)
{
  /* the location to start to move */
  char *offset = vec_offset(self, index);

  /* how many bytes to be moved to the right */
  int nbytes = (self->length - index) * self->objsize;

  /* NOTES: do not use memcpy */
  memmove(offset + self->objsize, offset, nbytes);
}

/*
 * Insert the pointer's content into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vec_insert(Vec *self, int index, void *obj)
{
  if (self == NULL) return -1;

  if (index < 0 || index > self->length) return -1;

  if (__vec_maybe_expand(self, 1)) return -1;

  /* move other items to the right */
  __vec_move_right(self, index);

  /* insert the object */
  void *offset = vec_offset(self, index);
  memcpy(offset, obj, self->objsize);
  ++self->length;
  return 0;
}

static void __vec_move_left(Vec *self, int index)
{
  /* the location to start to move */
  void *offset = vec_offset(self, index);

  /* how many bytes to be moved to the left */
  int nbytes = (self->length - index - 1) * self->objsize;

  /* NOTES: do not use memcpy */
  memmove(offset, offset + self->objsize, nbytes);
}

/*
 * Remove the element at the index and shrink the vector by one.
 * This is relatively expensive operation.
 */
int vec_remove(Vec *self, int index, void *obj)
{
  if (self == NULL) return -1;

  if (index < 0 || index >= self->length) return -1;

  /* the location to start to move */
  void *offset = vec_offset(self, index);

  /* save old obj to 'obj' */
  if (obj != NULL) memcpy(obj, offset, self->objsize);

  /* remove the index object */
  __vec_move_left(self, index);
  --self->length;
  return 0;
}

/* Swap the two elements */
void vec_swap(Vec *self, int idx1, int idx2)
{
  if (idx1 < 0 || idx1 >= self->length) return;
  if (idx2 < 0 || idx2 >= self->length) return;
  void *offset1 = vec_offset(self, idx1);
  void *offset2 = vec_offset(self, idx2);
  char tmp[self->objsize];
  memcpy(tmp, offset1, self->objsize);
  memcpy(offset1, offset2, self->objsize);
  memcpy(offset2, tmp, self->objsize);
}

/* Reverse order of the vector */
void vec_reverse(Vec *self, int index, int len)
{
  int i = index;
  int j = index + len - 1;
  while (i < j) {
    vec_swap(self, i, j);
    ++i; --j;
  }
}

/* Add an array of elements at the end of the vector */
int vec_push_arr(Vec *self, void *arr, int len)
{
  if (self == NULL) return -1;

  if (__vec_maybe_expand(self, len)) return -1;

  /* insert objects */
  void *offset = vec_offset(self, self->length);
  memcpy(offset, arr, self->objsize * len);
  self->length += len;
  return 0;
}

/* Copy # of elements into new vector */
Vec vec_copy(Vec *self, int len)
{
  Vec buf;
  len = MIN(self->length, len);
  vec_init_capacity(&buf, self->objsize, len);
  buf.length = len;
  memcpy(buf.elems, self->elems, self->objsize * len);
  return buf;
}

/* Find element */
int vec_find(Vec *self, void *obj, int (*cmp)(const void *, const void *))
{
  void *p;
  int i;
  vec_foreach(p, i, self) {
    if (!cmp(p, obj))
      return i;
  }
  return -1;
}

/* Find element in reverse order */
int vec_find_rev(Vec *self, void *obj, int (*cmp)(const void *, const void *))
{
  void *p;
  int i;
  vec_foreach_rev(p, i, self) {
    if (!cmp(p, obj))
      return i;
  }
  return -1;
}
