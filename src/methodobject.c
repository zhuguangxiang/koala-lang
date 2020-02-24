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
#include "valistobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "codeobject.h"
#include "eval.h"
#include "atom.h"

Object *cmethod_new(MethodDef *def)
{
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name  = def->name;
  method->desc  = str_to_proto(def->ptype, def->rtype);
  method->cfunc = 1,
  method->ptr   = def->func;
  return (Object *)method;
}

Object *method_new(char *name, Object *code)
{
  CodeObject *co = (CodeObject *)code;
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name = atom(name);
  method->desc = TYPE_INCREF(co->proto);
  method->ptr  = OB_INCREF(code);
  return (Object *)method;
}

Object *nmethod_new(char *name, TypeDesc *desc, void *ptr)
{
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name = atom(name);
  method->desc = TYPE_INCREF(desc);
  method->ptr  = NULL;
  method->cfunc = 1;
  method->native = 1;
  method->ptr = ptr;
  return (Object *)method;
}

static Object *get_valist_args(Object *args, Vector *argtypes)
{
  int size = vector_size(argtypes);
  int start = size - 1;
  TypeDesc *lasttype = vector_top_back(argtypes);
  TypeDesc *subtype = vector_get(lasttype->klass.typeargs, 0);

  if (start == 0) {
    if (args == NULL) {
      return valist_new(0, subtype);
    }

    if (!tuple_check(args)) {
      Object *valist = valist_new(1, subtype);
      valist_set(valist, 0, args);
      return valist;
    } else {
      Object *ob;
      Object *valist = valist_new(tuple_size(args), subtype);
      for (int i = 0; i < tuple_size(args); ++i) {
        ob = tuple_get(args, i);
        valist_set(valist, i, ob);
        OB_DECREF(ob);
      }
      return valist;
    }
  } else {
    expect(start >= 1);
    expect(args != NULL);
    if (!tuple_check(args)) {
      Object *nargs = tuple_new(size);
      tuple_set(nargs, 0, args);
      Object *valist = valist_new(0, subtype);
      tuple_set(nargs, 1, valist);
      OB_DECREF(valist);
      return nargs;
    } else {
      Object *ob;
      Object *nargs = tuple_new(size);
      for (int i = 0; i < start; ++i) {
        ob = tuple_get(args, i);
        tuple_set(nargs, i, ob);
        OB_DECREF(ob);
      }

      Object *valist;
      ob = tuple_get(args, start);
      if (!valist_check(ob)) {
        OB_DECREF(ob);
        valist = valist_new(tuple_size(args) - start, subtype);
        for (int i = start; i < tuple_size(args); ++i) {
          ob = tuple_get(args, i);
          valist_set(valist, i - start, ob);
          OB_DECREF(ob);
        }
      } else {
        valist = ob;
        expect(tuple_size(args) == start + 1);
      }
      tuple_set(nargs, start, valist);
      OB_DECREF(valist);
      return nargs;
    }
  }
}

Object *method_call(Object *self, Object *ob, Object *args)
{
  if (!method_check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }

  MethodObject *meth = (MethodObject *)self;
  Vector *argtypes = meth->desc->proto.args;
  TypeDesc *lasttype = vector_top_back(argtypes);
  Object *nargs = args;
  if (lasttype != NULL && desc_isvalist(lasttype)) {
    nargs = get_valist_args(args, argtypes);
  }

  Object *res;
  if (meth->cfunc) {
    func_t fn = meth->ptr;
    if (fn == NULL && meth->native) {
      debug("try to find native function '%s'", meth->name);
    }
    res = fn(ob, nargs);
  } else {
    res = koala_evalcode(meth->ptr, ob, nargs, NULL);
  }

  if (nargs != args)
    OB_DECREF(nargs);

  return res;
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

static Object *_method_call_(Object *self, Object *args)
{
  if (!method_check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }

  expect(args != NULL);

  Object *ob;
  Object *para;
  if (!tuple_check(args)) {
    ob = OB_INCREF(args);
    para = NULL;
  } else {
    int size = tuple_size(args);
    expect(size > 0);
    ob = tuple_get(args, 0);
    if (size == 2)
      para = tuple_get(args, 1);
    else
      para = tuple_slice(args, 1, -1);
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

static MethodDef meth_methods[] = {
  {"call", "...", "A", _method_call_},
  {NULL}
};

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

static void proto_free(Object *ob)
{
  if (!proto_check(ob)) {
    error("object of '%.64s' is not a Proto", OB_TYPE_NAME(ob));
    return;
  }
  ProtoObject *proto = (ProtoObject *)ob;
  debug("[Freed] Protot %s", proto->name);
  TYPE_DECREF(proto->desc);
  kfree(ob);
}

static Object *proto_str(Object *self, Object *args)
{
  if (!proto_check(self)) {
    error("object of '%.64s' is not a Proto", OB_TYPE_NAME(self));
    return NULL;
  }

  ProtoObject *proto = (ProtoObject *)self;
  return string_new(proto->name);
}

TypeObject proto_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Proto",
  .free = proto_free,
  .str  = proto_str,
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

Object *proto_new(char *name, TypeDesc *desc)
{
  ProtoObject *proto = kmalloc(sizeof(ProtoObject));
  init_object_head(proto, &proto_type);
  proto->name = atom(name);
  proto->desc = TYPE_INCREF(desc);
  return (Object *)proto;
}
