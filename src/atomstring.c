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
#include "stringex.h"
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
} StringEntry;

/* string pool */
typedef struct {
  HashTable table;
} StringPool;

static StringPool pool;

static StringEntry *strentry_new(char *str)
{
  int len = strlen(str);
  StringEntry *e = Malloc(sizeof(StringEntry) + len + 1);
  Init_HashNode(&e->hnode, e);
  e->val.str = e->data;
  e->size = len + 1;
  memcpy(e->data, str, len);
  return e;
}

int AtomString_Find(char *str, String *s)
{
  StringEntry key = {.val = str};
  HashNode *hnode = HashTable_Find(&pool.table, &key);
  if (!hnode)
    return 0;
  StringEntry *e = container_of(hnode, StringEntry, hnode);
  *s = e->val;
  return 1;
}

String AtomString_New_NString(char *str, int len)
{
  String s;
  char *tmp = string_ndup(str, len);
  s = AtomString_New(tmp);
  Mfree(tmp);
  return s;
}

String AtomString_New(char *str)
{
  String s;
  if (AtomString_Find(str, &s))
    return s;

  StringEntry *e = strentry_new(str);
  HashTable_Insert(&pool.table, &e->hnode);
  return e->val;
}

String AtomString_New_NStr(char *str, int len)
{
  char *zstr = string_ndup(str, len);
  String s = AtomString_New(zstr);
  Mfree(zstr);
  return s;
}

int AtomString_Length(String s)
{
  if (s.str == NULL)
    return 0;
  return strlen(s.str);
}

uint32 AtomString_Hash(String s)
{
  if (s.str == NULL)
    return 0;
  return hash_string(s.str);
}

int AtomString_Equal(String s1, String s2)
{
  if (s1.str == NULL || s2.str == NULL)
    return 0;
  if (s1.str == s2.str)
    return 1;
  return !strcmp(s1.str, s2.str);
}

static uint32 __string_hash(void *key)
{
  StringEntry *e = key;
  return hash_string(e->val.str);
}

static int __string_equal(void *k1, void *k2)
{
  StringEntry *e1 = k1;
  StringEntry *e2 = k2;
  return !strcmp(e1->val.str, e2->val.str);
}

void AtomString_Init(void)
{
  HashTable_Init(&pool.table, __string_hash, __string_equal);
}

static void strentry_free(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  StringEntry *e = container_of(hnode, StringEntry, hnode);
  Mfree(e);
}

void AtomString_Fini(void)
{
  HashTable_Fini(&pool.table, strentry_free, NULL);
}
