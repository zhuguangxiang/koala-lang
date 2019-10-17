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

#ifndef _KOALA_ENUM_OBJECT_H_
#define _KOALA_ENUM_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct labelobject {
  OBJECT_HEAD
  char *name;
  Vector *types;
} LabelObject;
extern TypeObject label_type;
#define label_check(ob) (OB_TYPE(ob) == &label_type)
void init_label_type(void);

typedef struct enumobject {
  OBJECT_HEAD
  char *name;
  Object *values;
} EnumObject;

TypeObject *enum_type_new(char *path, char *name);
void type_add_label(TypeObject *type, char *name, Vector *types);
Object *enum_new(Object *ob, char *name, Object *values);
int enum_check_byname(Object *ob, Object *name);
int enum_check_value(Object *ob, Object *idx, Object *val);
Object *enum_get_value(Object *ob, Object *idx);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ENUM_OBJECT_H_ */
