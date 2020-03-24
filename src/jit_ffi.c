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

#include "jit_ffi.h"
#include "koala.h"

static void *int_value(Object *ob)
{
  int64_t *val = kmalloc(sizeof(int64_t));
  *val = integer_asint(ob);
  return val;
}

static void *byte_value(Object *ob)
{
  uint8_t *val = kmalloc(sizeof(uint8_t));
  *val = byte_asint(ob);
  return val;
}

static void *float_value(Object *ob)
{
  double *val = kmalloc(sizeof(double));
  *val = float_asflt(ob);
  return val;
}

static void *bool_value(Object *ob)
{
  uint8_t *val = kmalloc(sizeof(uint8_t));
  *val = bool_istrue(ob);
  return val;
}

static void *char_value(Object *ob)
{
  uint32_t *val = kmalloc(sizeof(uint32_t));
  *val = char_asch(ob);
  return val;
}

static void *klass_value(Object *ob)
{
  void *val = kmalloc(sizeof(void *));
  *(void **)val = ob;
  return val;
}

static Object *int_object(void *ptr)
{
  int64_t val = *(int64_t *)ptr;
  return integer_new(val);
}

static struct base_mapping {
  int kind;
  ffi_type *type;
  void *(*value)(Object *);
  Object *(*object)(void *);
} base_mappings[] = {
  {BASE_INT,    &ffi_type_sint64,   int_value,    int_object},
  {BASE_BYTE,   &ffi_type_uint8,    byte_value,   int_object},
  {BASE_FLOAT,  &ffi_type_double,   float_value,  int_object},
  {BASE_BOOL,   &ffi_type_uint8,    bool_value,   int_object},
  {BASE_STR,    &ffi_type_pointer,  klass_value,  int_object},
  {BASE_CHAR,   &ffi_type_uint32,   char_value,   int_object},
  {BASE_ANY,    &ffi_type_pointer,  klass_value,  int_object},
  {0,           NULL,               NULL,         int_object},
};

ffi_type *jit_ffi_type(TypeDesc *desc)
{
  if (!desc_isbase(desc)) {
    // type of koala's klass maybe ffi_type_pointer of struct.
    return &ffi_type_pointer;
  }

  // koala's type is base type
  struct base_mapping *m = base_mappings;
  while (m->type != NULL) {
    if (m->kind == desc->base) {
      return m->type;
    }
    ++m;
  }

  // here: bug!
  panic("why go here?");
  return NULL;
}

static void *obj_to_val(Object *ob, TypeDesc *desc)
{
  if (!desc_isbase(desc)) {
    // koala's klass ?
    return klass_value(ob);
  }

  // koala's type is base type
  struct base_mapping *m = base_mappings;
  while (m->type != NULL) {
    if (m->kind == desc->base) {
      return m->value(ob);
    }
    ++m;
  }

  // here: bug!
  panic("why go here?");
  return NULL;
}

static Object *val_to_obj(void *rval, TypeDesc *desc)
{
  if (!desc_isbase(desc)) {
    // koala's klass ?
    return NULL;
  }

  // koala's type is base type
  struct base_mapping *m = base_mappings;
  while (m->type != NULL) {
    if (m->kind == desc->base) {
      return m->object(rval);
    }
    ++m;
  }

  // here: bug!
  panic("why go here?");
  return NULL;
}

static Vector fninfos;

jit_func_t *jit_get_func(void *mcptr, TypeDesc *desc)
{
  Vector *types = desc->proto.args;
  int nargs = vector_size(types);
  jit_func_t *fn = kmalloc(sizeof(jit_func_t) + nargs * sizeof(void *));
  expect(fn != NULL);

  // argument types
  TypeDesc *type;
  for (int i = 0; i < nargs; i++) {
    type = vector_get(types, i);
    fn->argtypes[i] = jit_ffi_type(type);
  }

  // return type
  type = desc->proto.ret;
  fn->rtype = jit_ffi_type(type);

  // perpare ffi_cif
  ffi_prep_cif(&fn->cif, FFI_DEFAULT_ABI, nargs, fn->rtype, fn->argtypes);
  fn->mcptr = mcptr;
  fn->nargs = nargs;
  fn->desc = TYPE_INCREF(desc);

  // save to vector for free
  vector_push_back(&fninfos, fn);

  return fn;
}

static void fill_args_value(jit_func_t *fninfo, Object *args, Vector *vec)
{
  TypeDesc *desc = fninfo->desc;
  Vector *types = desc->proto.args;
  int nargs = vector_size(types);
  TypeDesc *type;
  void *val;
  if (nargs == 1) {
    type = vector_get(types, 0);
    val = obj_to_val(args, type);
    vector_push_back(vec, val);
  } else if (nargs > 1) {
    Object *ob;
    for (int i = 0; i < nargs; i++) {
      type = vector_get(types, i);
      ob = tuple_get(args, i);
      val = obj_to_val(ob, type);
      vector_push_back(vec, val);
      OB_DECREF(ob);
    }
  } else {
    // no need to fill values
  }
}

static void free_args_value(jit_func_t *fninfo, Vector *vec)
{
  void *v;
  vector_for_each(v, vec) {
    kfree(v);
  }
  vector_fini(vec);
}

Object *jit_ffi_call(jit_func_t *fninfo, Object *args)
{
  VECTOR(vec);
  fill_args_value(fninfo, args, &vec);
  void **avalue = (void **)vec.items;
  void *rvalue = kmalloc(fninfo->cif.rtype->size);
  ffi_call(&fninfo->cif, fninfo->mcptr, rvalue, avalue);
  TypeDesc *desc = fninfo->desc;
  Object *ret = val_to_obj(rvalue, desc->proto.ret);
  free_args_value(fninfo, &vec);
  kfree(rvalue);
  return ret;
}

void fini_jit_ffi(void)
{
  jit_func_t *fn;
  vector_for_each(fn, &fninfos) {
    TYPE_DECREF(fn->desc);
    kfree(fn);
  }
  vector_fini(&fninfos);
}
