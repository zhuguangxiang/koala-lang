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

#include "valistobject.h"
#include "intobject.h"

Object *valist_new(int size, TypeDesc *desc)
{
  int vsize = sizeof(VaListObject) + size * sizeof(Object *);
  VaListObject *valist = kmalloc(vsize);
  init_object_head(valist, &valist_type);
  valist->size = size;
  valist->desc = TYPE_INCREF(desc);
  return (Object *)valist;
}

void valist_free(Object *self)
{
  if (!valist_check(self)) {
    error("object of '%.64s' is not a VaList", OB_TYPE_NAME(self));
    return;
  }

  VaListObject *valist = (VaListObject *)self;
  TYPE_DECREF(valist->desc);
  for (int i = 0; i < valist->size; ++i) {
    OB_DECREF(valist->items[i]);
  }
  kfree(valist);
}

static Object *valist_getitem(Object *self, Object *args)
{
  int index = integer_asint(args);
  return valist_get(self, index);
}

static Object *valist_size(Object *self, Object *args)
{
  return integer_new(valist_len(self));
}

static MethodDef valist_methods[] = {
  {"__getitem__", "i",    "<T>",  valist_getitem},
  {"len", NULL, "i", valist_size},
  {NULL}
};

TypeObject valist_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "VaList",
  .free    = valist_free,
  .methods = valist_methods,
};

int valist_set(Object *self, int index, Object *v)
{
  if (!valist_check(self)) {
    error("object of '%.64s' is not a VaList", OB_TYPE_NAME(self));
    return -1;
  }

  VaListObject *valist = (VaListObject *)self;
  if (index < 0 || index > valist->size) {
    error("index %d out of range(0...%d)", index, valist->size);
    return -1;
  }

  Object *oldval = valist->items[index];
  OB_DECREF(oldval);
  valist->items[index] = OB_INCREF(v);
  return 0;
}

Object *valist_get(Object *self, int index)
{
  if (!valist_check(self)) {
    error("object of '%.64s' is not a VaList", OB_TYPE_NAME(self));
    return NULL;
  }

  VaListObject *valist = (VaListObject *)self;
  if (index < 0 || index >= valist->size) {
    error("index %d out of range(0..<%d)", index, valist->size);
    return NULL;
  }
  Object *val = valist->items[index];
  return OB_INCREF(val);
}

void init_valist_type(void)
{
  valist_type.desc = desc_from_valist;
  type_add_tp(&valist_type, "T", NULL);
  if (type_ready(&valist_type) < 0)
    panic("Cannot initalize 'VaList' type.");
}

int valist_len(Object *self)
{
  if (!valist_check(self)) {
    error("object of '%.64s' is not a VaList", OB_TYPE_NAME(self));
    return -1;
  }

  VaListObject *valist = (VaListObject *)self;
  return valist->size;
}
