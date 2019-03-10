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

#ifndef _KOALA_INTOBJECT_H_
#define _KOALA_INTOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct intobject {
  OBJECT_HEAD
  int64 value;
} IntObject;

extern Klass Int_Klass;
void Init_Integer_Klass(void);
void Fini_Integer_Klass(void);
Object *Integer_New(int64 value);
int64 Integer_Raw(Object *ob);

extern Klass Bool_Klass;
Object *Bool_New(int bval);
int Bool_Raw(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_INTOBJECT_H_ */
