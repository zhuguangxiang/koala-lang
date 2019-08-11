/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_CODEGEN_H_
#define _KOALA_CODEGEN_H_

#include "image.h"
#include "opcode.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct inst {
  struct list_head link;
  int bytes;
  int argc;
  uint8_t op;
  ConstValue arg;
  /* break and continue statements */
  int upbytes;
} Inst;

#define JMP_BREAK    1
#define JMP_CONTINUE 2

typedef struct jmp_inst {
  int type;
  Inst *inst;
} JmpInst;

typedef struct codeblock {
  int bytes;
  struct list_head insts;
  /* control flow */
  struct codeblock *next;
  /* false, no OP_RET, needs add one */
  int ret;
} CodeBlock;

Inst *Inst_Append(CodeBlock *b, uint8_t op, ConstValue *val);
Inst *Inst_Append_NoArg(CodeBlock *b, uint8_t op);
JmpInst *JmpInst_New(Inst *inst, int type);
void JmpInst_Free(JmpInst *jmp);

CodeBlock *CodeBlock_New(void);
void CodeBlock_Free(CodeBlock *block);
void CodeBlock_Merge(CodeBlock *from, CodeBlock *to);
void CodeBlock_Show(CodeBlock *block);
int CodeBlock_To_RawCode(Image *image, CodeBlock *block, uint8_t **code);

#define CODE_DUP(block) \
({ \
  Inst_Append_NoArg(block, DUP); \
})

#define CODE_LOAD_CONST(block, val) \
({ \
  Inst_Append(block, LOAD_CONST, &(val)); \
})

#define CODE_LOAD_PKG(block, path) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = path}; \
  Inst_Append(block, LOAD_PKG, &val); \
})

#define CODE_LOAD(block, index) \
({ \
  if (index == 0) { \
    Inst_Append_NoArg(block, LOAD_0); \
  } else if (index == 1) { \
    Inst_Append_NoArg(block, LOAD_1); \
  } else if (index == 2) { \
    Inst_Append_NoArg(block, LOAD_2); \
  } else if (index == 3) { \
    Inst_Append_NoArg(block, LOAD_3); \
  } else { \
    assert(index >= 0 && index < 250); \
    ConstValue val = {.kind = BASE_INT, .ival = index}; \
    Inst_Append(block, LOAD, &val); \
  } \
})

#define CODE_STORE(block, index) \
({ \
  if (index == 0) { \
    Inst_Append_NoArg(block, STORE_0); \
  } else if (index == 1) { \
    Inst_Append_NoArg(block, STORE_1); \
  } else if (index == 2) { \
    Inst_Append_NoArg(block, STORE_2); \
  } else if (index == 3) { \
    Inst_Append_NoArg(block, STORE_3); \
  } else { \
    assert(index >= 0 && index < 250); \
    ConstValue val = {.kind = BASE_INT, .ival = index}; \
    Inst_Append(block, STORE, &val); \
  } \
})

#define CODE_GET_ATTR(block, name) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = name}; \
  Inst_Append(block, GET_ATTR, &val); \
})

#define CODE_SET_ATTR(block, name) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = name}; \
  Inst_Append(block, SET_ATTR, &val); \
})

#define CODE_CALL(block, name, _argc) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = name}; \
  Inst *i = Inst_Append(block, CALL, &val); \
  i->argc = _argc; \
})

#define CODE_BINARY(block, op) \
({ \
  Inst_Append_NoArg(block, op); \
})

#define CODE_NEW_OBJECT(block, name) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = name}; \
  Inst_Append(block, NEW, &val); \
  Inst_Append_NoArg(block, DUP); \
})

#define CODE_NEW_EVAL(block, name, _argc) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = name}; \
  Inst *i = Inst_Append(block, NEW_EVAL, &val); \
  i->argc = _argc; \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODEGEN_H_ */
