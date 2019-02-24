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

#ifndef _KOALA_PROPERTIES_H_
#define _KOALA_PROPERTIES_H_

#include "hashtable.h"
#include "atomstring.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* properties structure */
typedef struct properties {
  HashTable table;
} Properties;

int Properties_Init(Properties *prop);
void Properties_Fini(Properties *prop);
int Properties_Put(Properties *prop, char *key, char *val);
char *Properties_GetOne(Properties *prop, char *key);
Vector *Properties_Get(Properties *prop, char *key);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PROPERTIES_H_ */
