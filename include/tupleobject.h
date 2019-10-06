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

#ifndef _KOALA_TUPLE_OBJECT_H_
#define _KOALA_TUPLE_OBJECT_H_

#include "object.h"
#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tupleobject {
  OBJECT_HEAD
  int size;
  Object *items[0];
} TupleObject;

extern TypeObject tuple_type;
#define tuple_check(ob) (OB_TYPE(ob) == &tuple_type)
void init_tuple_type(void);
Object *tuple_new(int size);
Object *tuple_pack(int size, ...);
int tuple_size(Object *self);
Object *tuple_get(Object *self, int index);
int tuple_set(Object *self, int index, Object *val);
Object *tuple_slice(Object *self, int i, int j);
void *tuple_iter_next(struct iterator *iter);
#define TUPLE_ITERATOR(name, tuple) \
  ITERATOR(name, tuple, tuple_iter_next)
#define tuple_for_each(item, tuple) \
  for (int idx = 0; idx < ((TupleObject *)tuple)->size && \
    ({item = tuple_get(tuple, idx); 1;}); ++idx)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TUPLE_OBJECT_H_ */
