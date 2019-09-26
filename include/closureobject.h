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

#ifndef _KOALA_CLOSURE_OBJECT_H_
#define _KOALA_CLOSURE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upval {
  char *name;
  Object **ref;
  Object *value;
} UpVal;

typedef struct closureobject {
  OBJECT_HEAD
  Vector *upvals;
  Object *code;
} ClosureObject;

extern TypeObject closure_type;
#define closure_check(ob) (OB_TYPE(ob) == &closure_type)
void init_closure_type(void);
Object *closure_new(Object *code, Vector *upvals);
UpVal *upval_new(Object **ref);
#define closure_getcode(ob) (((ClosureObject *)ob)->code)
Object *closure_load(Object *ob, int index);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CLOSURE_OBJECT_H_ */
