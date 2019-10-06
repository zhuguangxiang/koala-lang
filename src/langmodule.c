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
#include "opcode.h"

static void show_args(int op, int index, uint8_t *codes, Object *consts)
{

#define ONE_BYTE() ({ \
  codes[index]; \
})

#define TWO_BYTES() ({ \
  uint8_t l = codes[index]; \
  uint8_t h = codes[index + 1]; \
  ((h << 8) + l); \
})

  static char *path = NULL;
  static char *type = NULL;
  Object *ob;
  int val;

  switch (op) {
  case OP_LOAD_CONST: {
    val = TWO_BYTES();
    ob = tuple_get(consts, val);
    if (integer_check(ob)) {
      printf("%ld", integer_asint(ob));
    } else if (string_check(ob)) {
      printf("'%.64s'", string_asstr(ob));
    } else {
      printf("<null>");
    }
    OB_DECREF(ob);
    break;
  }
  case OP_LOAD_MODULE:
  case OP_GET_METHOD:
  case OP_GET_VALUE:
  case OP_SET_VALUE:
  case OP_CALL: {
    val = TWO_BYTES();
    ob = tuple_get(consts, val);
    printf("'%.64s'", string_asstr(ob));
    OB_DECREF(ob);
    break;
  }
  case OP_JMP:
  case OP_JMP_TRUE:
  case OP_JMP_FALSE: {
    val = (short)TWO_BYTES();
    printf("%d", val);
    break;
  }
  case OP_LOAD:
  case OP_STORE: {
    val = ONE_BYTE();
    printf("%d", val);
    break;
  }
  case OP_NEW: {
    val = TWO_BYTES();
    ob = tuple_get(consts, val);
    TypeDesc *desc = descob_getdesc(ob);
    if (desc->kind == TYPE_KLASS) {
      path = desc->klass.path;
      type = desc->klass.type;
      if (path != NULL)
        printf("%.64s", path);
      printf("%.64s", type);
    }
    OB_DECREF(ob);
    break;
  }
  case OP_INIT_CALL: {
    if (path != NULL)
      printf("%.64s", path);
    printf("%.64s.'__init__'", type);
    break;
  }
  default:
    break;
  }
}

static Object *disassemble(Object *self, Object *arg)
{
  if (!method_check(arg)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(arg));
    return NULL;
  }

  MethodObject *meth = (MethodObject *)arg;
  if (meth->cfunc) {
    printf("'%.64s' is cfunc\n", meth->name);
    return NULL;
  }

  CodeObject *co = (CodeObject *)meth->ptr;
  int size = co->size;
  uint8_t *codes = co->codes;
  int index = 0;
  int op;
  char *opname;
  int bytes;
  printf("assembly of func '%.64s':\n", co->name);
  printf("  locals=%d, freevals=%d, upvals=%d\n", vector_size(&co->locvec),
         vector_size(&co->freevec), vector_size(&co->upvec));
  while (index < co->size) {
    printf("%6d", index);
    op = codes[index++];
    bytes = opcode_argc(op);
    printf("%6d", bytes + 1);
    opname = opcode_str(op);
    printf("  %-16s", opname);
    show_args(op, index, codes, co->consts);
    index += bytes;
    printf("\n");
  }

  return NULL;
}

static Object *help(Object *self, Object *arg)
{

}

static Object *_exit_(Object *self, Object *arg)
{
  extern int halt;
  //graceful halt
  halt = 1;
  return NULL;
}

static MethodDef lang_methods[] = {
  {"disassemble", "A", NULL, disassemble},
  {"help", "A", NULL, help},
  {"exit", NULL, NULL, _exit_},
  {NULL}
};

void init_lang_module(void)
{
  Object *m = module_new("lang");
  module_add_type(m, &any_type);
  module_add_type(m, &byte_type);
  module_add_type(m, &integer_type);
  module_add_type(m, &bool_type);
  module_add_type(m, &string_type);
  module_add_type(m, &char_type);
  module_add_type(m, &float_type);
  module_add_type(m, &array_type);
  module_add_type(m, &tuple_type);
  module_add_type(m, &map_type);
  module_add_type(m, &field_type);
  module_add_type(m, &method_type);
  module_add_type(m, &proto_type);
  module_add_type(m, &class_type);
  module_add_type(m, &module_type);
  module_add_type(m, &iter_type);
  module_add_type(m, &range_type);
  module_add_type(m, &code_type);
  module_add_type(m, &closure_type);
  module_add_funcdefs(m, lang_methods);
  module_install("lang", m);
  OB_DECREF(m);
}

void fini_lang_module(void)
{
  module_uninstall("lang");
}
