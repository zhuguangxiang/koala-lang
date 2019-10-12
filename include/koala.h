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

#ifndef _KOALA_API_H_
#define _KOALA_API_H_

#include "version.h"
#include "log.h"
#include "memory.h"
#include "atom.h"
#include "eval.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "classobject.h"
#include "intobject.h"
#include "floatobject.h"
#include "stringobject.h"
#include "arrayobject.h"
#include "tupleobject.h"
#include "mapobject.h"
#include "moduleobject.h"
#include "codeobject.h"
#include "enumobject.h"
#include "rangeobject.h"
#include "iterobject.h"
#include "closureobject.h"
#include "fmtmodule.h"
#include "iomodule.h"
#include "osmodule.h"
#include "modules.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void koala_initialize(void);
void koala_finalize(void);
void koala_readline(void);
void koala_compile(char *path);
void koala_execute(char *path);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_API_H_ */
