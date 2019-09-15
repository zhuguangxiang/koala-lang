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

#ifndef _KOALA_FIELD_OBJECT_H_
#define _KOALA_FIELD_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fieldobject {
  OBJECT_HEAD
  /* field name */
  char *name;
  /* field owner */
  Object *owner;
  /* field type descriptor */
  TypeDesc *desc;
  /* getter & setter */
  func_t get;
  setfunc set;
  /* offset of value */
  int offset;
  /* enum value */
  int enumvalue;
} FieldObject;

extern TypeObject field_type;
#define field_check(ob) (OB_TYPE(ob) == &field_type)
void init_field_type(void);
Object *field_new(char *name, TypeDesc *desc);
static inline void Field_SetFunc(Object *self, setfunc set, func_t get)
{
  if (!field_check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return;
  }

  FieldObject *field = (FieldObject *)self;
  field->set = set;
  field->get = get;
}

Object *field_default_getter(Object *self, Object *ob);
int field_default_setter(Object *self, Object *ob, Object *val);

Object *field_get(Object *self, Object *ob);
int field_set(Object *self, Object *ob, Object *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */
