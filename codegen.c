
#include "codegen.h"
#include "opcode.h"
#include "log.h"

Inst *Inst_New(uint8 op, TValue *val)
{
	Inst *i = malloc(sizeof(Inst));
	init_list_head(&i->link);
	i->op = op;
	i->bytes = 1 + opcode_argsize(op);
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

Inst *Inst_Append(CodeBlock *b, uint8 op, TValue *val)
{
	char buf[64];
	TValue_Print(buf, 32, val, 0);
	debug("inst:'%s %s'", opcode_string(op), buf);
	Inst *i = Inst_New(op, val);
	list_add_tail(&i->link, &b->insts);
	b->bytes += i->bytes;
	i->upbytes = b->bytes;
	return i;
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
		case OP_CALL:
		case OP_NEW: {
			index = ConstItem_Set_String(atbl, i->arg.cstr);
			Buffer_Write_4Bytes(buf, index);
			Buffer_Write_2Bytes(buf, i->argc);
			break;
		}
		case OP_RET: {
			break;
		}
		case OP_ADD:
		case OP_SUB: {
			break;
		}
		case OP_GT:
		case OP_GE:
		case OP_LT:
		case OP_LE:
		case OP_EQ:
		case OP_NEQ: {
			break;
		}
		case OP_JUMP:
		case OP_JUMP_TRUE:
		case OP_JUMP_FALSE: {
			Buffer_Write_4Bytes(buf, i->arg.ival);
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

struct gencode_struct {
	int bcls;
	KImage *image;
	char *clazz;
};

static void __gen_code_fn(Symbol *sym, void *arg)
{
	struct gencode_struct *tmp = arg;
	switch (sym->kind) {
		case SYM_VAR: {
			if (tmp->bcls) {
				debug("add var '%s' into class", sym->name);
				KImage_Add_Field(tmp->image, tmp->clazz, sym->name, sym->desc);
			} else {
				debug("%s %s:", sym->access & ACCESS_CONST ? "const" : "var",
					sym->name);
				if (sym->access & ACCESS_CONST)
					KImage_Add_Const(tmp->image, sym->name, sym->desc);
				else
					KImage_Add_Var(tmp->image, sym->name, sym->desc);
			}
			break;
		}
		case SYM_PROTO: {
			debug(">>>> func %s:", sym->name);
			CodeBlock *b = sym->ptr;
			int locvars = sym->locvars;
			AtomTable *atbl = tmp->image->table;

			Buffer buf;
			Buffer_Init(&buf, 32);
			Inst *i;
			list_for_each_entry(i, &b->insts, link) {
				Inst_Gen(atbl, &buf, i);
			}

			//FIXME:
			if (!strcmp(sym->name, "__init__")) {
				//load self
				TValue val = INT_VALUE_INIT(0);
				Inst *iret = Inst_New(OP_LOAD, &val);
				Inst_Gen(atbl, &buf, iret);
			}

			Inst *iret = Inst_New(OP_RET, NULL);
			Inst_Gen(atbl, &buf, iret);

			uint8 *data = Buffer_RawData(&buf);
			int size = Buffer_Size(&buf);
			code_show(data, size);
			Buffer_Fini(&buf);

			if (tmp->bcls) {
				KImage_Add_Method(tmp->image, tmp->clazz, sym->name,
					sym->desc->proto, locvars, data, size);
			} else {
				KImage_Add_Func(tmp->image, sym->name, sym->desc->proto, locvars,
					data, size);
			}
			break;
		}
		case SYM_CLASS: {
			debug("class %s:", sym->name);
			KImage_Add_Class(tmp->image, sym->name, NULL, NULL);
			struct gencode_struct tmp2 = {1, tmp->image, sym->name};
			STbl_Traverse(sym->ptr, __gen_code_fn, &tmp2);
			break;
		}
		default: {
			assertm(0, "unknown symbol kind:%d", sym->kind);
		}
	}
}

void codegen_klc(ParserState *ps, char *out)
{
	printf("----------codegen------------\n");
	KImage *image = KImage_New(ps->package);
	struct gencode_struct tmp = {0, image, NULL};
	STbl_Traverse(ps->stbl, __gen_code_fn, &tmp);
	KImage_Finish(image);
	KImage_Show(image);
	KImage_Write_File(image, out);
	printf("----------codegen end--------\n");
}

void codegen_binary(ParserState *ps, int op)
{
	switch (op) {
		case BINARY_ADD: {
			debug("add 'OP_ADD'");
			Inst_Append(ps->u->block, OP_ADD, NULL);
			break;
		}
		case BINARY_SUB: {
			debug("add 'OP_SUB'");
			Inst_Append(ps->u->block, OP_SUB, NULL);
			break;
		}
		case BINARY_GT: {
			debug("add 'OP_GT'");
			Inst_Append(ps->u->block, OP_GT, NULL);
			break;
		}
		case BINARY_LT: {
			debug("add 'OP_LT'");
			Inst_Append(ps->u->block, OP_LT, NULL);
			break;
		}
		case BINARY_EQ: {
			debug("add 'OP_EQ'");
			Inst_Append(ps->u->block, OP_EQ, NULL);
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}
