
#include "codeblock.h"
#include "opcode.h"
#include "log.h"

Inst *Inst_New(uint8 op, TValue *val)
{
	Inst *i = malloc(sizeof(Inst));
	init_list_head(&i->link);
	i->op = op;
	if (val)
		i->arg = *val;
	else
		initnilvalue(&i->arg);
	return i;
}

void Inst_Free(Inst *i)
{
	free(i);
}

void Inst_Append(CodeBlock *b, uint8 op, TValue *val)
{
	char buf[32];
	TValue_Print(buf, 32, val);
	debug("inst:'%s %s'", OPCode_ToString(op), buf);
	Inst *i = Inst_New(op, val);
	list_add_tail(&i->link, &b->insts);
}

void Inst_Gen(AtomTable *atbl, Buffer *buf, Inst *i)
{
	int index = -1;
	Buffer_Write_Byte(buf, i->op);
	switch (i->op) {
		case OP_HALT: {
			break;
		}
		case OP_LOADK: {
			TValue *val = &i->arg;
			if (VALUE_ISINT(val)) {
				index = ConstItem_Set_Int(atbl, VALUE_INT(val));
			} else if (VALUE_ISFLOAT(val)) {
				index = ConstItem_Set_Float(atbl, VALUE_FLOAT(val));
			} else if (VALUE_ISBOOL(val)) {
				index = ConstItem_Set_Bool(atbl, VALUE_BOOL(val));
			} else if (VALUE_ISCSTR(val)) {
				index = ConstItem_Set_String(atbl, VALUE_CSTR(val));
			} else {
				assert(0);
			}
			Buffer_Write_4Bytes(buf, index);
			break;
		}
		case OP_LOADM: {
			index = ConstItem_Set_String(atbl, i->arg.cstr);
			Buffer_Write_4Bytes(buf, index);
			break;
		}
		case OP_LOAD: {
			Buffer_Write_2Bytes(buf, i->arg.ival);
			break;
		}
		case OP_STORE: {
			Buffer_Write_2Bytes(buf, i->arg.ival);
			break;
		}
		case OP_GETFIELD: {
			index = ConstItem_Set_String(atbl, i->arg.cstr);
			Buffer_Write_4Bytes(buf, index);
			break;
		}
		case OP_SETFIELD: {
			index = ConstItem_Set_String(atbl, i->arg.cstr);
			Buffer_Write_4Bytes(buf, index);
			break;
		}
		case OP_CALL: {
			index = ConstItem_Set_String(atbl, i->arg.cstr);
			Buffer_Write_4Bytes(buf, index);
			break;
		}
		case OP_RET: {
			break;
		}
		case OP_ADD:
		case OP_SUB: {
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

CodeBlock *CodeBlock_New(AtomTable *atbl)
{
	CodeBlock *b = calloc(1, sizeof(CodeBlock));
	init_list_head(&b->link);
	STbl_Init(&b->stbl, atbl);
	init_list_head(&b->insts);
	return b;
}

void CodeBlock_Free(CodeBlock *b)
{
	if (!b) return;
	assert(list_unlinked(&b->link));
	STbl_Fini(&b->stbl);

	Inst *i, *n;
	list_for_each_entry_safe(i, n, &b->insts, link) {
		list_del(&i->link);
		Inst_Free(i);
	}

	free(b);
}

void CodeBlock_Show(CodeBlock *block)
{
	if (!block) return;

	char buf[64];

	printf("---------CodeBlock_Show---------------\n");
	if (!list_empty(&block->insts)) {
		Inst *i;
		list_for_each_entry(i, &block->insts, link) {
			printf("opcode:%s\n", OPCode_ToString(i->op));
			TValue_Print(buf, sizeof(buf), &i->arg);
			printf("arg:%s\n", buf);
			printf("-----------------------\n");
		}
	}
	printf("--------CodeBlock_Show End------------\n");
}
