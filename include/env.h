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

#ifndef _KOALA_ENV_H_
#define _KOALA_ENV_H_

#include "properties.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KOALA_PATH "koala.path"
#define KOALA_INIT "__init__"
#define KOALA_MAIN "main"
#define KOALA_TASK_STACKSIZE  (512 * 1024)

extern Properties properties;
#define Init_Env() Properties_Init(&properties)
#define Fini_Env() Properties_Fini(&properties)
#define Env_Set(name, value) Properties_Put(&properties, name, value)
#define Env_Set_Vec(name, vec)  \
({                              \
  char *value;                  \
  Vector_ForEach(value, vec)    \
    Env_Set(name, value);       \
})
#define Env_Get(name) Properties_GetOne(&properties, name)
#define Env_Get_Vec(name) Properties_Get(&properties, name)

#define KOALA_PKG_LANG "lang"
#define KOALA_PKG_FMT  "fmt"
#define KOALA_PKG_IO   "io"

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ENV_H_ */
