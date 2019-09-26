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

#include "closureobject.h"

static void closure_free(Object *ob)
{
  if (!closure_check(ob)) {
    error("object of '%.64s' is not a Closure", OB_TYPE_NAME(ob));
    return;
  }
  debug("[Freed] Closure");
  ClosureObject *closure = (ClosureObject *)ob;
  OB_DECREF(closure->code);
  UpVal *up;
  vector_for_each(up, closure->upvals) {

  }
  kfree(ob);
}

TypeObject closure_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Closure",
  .free    = closure_free,
};


void init_closure_type(void)
{
  closure_type.desc = desc_from_klass("lang", "Closure");
  if (type_ready(&closure_type) < 0)
    panic("Cannot initalize 'Closure' type.");
}

Object *closure_new(Object *code, Vector *upvals)
{
  ClosureObject *closure = kmalloc(sizeof(ClosureObject));
  init_object_head(closure, &closure_type);
  closure->code = OB_INCREF(code);
  closure->upvals = upvals;
  return (Object *)closure;
}

UpVal *upval_new(Object **ref)
{
  UpVal *up = kmalloc(sizeof(UpVal));
  up->ref = ref;
  return up;
}

Object *closure_load(Object *ob, int index)
{
  if (!closure_check(ob)) {
    error("object of '%.64s' is not a Closure", OB_TYPE_NAME(ob));
    return NULL;
  }

  ClosureObject *self = (ClosureObject *)ob;
  debug("closure load index %d", index);
  UpVal *up = vector_get(self->upvals, index - 1);
  expect(up != NULL);
  expect(up->ref != NULL);
  return *up->ref;
}
