
#ifndef _KOALA_CODEGEN_H_
#define _KOALA_CODEGEN_H_

#include "parser.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

Inst *Inst_New(uint8 op, Argument *val);
void Inst_Free(Inst *i);
Inst *Inst_Append(CodeBlock *b, uint8 op, Argument *val);
Inst *Inst_Append_NoArg(CodeBlock *b, uint8 op);
void Inst_Gen(AtomTable *atbl, Buffer *buf, Inst *i);

void codegen_binary(ParserState *ps, int op);
void codegen_unary(ParserState *ps, int op);
void codegen_klc(PackageInfo *pkg);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEGEN_H_ */
