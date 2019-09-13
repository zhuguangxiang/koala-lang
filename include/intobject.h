/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_INT_OBJECT_H_
#define _KOALA_INT_OBJECT_H_

#include "object.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct intobject {
  OBJECT_HEAD
  int64_t value;
} IntegerObject;

typedef struct boolobject {
  OBJECT_HEAD
  int value;
} BoolObject;

typedef struct byteobject {
  OBJECT_HEAD
  int value;
} ByteObject;

extern TypeObject integer_type;
#define Integer_Check(ob) (OB_TYPE(ob) == &integer_type)
void init_integer_type(void);
Object *Integer_New(int64_t val);
static inline int64_t Integer_AsInt(Object *ob)
{
  if (!Integer_Check(ob)) {
    error("object of '%.64s' is not a Integer.", OB_TYPE(ob)->name);
    return 0;
  }
  IntegerObject *int_obj = (IntegerObject *)ob;
  return int_obj->value;
}

extern TypeObject byte_type;
#define Byte_Check(ob) (OB_TYPE(ob) == &byte_type)
void init_byte_type(void);
Object *Byte_New(int val);
static inline int Byte_AsInt(Object *ob)
{
  if (!Byte_Check(ob)) {
    error("object of '%.64s' is not a Byte.", OB_TYPE(ob)->name);
    return 0;
  }
  ByteObject *byte_obj = (ByteObject *)ob;
  return byte_obj->value;
}

extern TypeObject bool_type;
#define Bool_Check(ob) (OB_TYPE(ob) == &bool_type)
void init_bool_type(void);
extern BoolObject OB_True;
extern BoolObject OB_False;

static inline Object *Bool_True(void)
{
  Object *res = (Object *)&OB_True;
  return OB_INCREF(res);
}

static inline Object *Bool_False(void)
{
  Object *res = (Object *)&OB_False;
  return OB_INCREF(res);
}

static inline int Bool_IsTrue(Object *ob)
{
  if (!Bool_Check(ob)) {
    error("object of '%.64s' is not a Bool.", OB_TYPE(ob)->name);
    return 0;
  }
  return ob == (Object *)&OB_True ? 1 : 0;
}

static inline int Bool_IsFalse(Object *ob)
{
  if (!Bool_Check(ob)) {
    error("object of '%.64s' is not a Bool.", OB_TYPE(ob)->name);
    return 0;
  }
  return ob == (Object *)&OB_False ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INT_OBJECT_H_ */
