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

#ifndef _KOALA_CODEGEN_H_
#define _KOALA_CODEGEN_H_

#include "image.h"
#include "opcode.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct inst {
  struct list_head link;
  int bytes;
  int argc;
  uint8 op;
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

Inst *Inst_Append(CodeBlock *b, uint8 op, ConstValue *val);
Inst *Inst_Append_NoArg(CodeBlock *b, uint8 op);
JmpInst *JmpInst_New(Inst *inst, int type);
void JmpInst_Free(JmpInst *jmp);

CodeBlock *CodeBlock_New(void);
void CodeBlock_Free(CodeBlock *block);
void CodeBlock_Merge(CodeBlock *from, CodeBlock *to);
void CodeBlock_Show(CodeBlock *block);
int CodeBlock_To_RawCode(KImage *image, CodeBlock *block, uint8 **code);

#define CODE_DUP(block) \
({ \
  Inst_Append_NoArg(block, DUP); \
})

#define CODE_LOAD_PKG(block, path) \
({ \
  ConstValue val = {.kind = BASE_STRING, .str = path}; \
  Inst_Append(block, LOAD_PKG, &val); \
})

#define CODE_LOAD(block, index) \
({ \
  ConstValue val = {.kind = BASE_INT, .ival = index}; \
  Inst_Append(block, LOAD, &val); \
})

#define CODE_STORE(block, index) \
({ \
  ConstValue val = {.kind = BASE_INT, .ival = index}; \
  Inst_Append(block, STORE, &val); \
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

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEGEN_H_ */
