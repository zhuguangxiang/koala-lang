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

#ifndef _KOALA_ARRAY_OBJECT_H_
#define _KOALA_ARRAY_OBJECT_H_

#include "object.h"
#include "slice.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arrayobject {
  OBJECT_HEAD
  TypeDesc *desc;
  Slice buf;
} ArrayObject;

extern TypeObject array_type;
#define array_check(ob) (OB_TYPE(ob) == &array_type)
void init_array_type(void);
Object *array_new(TypeDesc *desc);
Object *array_with_buf(TypeDesc *desc, Slice buf);
Object *array_push_back(Object *self, Object *val);
int array_set(Object *self, int index, Object *v);
int array_len(Object *self);
void *array_ptr(Object *self);
Slice *array_slice(Object *self);
Object *byte_array_new(void);
Object *byte_array_with_buf(Slice buf);
Object *byte_array_no_buf(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAY_OBJECT_H_ */
