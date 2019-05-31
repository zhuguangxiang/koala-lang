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

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * dynamic array
 */
typedef struct vector {
  /* item size, default is sizeof(void *) */
  int itemsize;
  /* slots used */
  int size;
  /* total available slots */
  int capacity;
  /* array of pointer */
  void **items;
} Vector;

/* declare a vector with name */
#define VECTOR(name) Vector name = {sizeof(void *), 0, 0, NULL}

/* initialize a vector */
void Vector_Init(Vector *vec);

/* new a vector, and use Vector_Free to free the vector */
Vector *Vector_Capacity_Object(int capacity, int itemsize);
#define Vector_Capacity(capacity) \
  Vector_Capacity_Object(capacity, sizeof(void *))
#define Vector_New() Vector_Capacity(0)
#define Vector_New_Object(itemsize) \
  Vector_Capacity_Object(0, itemsize)

/* call the function before free the vector */
typedef void (*vec_finifunc)(void *item, void *arg);

/* free the vector, if it is created by Vector_New */
void Vector_Free(Vector *vec, vec_finifunc fini, void *arg);

/* free the vector self only, not care its elements */
#define Vector_Free_Self(vec) Vector_Free(vec, NULL, NULL)

/* fini the vector, if it is declared by VECTOR(name) */
void Vector_Fini(Vector *vec, vec_finifunc fini, void *arg);

/* fini the vector self only */
#define Vector_Fini_Self(vec) Vector_Fini(vec, NULL, NULL)

/*
 * set value into 'index' position
 * notes that index >=0 and index <= it's size
 * if index is vector's size, the same as Vector_Append
 */
int Vector_Set(Vector *vec, int index, void *item);

/*
 * get 'index' position's value
 * if index is out bound, it returns NULL
 */
void *Vector_Get(Vector *vec, int index);

/* get last one item */
#define Vector_Get_Last(vec) Vector_Get(vec, Vector_Size(vec) - 1)

/* append a value into the vector's end position */
#define Vector_Append(vec, item) Vector_Set(vec, (vec)->size, item)

/*
 * like string's strcat
 * src canbe null
 * notes that the src vector is not cleared.
 */
void Vector_Concat(Vector *dest, Vector *src);

/* returns the vector's size */
#define Vector_Size(vec) (NULL != (vec) ? (vec)->size : 0)

#define VECTOR_OFFSET(vec, i) \
  ((char *)(vec)->items + i * (vec)->itemsize)

#define VECTOR_INDEX(vec, i) \
({ \
  ((vec)->itemsize <= sizeof(void *)) ? \
  *(void **)VECTOR_OFFSET(vec, i) : (void *)VECTOR_OFFSET(vec, i); \
})


/* foreach for vector */
#define Vector_ForEach(item, vec)         \
for (int i = 0;                           \
     i < Vector_Size(vec) &&              \
     ({item = VECTOR_INDEX(vec, i); 1;}); \
     i++)

/* foreach revsersely for vector */
#define Vector_ForEach_Reverse(item, vec) \
for (int i = Vector_Size(vec) - 1;        \
     (i >= 0) &&                          \
     ({item = VECTOR_INDEX(vec, i); 1;}); \
     i--)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
