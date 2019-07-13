/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "object.h"

struct klass class_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Class",
};

struct klass any_type = {
  OBJECT_HEAD_INIT(&lass_type)
  .name = "Any"
};

struct klass nil_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "NilType"
};

struct object nil_obj = {
  OBJECT_HEAD_INIT(&nil_type)
};

struct object *__class_members__(struct object *ob, struct object *args)
{
  return NULL;
}

struct object *__class_name__(struct object *ob, struct object *args)
{
  OB_TYPE_ASSERT(ob, &class_type);
  assert(!args);
  struct klass *klazz = (struct klass *)ob;
  return strobj_new(klazz->name);
}

static struct cfuncdef class_funcs[] = {
  {"__members__", NULL, "Llang.Tuple", __class_members__},
  {"__name__", NULL, NULL, __class_name__},
  {NULL, NULL, NULL, NULL},
};

void init_klass(void)
{

}
