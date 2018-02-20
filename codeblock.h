
#ifndef _KOALA_CODEBLOCK_H_
#define _KOALA_CODEBLOCK_H_

#include "object.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct inst {
	struct list_head link;
	uint8 op;
	TValue arg;
} Inst;

typedef struct codeblock {
	char *name; /* for debugging */
	struct list_head link;
	STable stbl;
	struct list_head insts;
	 /* true if a OP_RET opcode is inserted. */
	int bret;
} CodeBlock;

Inst *Inst_New(uint8 op, TValue *val);
void Inst_Append(CodeBlock *b, uint8 op, TValue *val);
void Inst_Gen(AtomTable *atbl, Buffer *buf, Inst *i);
CodeBlock *CodeBlock_New(AtomTable *atbl);
void CodeBlock_Free(CodeBlock *b);
void CodeBlock_Show(CodeBlock *block);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEBLOCK_H_ */
