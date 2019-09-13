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

#ifndef _KOALA_FMT_MODULE_H_
#define _KOALA_FMT_MODULE_H_

#include "object.h"
#include "strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fmtterobject {
  OBJECT_HEAD
  StrBuf buf;
} FmtterObject;

extern TypeObject fmtter_type;
#define Fmtter_Check(ob) (OB_TYPE(ob) == &fmtter_type)
void init_fmtter_type(void);
void init_fmt_module(void);
void fini_fmt_moudle(void);
Object *Fmtter_New(void);
void Fmtter_Free(Object *ob);
Object *Fmtter_WriteFormat(Object *self, Object *args);
Object *Fmtter_WriteString(Object *self, Object *args);
Object *Fmtter_WriteInteger(Object *self, Object *args);
Object *Fmtter_WriteTuple(Object *self, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FMT_MODULE_H_ */
