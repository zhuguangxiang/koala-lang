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

#define DEFAULT_CAPACITY 8

void Vector_Init(Vector *vec)
{
	vec->size = 0;
	vec->capacity = 0;
	vec->items = NULL;
}

Vector *Vector_New(void)
{
	Vector *vec = mm_alloc(sizeof(Vector));
	if (!vec)
		return NULL;

	vec->size = 0;
	vec->capacity = 0;
	vec->items = NULL;
	return vec;
}

void Vector_Free(Vector *vec, vec_finifunc fini, void *arg)
{
	if (!vec)
		return;
	Vector_Fini(vec, fini, arg);
	mm_free(vec);
}

void Vector_Fini(Vector *vec, vec_finifunc fini, void *arg)
{
	if (!vec->items)
		return;

	if (fini) {
		void *item;
		Vector_ForEach(item, vec)
			fini(item, arg);
	}

	mm_free(vec->items);

	vec->size = 0;
	vec->capacity = 0;
	vec->items = NULL;
}

static int vector_expand(Vector *vec, int newsize)
{
	int capacity = vec->capacity;
	if (newsize <= capacity)
		return 0;

	int new_allocated = (capacity == 0) ? DEFAULT_CAPACITY : capacity << 1;
	void *items = mm_alloc(new_allocated * sizeof(void *));
	if (!items)
		return -1;

	if (vec->items) {
		memcpy(items, vec->items, vec->size * sizeof(void *));
		mm_free(vec->items);
	}
	vec->capacity = new_allocated;
	vec->items = items;
	return 0;
}

int Vector_Set(Vector *vec, int index, void *item)
{
	if (index < 0 || index > Vector_Size(vec))
		return -1;

	if (vector_expand(vec, index + 1) < 0)
		return -1;

	vec->items[index] = item;
	if (index >= vec->size)
		vec->size = index + 1;
	return 0;
}

void *Vector_Get(Vector *vec, int index)
{
	if (!vec->items || index < 0 || index >= vec->size)
		return NULL;

	return vec->items[index];
}

int Vector_Concat(Vector *dest, Vector *src)
{
	void *item;
	Vector_ForEach(item, src)
		Vector_Append(dest, item);
	return 0;
}
