/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "codeobject.h"
#include "opcode.h"
#include "log.h"
#include "mem.h"

LOGGER(0)

Object *Code_New(char *name, TypeDesc *proto, uint8 *codes, int size)
{
  int memsize = sizeof(CodeObject) + sizeof(CodeInfo) + size;
  CodeObject *code = Malloc(memsize);
  Init_Object_Head(code, &Code_Klass);
  TYPE_INCREF(proto);
  code->name = AtomString_New(name).str;
  code->kind = KCODE_KIND;
  code->proto = proto;
  CodeInfo *ci = (CodeInfo *)(code + 1);
  Vector_Init(&ci->locvec);
  memcpy(ci->codes, codes, size);
  ci->size = size;
  code->codeinfo = ci;
  return (Object *)code;
}

Object *CFunc_New(char *name, TypeDesc *proto, cfunc_t func)
{
  CodeObject *code = Malloc(sizeof(CodeObject));
  Init_Object_Head(code, &Code_Klass);
  TYPE_INCREF(proto);
  code->name = name;
  code->kind = CFUNC_KIND;
  code->proto = proto;
  code->cfunc = func;
  return (Object *)code;
}

Object *Code_From_CFunc(CFuncDef *f)
{
  Vector *para = String_ToTypeList(f->pdesc);
  TypeDesc *ret = String_To_TypeDesc(f->rdesc);
  TypeDesc *proto = TypeDesc_New_Proto(para, ret);
  return CFunc_New(f->name, proto, f->func);
}

void Code_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *code = (CodeObject *)ob;
  TYPE_DECREF(code->proto);
  if (code->kind == KCODE_KIND) {
    CodeInfo *ci = code->codeinfo;
    OB_DECREF(ci->consts);
    MNode *m;
    Vector_ForEach(m, &ci->locvec) {
      MNode_Free(m);
    }
    Vector_Fini_Self(&ci->locvec);
  }
  Mfree(ob);
}

int Code_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int index)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *code = (CodeObject *)ob;
  assert(IsKCode(ob));
  CodeInfo *ci = code->codeinfo;

  MNode *m = MNode_New(VAR_KIND, name, desc);
  m->offset = index;
  Vector_Append(&ci->locvec, m);
  return 0;
}

int Code_Get_Argc(Object *ob)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *code = (CodeObject *)ob;
  Vector *desc = ((ProtoDesc *)code->proto)->arg;
  return (desc == NULL) ? 0: Vector_Size(desc);
}

static Object *__code_disassemble(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *cob = (CodeObject *)ob;
  //FIXME
  return NULL;
}

static CFuncDef code_funcs[] = {
  {"Disassemble", NULL, NULL, __code_disassemble},
  {NULL}
};

void Init_Code_Klass(void)
{
  Init_Klass_Self(&Code_Klass);
  Klass_Add_CMethods(&Code_Klass, code_funcs);
}

void Fini_Code_Klass(void)
{
  assert(OB_REFCNT(&Code_Klass) == 1);
  Fini_Klass(&Code_Klass);
}

Klass Code_Klass = {
  OBJECT_HEAD_INIT(&Class_Klass)
  .name = "Code",
  .ob_free = Code_Free,
};

#if 0

static TypeDesc *any_any_proto(void)
{
  TypeDesc *any = TypeDesc_Get_Base(BASE_ANY);
  return TypeDesc_Get_SProto(any, any);
}

static TypeDesc *void_2any_proto(void)
{
  TypeDesc *any = TypeDesc_Get_Base(BASE_ANY);
  Vector *arg = Vector_New();
  TYPE_INCREF(any);
  Vector_Append(arg, any);
  TYPE_INCREF(any);
  Vector_Append(arg, any);
  return TypeDesc_Get_Proto(arg, NULL);
}

static TypeDesc *int_void_proto(void)
{
  TypeDesc *desc = TypeDesc_Get_Base(BASE_INT);
  return TypeDesc_Get_SProto(NULL, desc);
}

static TypeDesc *bool_any_proto(void)
{
  TypeDesc *arg = TypeDesc_Get_Base(BASE_ANY);
  TypeDesc *ret = TypeDesc_Get_Base(BASE_BOOL);
  return TypeDesc_Get_SProto(arg, ret);
}

static TypeDesc *str_void_proto(void)
{
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  return TypeDesc_Get_SProto(NULL, desc);
}

static void init_operator(Klass *klazz, int op, cfunc_t func)
{
  if (func == NULL)
    return;

  char *name = OpCode_Operator(op);
  TypeDesc *proto = any_any_proto();
  Object *code = CFunc_New(name, proto, func);
  Klass_Add_Method(klazz, code);
}

static void init_operator2(Klass *klazz, int op, cfunc_t func)
{
  if (func == NULL)
    return;

  char *name = OpCode_Operator(op);
  TypeDesc *proto = void_2any_proto();
  Object *code = CFunc_New(name, proto, func);
  Klass_Add_Method(klazz, code);
}

void Init_Mapping_Operators(Klass *klazz)
{
  Object *code;
  char *name;
  TypeDesc *proto;
  NumberOperations *nuops = klazz->num_ops;
  MapOperations *mapops = klazz->map_ops;

  if (nuops != NULL) {
    init_operator(klazz, ADD, nuops->add);
    init_operator(klazz, SUB, nuops->sub);
    init_operator(klazz, MUL, nuops->mul);
    init_operator(klazz, DIV, nuops->div);
    init_operator(klazz, MOD, nuops->mod);
    init_operator(klazz, POW, nuops->pow);
    init_operator(klazz, NEG, nuops->neg);

    init_operator(klazz, GT,  nuops->gt);
    init_operator(klazz, GT,  nuops->ge);
    init_operator(klazz, LT,  nuops->lt);
    init_operator(klazz, LE,  nuops->le);
    init_operator(klazz, EQ,  nuops->eq);
    init_operator(klazz, NEQ, nuops->neq);

    init_operator(klazz, BAND, nuops->band);
    init_operator(klazz, BOR,  nuops->bor);
    init_operator(klazz, BXOR, nuops->bxor);
    init_operator(klazz, BNOT, nuops->bnot);
    init_operator(klazz, LSHIFT, nuops->lshift);
    init_operator(klazz, RSHIFT, nuops->rshift);

    init_operator(klazz, AND, nuops->land);
    init_operator(klazz, OR,  nuops->lor);
    init_operator(klazz, NOT, nuops->lnot);
  }

  if (mapops != NULL) {
    init_operator(klazz, MAP_LOAD, mapops->get);
    init_operator2(klazz, MAP_STORE, mapops->set);
  }

  if (klazz->ob_hash != NULL) {
    TypeDesc *proto = int_void_proto();
    Object *code = CFunc_New("__hash__", proto, klazz->ob_hash);
    Klass_Add_Method(klazz, code);
  }

  if (klazz->ob_cmp != NULL) {
    TypeDesc *proto = bool_any_proto();
    Object *code = CFunc_New("__cmp__", proto, klazz->ob_cmp);
    Klass_Add_Method(klazz, code);
  }

  if (klazz->ob_str != NULL) {
    TypeDesc *proto = str_void_proto();
    Object *code = CFunc_New("__tostring__", proto, klazz->ob_str);
    Klass_Add_Method(klazz, code);
  }
}
#endif
