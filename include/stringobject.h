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

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include "common.h"
#include "object.h"
#include "vec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stringobject {
  OBJECT_HEAD
  /* unicode length */
  int len;
  Vec buf;
} StringObject;

typedef struct charobject {
  OBJECT_HEAD
  unsigned int value;
} CharObject;

extern TypeObject string_type;
#define string_check(ob) (OB_TYPE(ob) == &string_type)
void init_string_type(void);
Object *string_new(char *str);
char *string_asstr(Object *self);
Object *string_length(Object *self, Object *args);

extern TypeObject char_type;
#define char_check(ob) (OB_TYPE(ob) == &char_type)
void init_char_type(void);
Object *char_new(unsigned int val);
static inline int char_asch(Object *ob)
{
  if (char_check(ob)) {
    error("object of '%.64s' is not a Char.", OB_TYPE(ob)->name);
    return 0;
  }

  CharObject *ch = (CharObject *)ob;
  return ch->value;
}
Object *string_asbytes(Object *self, Object *args);
Object *string_format(Object *self, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
