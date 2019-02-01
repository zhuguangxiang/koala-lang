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

#ifdef __cplusplus
extern "C" {
#endif

#define ARG_NIL   0
#define ARG_INT   1
#define ARG_FLOAT 2
#define ARG_BOOL  3
#define ARG_STR   4
#define ARG_UCHAR 5

typedef struct argument {
  int kind;
  union {
    uchar uch;
    int64 ival;
    float64 fval;
    int bval;
    char *str;
  };
} Argument;

typedef struct inst {
  struct list_head link;
  int bytes;
  int argc;
  uint8 op;
  Argument arg;
  int upbytes;  /* break and continue statements */
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
  struct codeblock *next; /* control flow */
  int ret;                /* false, no OP_RET, needs add one */
} CodeBlock;

int OpCode_ArgCount(uint8 op);
char *OpCode_String(uint8 op);

Inst *Inst_New(uint8 op, Argument *val);
void Inst_Free(Inst *i);
Inst *Inst_Append(CodeBlock *b, uint8 op, Argument *val);
Inst *Inst_Append_NoArg(CodeBlock *b, uint8 op);
JmpInst *JmpInst_New(Inst *inst, int type);
void JmpInst_Free(JmpInst *jmp);

CodeBlock *CodeBlock_New(void);
void CodeBlock_Free(CodeBlock *block);
void CodeBlock_Merge(CodeBlock *from, CodeBlock *to);
void CodeBlock_Show(CodeBlock *block);
int CodeBlock_To_RawCode(KImage *image, CodeBlock *block, uint8 **code);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEGEN_H_ */
