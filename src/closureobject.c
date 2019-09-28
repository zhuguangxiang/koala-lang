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
    --up->refcnt;
    expect(up->refcnt >= 0);
    if (up->refcnt == 0) {
      debug("[Closure] free upval '%s'", up->name);
      upval_free(up);
    } else {
      debug("[Closure] upval '%s' refcnt is %d ", up->name, up->refcnt);
    }
  }
  vector_free(closure->upvals);
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

UpVal *upval_new(char *name, Object **ref)
{
  UpVal *up = kmalloc(sizeof(UpVal));
  up->name = name;
  up->refcnt = 1;
  up->ref = ref;
  return up;
}

void upval_free(UpVal *val)
{
  OB_DECREF(val->value);
  kfree(val);
}

Object *upval_load(Object *ob, int index)
{
  if (!closure_check(ob)) {
    error("object of '%.64s' is not a Closure", OB_TYPE_NAME(ob));
    return NULL;
  }

  ClosureObject *self = (ClosureObject *)ob;
  UpVal *up = vector_get(self->upvals, index);
  expect(up != NULL);
  debug("load upval('%s') by index %d", up->name, index);
  expect(up->ref != NULL);
  Object *res = *up->ref;
  return OB_INCREF(res);
}
