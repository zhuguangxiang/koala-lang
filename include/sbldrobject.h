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

#ifndef _KOALA_SBLDR_OBJECT_H_
#define _KOALA_SBLDR_OBJECT_H_

#include "object.h"
#include "slice.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sbldrobject {
  OBJECT_HEAD
  Object *buf;
} SBldrObject;

extern TypeObject sbldr_type;
#define sbldr_check(ob) (OB_TYPE(ob) == &sbldr_type)
void init_sbldr_type(void);
Slice *sbldr_slice(Object *self);
int sbldr_len(Object *self);
char *sbldr_ptr(Object *self);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SBLDR_OBJECT_H_ */
