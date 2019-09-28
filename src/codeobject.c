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

#include "codeobject.h"
#include "closureobject.h"
#include "image.h"
#include "atom.h"

static void code_clean(Object *ob)
{
  if (!code_check(ob)) {
    error("object of '%.64s' is not a Code", OB_TYPE_NAME(ob));
    return;
  }

  CodeObject *co = (CodeObject *)ob;
  debug("clean code '%s'", co->name);

  /* free local variables */
  LocVar *item;
  vector_for_each(item, &co->locvec) {
    locvar_free(item);
  }
  vector_fini(&co->locvec);
  vector_fini(&co->freevec);
  vector_fini(&co->upvec);

  /* free constant pool */
  OB_DECREF(co->consts);

  /* free func's descriptor */
  TYPE_DECREF(co->proto);
}

static void code_free(Object *ob)
{
  code_clean(ob);
  gcfree(ob);
}

TypeObject code_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Code",
  .clean = code_clean,
  .free  = code_free,
};

Object *code_new(char *name, TypeDesc *proto, uint8_t *codes, int size)
{
  debug("new '%s' code", name);
  CodeObject *co = gcmalloc(sizeof(CodeObject) + size);
  init_object_head(co, &code_type);
  co->name = atom(name);
  expect(proto->kind == TYPE_PROTO);
  co->proto = TYPE_INCREF(proto);
  co->size = size;
  memcpy(co->codes, codes, size);
  return (Object *)co;
}
