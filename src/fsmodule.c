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

static void file_free(Object *ob)
{
  if (!file_check(ob)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(ob));
    return;
  }

  FileObject *file = (FileObject *)ob;
  if (file->fp != NULL)
    fclose(file->fp);

  kfree(file);
}

static Object *file_read(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  return result_new_ok(integer_new(200));
}

/* func write(buf [byte]) Result<int, Error> */
static Object *file_write(Object *self, Object *args)
{
  if (!file_check(self)) {
    error("object of '%.64s' is not a File", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!array_check(args)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(args));
    return NULL;
  }

  FileObject *file = (FileObject *)self;
  char *buf = array_raw(args);
  int size = array_size(args);
  int nbytes = fwrite(buf, 1, size, file->fp);
  debug("expect %d bytes, and write %d bytes", size, nbytes);
  return result_new_ok(integer_new(100));
}

static MethodDef file_methods[] = {
  {"read", "[bi",  "Llang.Result(i)(Llang.Error;);",  file_read},
  {"write", "[b", "Llang.Result(i)(Llang.Error;);", file_write},
  {NULL}
};

TypeObject file_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "File",
  .free    = file_free,
  .methods = file_methods,
};

static Object *open_file(Object *self, Object *ob)
{
  return NULL;
}

static Object *mkdir(Object *self, Object *ob)
{
  return NULL;
}

static MethodDef fs_funcs[] = {
  {"open",  "ss", "Lfs.File;",  open_file },
  {"mkdir", "s",  NULL,         mkdir     },
  {NULL},
};

void init_fs_module(void)
{
  file_type.desc = desc_from_klass("fs", "File");
  int res = type_ready(&file_type);
  if (res != 0)
    panic("Cannot initalize 'File' type.");

  Object *m = module_new("fs");
  module_add_funcdefs(m, fs_funcs);
  module_add_type(m, &file_type);
  module_install("fs", m);
  OB_DECREF(m);
}

void fini_fs_module(void)
{
  type_fini(&file_type);
  module_uninstall("fs");
}

Object *file_new(FILE *fp)
{
  FileObject *file = kmalloc(sizeof(FileObject));
  init_object_head(file, &file_type);
  file->fp = fp;
  return (Object *)file;
}
