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

#include "log.h"
#include "mem.h"
#include "tupleobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "buffer.h"

Object *Tuple_New(int size)
{
  int sz = sizeof(TupleObject) + size * sizeof(Object *);
  TupleObject *tuple = gc_alloc(sz);
  assert(tuple);
  Init_Object_Head(tuple, &Tuple_Klass);
  tuple->size = size;
  return (Object *)tuple;
}

void Tuple_Free(Object *ob)
{
  if (!ob) return;
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  /* FIMXE */
  gc_free(ob);
}

Object *Tuple_Get(Object *ob, int index)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;

  if (index < 0 || index >= tuple->size) {
    Log_Error("index %d out of bound", index);
    return NULL;
  }

  return tuple->items[index];
}

Object *Tuple_Get_Slice(Object *ob, int min, int max)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  if (min > max) return NULL;

  Object *tuple = Tuple_New(max - min + 1);
  int index = min;
  int i = 0;
  while (index <= max) {
    Tuple_Set(tuple, i, Tuple_Get(ob, index));
    i++;
    index++;
  }
  return tuple;
}

int Tuple_Size(Object *ob)
{
  if (!ob) return 0;
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;
  return tuple->size;
}

int Tuple_Set(Object *ob, int index, Object *val)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TupleObject *tuple = (TupleObject *)ob;

  if (index < 0 || index >= tuple->size) {
    Log_Error("index %d out of bound", index);
    return -1;
  }

  tuple->items[index] = val;
  return 0;
}

static Object *__tuple_get(Object *ob, Object *args)
{
  return Tuple_Get(ob, Integer_ToCInt(args));
}

static Object *__tuple_size(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  assert(!args);
  TupleObject *tuple = (TupleObject *)ob;
  return Integer_New(tuple->size);
}

static FuncDef tuple_funcs[] = {
  {"Get", "A", "i", __tuple_get},
  {"Size", "i", NULL, __tuple_size},
  {NULL}
};

void Init_Tuple_Klass(void)
{
  Klass_Add_CFunctions(&Tuple_Klass, tuple_funcs);
}

void Fini_Tuple_Klass(void)
{
  Fini_Klass(&Tuple_Klass);
}

static void tuple_free(Object *ob)
{
  Tuple_Free(ob);
}

static Object *tuple_tostring(Object *ob)
{
  TupleObject *tuple = (TupleObject *)ob;
  Buffer buf;
  Buffer_Init(&buf, 64);
  char ch = '[';
  Buffer_Write_Byte(&buf, ch);
  Object *s;
  Object *o;
  int count = 0;
  for (int i = 0; i < tuple->size; i++) {
    o = tuple->items[i];
    if (o) {
      if (count++ > 0)
        Buffer_Write(&buf, ", ", 2);
      if (Buffer_Size(&buf) > 512) {
        Buffer_Write(&buf, "...", 3);
        break;
      }
      if (OB_KLASS(o) == &String_Klass) {
        ch = '\'';
        Buffer_Write_Byte(&buf, ch);
        Buffer_Write(&buf, String_RawString(o), String_Length(o));
        Buffer_Write_Byte(&buf, ch);
      } else {
        s = OB_KLASS(o)->ob_str(o);
        Buffer_Write(&buf, String_RawString(s), String_Length(s));
        OB_DECREF(s);
      }
    }
  }
  ch = ']';
  Buffer_Write_Byte(&buf, ch);
  char *data = Buffer_RawData(&buf);
  Buffer_Fini(&buf);
  s = String_New(data);
  mm_free(data);
  return s;
}

Klass Tuple_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Tuple",
  .basesize = sizeof(TupleObject),
  .flags = OB_FLAGS_GC,
  .ob_free = tuple_free,
  .ob_str = tuple_tostring,
};
