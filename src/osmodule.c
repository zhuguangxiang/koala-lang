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

#include <dlfcn.h>
#include "koala.h"

static Object *os_path_get(Object *self, Object *ob)
{
  if (!field_check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!module_check(ob)) {
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
  {"path", "[s", os_path_get, NULL},
  {NULL},
};

static Object *os_load_library(Object *self, Object *ob)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  char *str = string_asstr(ob);
  debug("load library '%s'", str);

  void *dlptr = dlopen(str, RTLD_LAZY);
  if (dlptr == NULL) {
    error("load library '%s' failed", str);
    return NULL;
  }

  void (*init)(void *) = dlsym(dlptr, "init_module");
  if (init == NULL) {
    error("library '%s' is has not 'void init_module(void *)'", str);
    dlclose(dlptr);
    return NULL;
  }

  Object *m = module_new("testmodule");
  ((ModuleObject *)m)->dlptr = dlptr;
  init(m);
  module_install("testmodule", m);
  OB_DECREF(m);
  //dlclose(dlptr);
  return NULL;
}

static MethodDef os_funcs[] = {
  {"load_library", "s", NULL, os_load_library},
  {NULL},
};

void init_os_module(void)
{
  Object *m = module_new("os");
  module_add_vardefs(m, os_fields);
  module_add_funcdefs(m, os_funcs);
  module_install("os", m);
  OB_DECREF(m);
}

void fini_os_module(void)
{
  module_uninstall("os");
}
