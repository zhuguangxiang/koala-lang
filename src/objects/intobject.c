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

#include "intobject.h"
#include "stringobject.h"
#include "hashfunc.h"
#include "mem.h"
#include "cache.h"

static Cache Int_Cache;

Object *Integer_New(int64 value)
{
  IntObject *iob = Cache_Take(&Int_Cache);
  Init_Object_Head(iob, &Int_Klass);
  iob->value = value;
  return (Object *)iob;
}

void Integer_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Int_Klass);
  Cache_Restore(&Int_Cache, ob);
}

int64 Integer_Raw(Object *ob)
{
  OB_ASSERT_KLASS(ob, Int_Klass);
  IntObject *iob = (IntObject *)ob;
  return iob->value;
}

static Object *integer_equal(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, Int_Klass);
  OB_ASSERT_KLASS(v2, Int_Klass);
  IntObject *iob1 = (IntObject *)v1;
  IntObject *iob2 = (IntObject *)v2;
  return Bool_New(iob1->value == iob2->value);
}

static Object *integer_hash(Object *v, Object *args)
{
  OB_ASSERT_KLASS(v, Int_Klass);
  IntObject *iob = (IntObject *)v;
  return Integer_New(hash_uint32((uint32)iob->value, 32));
}

static Object *integer_tostring(Object *v, Object *args)
{
  OB_ASSERT_KLASS(v, Int_Klass);
  IntObject *iob = (IntObject *)v;
  char buf[128];
  snprintf(buf, 127, "%lld", iob->value);
  return String_New(buf);
}

static Object *__integer_add__(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, Int_Klass);
  OB_ASSERT_KLASS(v2, Int_Klass);
  IntObject *iob1 = (IntObject *)v1;
  IntObject *iob2 = (IntObject *)v2;
  return Integer_New(iob1->value + iob2->value);
}

static NumberOperations integer_numops = {
  .add = __integer_add__,
};

Klass Int_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Integer",
  .ob_free  = Integer_Free,
  .ob_hash  = integer_hash,
  .ob_cmp   = integer_equal,
  .ob_str   = integer_tostring,
  .num_ops  = &integer_numops,
};

void Init_Integer_Klass(void)
{
  Init_Cache(&Int_Cache, "IntObject", sizeof(IntObject));
  Init_Klass(&Int_Klass, NULL);
  Init_Klass(&Bool_Klass, NULL);
}

void Fini_Integer_Klass(void)
{
  assert(OB_REFCNT(&Int_Klass) == 1);
  assert(OB_REFCNT(&Bool_Klass) == 1);
  Fini_Cache(&Int_Cache, NULL, NULL);
  Fini_Klass(&Int_Klass);
  Fini_Klass(&Bool_Klass);
}

static Object *bool_tostring(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Bool_Klass);
}

static Object *__bool_and__(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, Bool_Klass);
  OB_ASSERT_KLASS(v2, Bool_Klass);
  IntObject *iob1 = (IntObject *)v1;
  IntObject *iob2 = (IntObject *)v2;
  return Bool_New(iob1->value && iob2->value);
}

static Object *__bool_or__(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, Bool_Klass);
  OB_ASSERT_KLASS(v2, Bool_Klass);
  IntObject *iob1 = (IntObject *)v1;
  IntObject *iob2 = (IntObject *)v2;
  return Bool_New(iob1->value || iob2->value);
}

static Object *__bool_not__(Object *v1, Object *v2)
{
  OB_ASSERT_KLASS(v1, Bool_Klass);
  assert(v2 == NULL);
  IntObject *iob1 = (IntObject *)v1;
  return Bool_New(!iob1->value);
}

static NumberOperations bool_numops = {
  .land = __bool_and__,
  .lor  = __bool_or__,
  .lnot = __bool_not__,
};

Klass Bool_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Bool",
  .ob_str  = bool_tostring,
  .num_ops = &bool_numops,
};

static IntObject btrue = {
  OBJECT_HEAD_INIT(&Bool_Klass)
  .value = 1,
};

static IntObject bfalse = {
  OBJECT_HEAD_INIT(&Bool_Klass)
  .value = 0,
};

Object *Bool_New(int bval)
{
  return bval ? (Object *)&btrue : (Object *)&bfalse;
}

int Bool_Raw(Object *ob)
{
  OB_ASSERT_KLASS(ob, Bool_Klass);
  IntObject *iob = (IntObject *)ob;
  return (int)iob->value;
}
