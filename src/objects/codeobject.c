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

#include "log.h"
#include "mem.h"
#include "codeobject.h"

LOGGER(0)

Object *Code_New(char *name, TypeDesc *proto, uint8 *codes, int size)
{
  CodeObject *code = Malloc(sizeof(CodeObject) + sizeof(CodeInfo));
  Init_Object_Head(code, &Code_Klass);
  TYPE_INCREF(proto);
  code->name = name;
  code->kind = KCODE_KIND;
  code->proto = proto;
  CodeInfo *ci = (CodeInfo *)(code + 1);
  Vector_Init(&ci->locvec);
  ci->codes = codes;
  ci->size = size;
  code->codeinfo = ci;
  return (Object *)code;
}

static Object *cfunc_new(char *name, TypeDesc *proto, cfunc_t func)
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

Object *Code_From_CFunction(CFunctionDef *f)
{
  Vector *rdesc = String_ToTypeList(f->rdesc);
  Vector *pdesc = String_ToTypeList(f->pdesc);
  TypeDesc *proto = TypeDesc_Get_Proto(pdesc, rdesc);
  return cfunc_new(f->name, proto, f->func);
}

void Code_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *code = (CodeObject *)ob;

  TYPE_DECREF(code->proto);
  if (IsKCode(code)) {
    CodeInfo *ci = code->codeinfo;
    Mfree(ci->codes);
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

static CFunctionDef code_funcs[] = {
  {"Disassemble", NULL, NULL, __code_disassemble},
  {NULL}
};

void Init_Code_Klass(void)
{
  Init_Klass(&Code_Klass, NULL);
  Klass_Add_CFunctions(&Code_Klass, code_funcs);
}

void Fini_Code_Klass(void)
{
  Fini_Klass(&Code_Klass);
}

Klass Code_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Code",
};
