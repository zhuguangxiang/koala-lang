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

#include "methodobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "codeobject.h"
#include "eval.h"
#include "atom.h"

Object *CMethod_New(MethodDef *def)
{
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name  = def->name;
  method->desc  = str_to_proto(def->ptype, def->rtype);
  method->cfunc = 1,
  method->ptr   = def->func;
  return (Object *)method;
}

Object *Method_New(char *name, Object *code)
{
  CodeObject *co = (CodeObject *)code;
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name = atom(name);
  method->desc = TYPE_INCREF(co->proto);
  method->ptr  = OB_INCREF(code);
  return (Object *)method;
}

Object *Method_Call(Object *self, Object *ob, Object *args)
{
  if (!method_check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }
  MethodObject *meth = (MethodObject *)self;
  if (meth->cfunc) {
    func_t fn = meth->ptr;
    return fn(ob, args);
  } else {
    return koala_evalcode(meth->ptr, ob, args);
  }
}

Object *method_getcode(Object *self)
{
  if (!method_check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }
  MethodObject *meth = (MethodObject *)self;
  if (!meth->cfunc) {
    return OB_INCREF(meth->ptr);
  } else {
    error("method '%s' is cfunc", meth->name);
    return NULL;
  }
}

static Object *method_call(Object *self, Object *args)
{
  if (!method_check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }

  expect(args != NULL);

  Object *ob;
  Object *para;
  if (!Tuple_Check(args)) {
    ob = OB_INCREF(args);
    para = NULL;
  } else {
    int size = Tuple_Size(args);
    expect(size > 0);
    ob = tuple_get(args, 0);
    if (size == 2)
      para = tuple_get(args, 1);
    else
      para = Tuple_Slice(args, 1, -1);
  }

  MethodObject *meth = (MethodObject *)self;
  Object *res;
  if (meth->cfunc) {
    func_t fn = meth->ptr;
    res = fn(ob, para);
  } else {
    panic("not implemented");
    res = NULL;
  }
  OB_DECREF(ob);
  OB_DECREF(para);
  return res;
}

static void meth_free(Object *ob)
{
  if (!method_check(ob)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(ob));
    return;
  }
  MethodObject *meth = (MethodObject *)ob;
  debug("[Freed] Method %s", meth->name);
  TYPE_DECREF(meth->desc);
  if (!meth->cfunc) {
    OB_DECREF(meth->ptr);
  }
  kfree(ob);
}

static MethodDef meth_methods[] = {
  {"call", "...", "A", method_call},
  {NULL}
};

static Object *meth_str(Object *self, Object *args)
{
  if (!method_check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }

  MethodObject *meth = (MethodObject *)self;
  return string_new(meth->name);
}

TypeObject method_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Method",
  .free    = meth_free,
  .str     = meth_str,
  .methods = meth_methods,
};

TypeObject proto_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Proto",
};

void init_method_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Method");
  method_type.desc = desc;
  if (type_ready(&method_type) < 0)
    panic("Cannot initalize 'Method' type.");
}

void init_proto_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Proto");
  proto_type.desc = desc;
  if (type_ready(&proto_type) < 0)
    panic("Cannot initalize 'Proto' type.");
}
