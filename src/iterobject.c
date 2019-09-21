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

#include "iterobject.h"

void iter_free(Object *self)
{
  if (!iter_check(self)) {
    error("object of '%.64s' is not an Iterator", OB_TYPE_NAME(self));
    return;
  }

  IterObject *iter = (IterObject *)self;
  TYPE_DECREF(iter->desc);
  OB_DECREF(iter->ob);
  OB_DECREF(iter->args);
  OB_DECREF(iter->step);
  kfree(iter);
}

static Object *iter_next(Object *self, Object *args)
{
  if (!iter_check(self)) {
    error("object of '%.64s' is not an Iterator", OB_TYPE_NAME(self));
    return NULL;
  }

  IterObject *iter = (IterObject *)self;
  Object *ob = iter->ob;
  func_t fn = OB_TYPE(ob)->iternext;
  if (fn != NULL)
    return fn(self, NULL);
  else
    return NULL;
}

static MethodDef iter_methods[] = {
  {"__next__", NULL, "<T>", iter_next},
  {NULL},
};

TypeObject iter_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Iterator",
  .free    = iter_free,
  .iternext = iter_next,
  .methods  = iter_methods,
};

void init_iter_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Iterator");
  TypeDesc *para = desc_from_paradef("T", NULL);
  desc_add_paradef(desc, para);
  TYPE_DECREF(para);
  iter_type.desc = desc;
  if (type_ready(&iter_type) < 0)
    panic("Cannot initalize 'Iterator' type.");
}

Object *iter_new(TypeDesc *desc, Object *ob, Object *args, Object *step)
{
  IterObject *iter = kmalloc(sizeof(IterObject));
  init_object_head(iter, &iter_type);
  //iter->desc = TYPE_INCREF(desc);
  iter->ob = OB_INCREF(ob);
  iter->args = OB_INCREF(args);
  iter->step = OB_INCREF(step);
  return (Object *)iter;
}
