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

#include "atomstring.h"
#include "hashtable.h"
#include "hashfunc.h"
#include "mem.h"

typedef struct {
  /* hash node */
  HashNode hnode;
  /* val.str = data */
  String val;
  /* strlen + 1 */
  int size;
  /* string's characters */
  char data[0];
} StrEntry;

/* string pool */
typedef struct {
  HashTable table;
} StrPool;

static StrPool pool;

static StrEntry *strentry_new(char *str)
{
  int len = strlen(str);
  StrEntry *e = mm_alloc(sizeof(StrEntry) + len + 1);
  Init_HashNode(&e->hnode, e);
  e->val.str = e->data;
  e->size = len + 1;
  memcpy(e->data, str, len);
  return e;
}

static int string_exist(char *str, String *s)
{
  StrEntry key = {.val.str = str};
  HashNode *hnode = HashTable_Find(&pool.table, &key);
  if (!hnode)
    return 0;
  StrEntry *e = container_of(hnode, StrEntry, hnode);
  *s = e->val;
  return 1;
}

String AtomString_New(char *str)
{
  String s;
  if (string_exist(str, &s))
    return s;

  StrEntry *e = strentry_new(str);
  HashTable_Insert(&pool.table, &e->hnode);
  return e->val;
}

int AtomString_Length(String str)
{
  return strlen(str.str);
}

int AtomString_Equal(String s1, String s2)
{
  return s1.str == s2.str;
}

static uint32 __string_hash(void *key)
{
  StrEntry *e = key;
  return hash_string(e->val.str);
}

static int __string_equal(void *k1, void *k2)
{
  StrEntry *e1 = k1;
  StrEntry *e2 = k2;
  return !strcmp(e1->val.str, e2->val.str);
}

void AtomString_Init(void)
{
  HashTable_Init(&pool.table, __string_hash, __string_equal);
}

static void strentry_free(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  StrEntry *e = container_of(hnode, StrEntry, hnode);
  mm_free(e);
}

void AtomString_Fini(void)
{
  HashTable_Fini(&pool.table, strentry_free, NULL);
}
