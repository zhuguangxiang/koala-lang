/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "vector.h"
#include "mem.h"

#define DEFAULT_CAPACITY 4
typedef void (*visitor)(void *obj, void *arg);

void Vector_Init(Vector *vec, int objsize)
{
  vec->objsize = objsize;
  vec->size = 0;
  vec->capacity = 0;
  vec->items = NULL;
}

void Vector_Fini(Vector *vec, void *vist, void *arg)
{
  if (vec == NULL || vec->items == NULL)
    return;

  if (vist != NULL) {
    visitor fn = (visitor)vist;
    void *obj;
    Vector_ForEach(obj, vec)
      fn(obj, arg);
  }

  Mfree(vec->items);
  vec->capacity = 0;
  vec->size = 0;
  vec->items = NULL;
}

Vector *Vector_Capacity(int capacity, int objsize)
{
  Vector *vec = Malloc(sizeof(Vector));
  if (vec == NULL)
    return NULL;

  vec->objsize = objsize;
  vec->size = 0;
  if (capacity > 0) {
    vec->capacity = capacity;
    vec->items = Malloc(capacity * objsize);
  } else {
    vec->capacity = 0;
    vec->items = NULL;
  }
  return vec;
}

void Vector_Free(Vector *vec, void *vist, void *arg)
{
  if (vec == NULL)
    return;
  Vector_Fini(vec, vist, arg);
  Mfree(vec);
}

static int vector_expand(Vector *vec, int newsize)
{
  int capacity = vec->capacity;
  if (newsize <= capacity)
    return 0;

  int new_allocated = (capacity == 0) ? DEFAULT_CAPACITY : capacity << 1;
  void *items = Malloc(new_allocated * vec->objsize);
  if (items == NULL)
    return -1;

  if (vec->items != NULL) {
    memcpy(items, vec->items, vec->size * vec->objsize);
    Mfree(vec->items);
  }
  vec->capacity = new_allocated;
  vec->items = items;
  return 0;
}

#define VECTOR_OFFSET(vec, i)                   \
  ((char *)(vec)->items + i * (vec)->objsize)

int Vector_Set(Vector *vec, int index, void *item)
{
  if (index < 0 || index > Vector_Size(vec))
    return -1;

  if (vector_expand(vec, index + 1) < 0)
    return -1;

  memcpy(VECTOR_OFFSET(vec, index), item, vec->objsize);

  if (index >= vec->size)
    vec->size = index + 1;
  return 0;
}

void *Vector_Get(Vector *vec, int index)
{
  if (!vec->items || index < 0 || index >= vec->size)
    return NULL;

  return (void *)VECTOR_OFFSET(vec, index);
}

void Vector_Concat(Vector *dest, Vector *src)
{
  if (src == NULL)
    return;

  void *obj;
  Vector_ForEach(obj, src)
    Vector_Append(dest, obj);
}
