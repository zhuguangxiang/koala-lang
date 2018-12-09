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

#ifndef _KOALA_ATOMTABLE_H_
#define _KOALA_ATOMTABLE_H_

#include "hashtable.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct atomentry {
  /* hash node for data is unique */
  HashNode hnode;
  /* index of AtomTable->item[type] */
  int type;
  /* index in 'type' vectors */
  int index;
  /* item data */
  void *data;
} AtomEntry;

typedef struct atomtable {
  /* hash table for data is unique */
  HashTable table;
  /* items' array size */
  int size;
  /* vector array */
  Vector items[0];
} AtomTable;

typedef void (*atomvisitfunc)(int type, void *data, void *arg);
/* new an atom table */
AtomTable *AtomTable_New(int size, hashfunc hash, equalfunc equal);
/* free an atom table */
void AtomTable_Free(AtomTable *table, atomvisitfunc fn, void *arg);
/* add an item to the tail of the 'type' vector */
int AtomTable_Append(AtomTable *table, int type, void *data, int unique);
/* get unique data's index in 'type' vector, only for unique data */
int AtomTable_Index(AtomTable *table, int type, void *data);
/* get index data in the 'type' vector */
static inline void *AtomTable_Get(AtomTable *table, int type, int index)
{
  assert(type >= 0 && type < table->size);
  return Vector_Get(table->items + type, index);
}
/* get the 'type' vector size */
static inline int AtomTable_Size(AtomTable *table, int type)
{
  assert(type >= 0 && type < table->size);
  return Vector_Size(table->items + type);
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ATOMTABLE_H_ */
