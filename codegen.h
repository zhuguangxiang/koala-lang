
#ifndef _KOALA_CODEGEN_H_
#define _KOALA_CODEGEN_H_

#include "parser.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

Inst *Inst_New(uint8 op, TValue *val);
void Inst_Free(Inst *i);
void Inst_Add(CodeBlock *b, uint8 op, TValue *val);
void Inst_Append(CodeBlock *b, uint8 op, TValue *val);
void Inst_Gen(AtomTable *atbl, Buffer *buf, Inst *i);

void codegen_binary(ParserState *ps, int op);
void codegen_klc(ParserState *ps, char *out);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEGEN_H_ */
