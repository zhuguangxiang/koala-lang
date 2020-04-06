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
#include <unistd.h>
#include "koala.h"

static Object *get_path(Object *self, Object *ob)
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
  expect(val != NULL);
  return OB_INCREF(val);
}

static int set_path(Object *self, Object *ob, Object *val)
{
  if (!field_check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return -1;
  }

  if (!module_check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return -1;
  }

  if (!array_check(val)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(val));
    return -1;
  }

  FieldObject *field = (FieldObject *)self;
  ModuleObject *mo = (ModuleObject *)ob;
  ArrayObject *arr = (ArrayObject *)val;

  if (!desc_isstr(arr->desc)) {
    error("object of '%.64s' is not an Array<String>", OB_TYPE_NAME(val));
    return -1;
  }

  Object *old = vector_set(&mo->values, field->offset, OB_INCREF(val));
  OB_DECREF(old);
  return 0;
}

static Object *default_path(void)
{
  TypeDesc *desc = desc_from_str;
  Object *val = array_new(desc, NULL);
  TYPE_DECREF(desc);
  return val;
}

static Object *default_stdout(void)
{
  return file_new(stdout, "stdout");
}

static Object *default_pkgpath(void)
{
  printf("default_pkgpath\n");
#define PATH_MAX  1024
  char exepath[PATH_MAX + 1] = {0};
  readlink("/proc/self/exe", exepath, PATH_MAX);
  printf("%s\n", exepath);
  char *path = strstr(exepath, "bin/koala");
  expect(path != NULL);
  STRBUF(sbuf);
  strbuf_nappend(&sbuf, exepath, path - exepath);
  strbuf_append(&sbuf, "pkg/");
  Object *pkgpath = string_new(strbuf_tostr(&sbuf));
  printf("%s\n", strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  return pkgpath;
}

static FieldDef sys_vars[] = {
  //{"path", "[s", field_default_getter, field_default_setter, default_path},
  //{"stdin",   "Lfs.File;",  get_stdin,  set_stdin,  default_stdin },
  //{"stdout", "Lfs.File;", field_default_getter, field_default_setter, default_stdout},
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

static MethodDef sys_funcs[] = {
  {"load_library", "s", NULL, os_load_library},
  {NULL},
};

void init_sys_module(void)
{
  Object *m = module_new("sys");
  module_add_vardefs(m, sys_vars);
  module_add_funcdefs(m, sys_funcs);
  module_install("sys", m);
  OB_DECREF(m);
}

void fini_sys_module(void)
{
  module_uninstall("sys");
}
