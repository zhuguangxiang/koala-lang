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

#include "koala.h"

static Object *os_path_get(Object *self, Object *ob)
{
  if (!field_check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!Module_Check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  ModuleObject *mo = (ModuleObject *)ob;
  Object *val = vector_get(&mo->values, field->offset);
  if (val == NULL) {
    TypeDesc *desc = desc_from_str;
    val = array_new(desc);
    TYPE_DECREF(desc);
    vector_set(&mo->values, field->offset, val);
  }
  return OB_INCREF(val);
}

static FieldDef os_fields[] = {
  {"path", "Llang.Array;", os_path_get, NULL},
  {NULL}
};

void init_os_module(void)
{
  Object *m = Module_New("os");
  Module_Add_VarDefs(m, os_fields);
  Module_Install("os", m);
  OB_DECREF(m);
}

void fini_os_module(void)
{
  Module_Uninstall("os");
}
