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

#include "atomtable.h"
#include "mem.h"

AtomTable *AtomTable_New(ht_hashfunc hash, ht_equalfunc equal, int size)
{
	int msz = sizeof(AtomTable) + size * sizeof(Vector);
	AtomTable *table = mm_alloc(msz);
	assert(table);
	HashTable_Init(&table->table, hash, equal);
	table->size = size;
	for (int i = 0; i < size; i++)
		Vector_Init(table->items + i);
	return table;
}

typedef struct atomdata {
	datafree fn;
	void *arg;
	int type;
} AtomData;

static void __atomdata_fini_fn(void *data, void *arg)
{
	AtomData *atomdata = arg;
	atomdata->fn(atomdata->type, data, atomdata->arg);
}

static void __atomentry_free_fn(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	AtomEntry *e = container_of(hnode, AtomEntry, hnode);
	mm_free(e);
}

void AtomTable_Free(AtomTable *table, datafree fn, void *arg)
{
	if (!table)
		return;

	AtomData itemdata = {fn, arg, 0};
	for (int i = 0; i < table->size; i++) {
		itemdata.type = i;
		Vector_Fini(table->items + i, __atomdata_fini_fn, &itemdata);
	}

	HashTable_Fini(&table->table, __atomentry_free_fn, NULL);
	mm_free(table);
}

static AtomEntry *atomentry_new(int type, int index, void *data)
{
	AtomEntry *e = mm_alloc(sizeof(AtomEntry));
	Init_HashNode(&e->hnode, e);
	e->type  = type;
	e->index = index;
	e->data  = data;
	return e;
}

int AtomTable_Append(AtomTable *table, int type, void *data, int unique)
{
	Vector *vec = table->items + type;
	Vector_Append(vec, data);
	int index = Vector_Size(vec) - 1;
	assert(index >= 0);
	if (unique) {
		AtomEntry *e = atomentry_new(type, index, data);
		int res = HashTable_Insert(&table->table, &e->hnode);
		assert(!res);
	}
	return index;
}

int AtomTable_Index(AtomTable *table, int type, void *data)
{
	AtomEntry e = {.type = type, .index = 0, .data = data};
	HashNode *hnode = HashTable_Find(&table->table, &e);
	if (!hnode)
		return -1;
	AtomEntry *res = container_of(hnode, AtomEntry, hnode);
	return res->index;
}

void *AtomTable_Get(AtomTable *table, int type, int index)
{
	assert(type >= 0 && type < table->size);
	return Vector_Get(table->items + type, index);
}

int AtomTable_Size(AtomTable *table, int type)
{
	assert(type >= 0 && type < table->size);
	return Vector_Size(table->items + type);
}
