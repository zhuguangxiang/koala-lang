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

static CodeObject *code_new(CodeKind kind, TypeDesc *proto)
{
  CodeObject *code = mm_alloc(sizeof(CodeObject));
  Init_Object_Head(code, &Code_Klass);
  code->kind = kind;
  code->proto = proto;
  return code;
}

Object *KCode_New(uint8 *codes, int size, TypeDesc *proto)
{
  CodeObject *code = code_new(CODE_KLANG, proto);
  code->kf.codes = codes;
  code->kf.size = size;
  return (Object *)code;
}

Object *CCode_New(cfunc cf, TypeDesc *proto)
{
  CodeObject *code = code_new(CODE_CLANG, proto);
  code->cf = cf;
  return (Object *)code;
}

void CodeObject_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *code = (CodeObject *)ob;

  TypeDesc_Free(code->proto);
  if (CODE_IS_K(code)) {
    //FIXME
  }

  mm_free(ob);
}

int KCode_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int pos)
{
  OB_ASSERT_KLASS(ob, Code_Klass);
  CodeObject *code = (CodeObject *)ob;
  assert(CODE_IS_K(code));

  MemberDef *member = MemberDef_New(MEMBER_VAR, name, desc, 0);
  member->offset = pos;
  Vector_Append(&code->kf.locvec, member);
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
  assert(!args);
  CodeObject *cob = (CodeObject *)ob;
  //FIXME
  return NULL;
}

static FuncDef code_funcs[] = {
  {"DisAssemble", NULL, NULL, __code_disassemble},
  {NULL}
};

void Init_Code_Klass(void)
{
  Klass_Add_CFunctions(&Code_Klass, code_funcs);
}

void Fini_Code_Klass(void)
{
  Fini_Klass(&Code_Klass);
}

Klass Code_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass)
  .name = "Code",
  .basesize = sizeof(CodeObject),
};
