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

#include "stringobject.h"
#include "intobject.h"
#include "stringbuf.h"
#include "hashfunc.h"
#include "mem.h"
#include "log.h"

LOGGER(0)

static HashTable strobj_cache;

static uint32 strobj_hash(void *k)
{
  StringObject *sob = k;
  return hash_string(sob->str);
}

static int strobj_equal(void *k1, void *k2)
{
  StringObject *sob1 = k1;
  StringObject *sob2 = k2;
  return !strcmp(sob1->str, sob2->str);
}

/* find string object from cache */
static inline StringObject *__find_strobj(char *str)
{
  StringObject key = {.str = str};
  HashNode *hnode = HashTable_Find(&strobj_cache, &key);
  return hnode ? container_of(hnode, StringObject, hnode) : NULL;
}

/* add string object to cache */
static inline int __add_strobj(StringObject *sob)
{
  return HashTable_Insert(&strobj_cache, &sob->hnode);
}

/* remove string object from cache */
static inline void __remove_strobj(StringObject *sob)
{
  HashTable_Remove(&strobj_cache, &sob->hnode);
}

Object *String_New(char *str)
{
  StringObject *sob = __find_strobj(str);
  if (sob) {
    Log_Debug("find '%s' in string cache", str);
    OB_INCREF(sob);
    return (Object *)sob;
  }

  String s;
  int size;
  if (AtomString_Find(str, &s)) {
    Log_Debug("find '%s' in atom string", str);
    size = sizeof(StringObject);
    sob = Malloc(size);
    sob->str = s.str;
  } else {
    int datasize = strlen(str) + 1;
    size = sizeof(StringObject) + datasize;
    sob = Malloc(size);
    sob->str = sob->data;
    strcpy(sob->str, str);
  }

  Init_Object_Head(sob, &String_Klass);
  sob->len = strlen(str);
  Init_HashNode(&sob->hnode, sob);
  __add_strobj(sob);
  return (Object *)sob;
}

void String_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *sob = (StringObject *)ob;
  __remove_strobj(sob);
  Log_Debug("free string: '%s'", sob->str);
  Mfree(ob);
}

char *String_Raw(Object *ob)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  StringObject *sob = (StringObject *)ob;
  return sob->str;
}

static Object *__string_concat(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  OB_ASSERT_KLASS(args, String_Klass);
  StringObject *sob1 = (StringObject *)ob;
  StringObject *sob2 = (StringObject *)args;
  //FIXME: too larger?
  char buf[sob1->len + sob2->len + 1];
  strcpy(buf, sob1->str);
  strcat(buf, sob2->str);
  return String_New(buf);
}

static Object *__string_length(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, String_Klass);
  assert(!args);
  StringObject *sob = (StringObject *)ob;
  return Integer_New(sob->len);
}

static CFunctionDef string_funcs[] = {
  {"Concat", "s", "s", __string_concat},
  {"Length", "i", NULL, __string_length},
  {NULL}
};

void Init_String_Klass(void)
{
  HashTable_Init(&strobj_cache, strobj_hash, strobj_equal);
  Init_Klass(&String_Klass, NULL);
  Klass_Add_CFunctions(&String_Klass, string_funcs);
}

static void finifunc(HashNode *hnode, void *arg)
{
  StringObject *sob = container_of(hnode, StringObject, hnode);
  OB_DECREF(sob);
}

void Fini_String_Klass(void)
{
  HashTable_Fini(&strobj_cache, finifunc, NULL);
  Fini_Klass(&String_Klass);
}

static Object *string_equal(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, String_Klass);
  OB_ASSERT_KLASS(v2, String_Klass);
  return Bool_New(strobj_equal(v1, v2));
}

static Object *string_hash(Object *v, Object *args)
{
  OB_ASSERT_KLASS(v, String_Klass);
  return Integer_New(strobj_hash(v));
}

/* itself */
static Object *string_tostring(Object *v, Object *args)
{
  OB_ASSERT_KLASS(v, String_Klass);
  OB_INCREF(v);
  return v;
}

static Object *__string_add__(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, String_Klass);
  OB_ASSERT_KLASS(v2, String_Klass);
  StringObject *sob1 = (StringObject *)v1;
  StringObject *sob2 = (StringObject *)v2;

  DeclareStringBuf(buf);
  StringBuf_Format_CStr(buf, "##", sob1->str, sob2->str);
  Object *ob = String_New(buf.data);
  FiniStringBuf(buf);
  return ob;
}

static NumberOperations string_numops = {
  .add = __string_add__,
};

Klass String_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "String",
  .ob_free  = String_Free,
  .ob_hash  = string_hash,
  .ob_cmp   = string_equal,
  .ob_str   = string_tostring,
  .num_ops  = &string_numops,
};
