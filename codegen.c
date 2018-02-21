
#include "parser.h"
#include "buffer.h"
#include "opcode.h"
#include "log.h"

static void __gen_code_fn(Symbol *sym, void *arg)
{
	KImage *image = arg;
	switch (sym->kind) {
		case SYM_VAR: {
			debug("%s %s:", sym->access & ACCESS_CONST ? "const" : "var", sym->name);
			if (sym->access & ACCESS_CONST)
				KImage_Add_Const(image, sym->name, sym->desc);
			else
				KImage_Add_Var(image, sym->name, sym->desc);
			break;
		}
		case SYM_PROTO: {
			debug("func %s:", sym->name);
			CodeBlock *b = sym->ptr;
			int locvars = sym->locvars;
			Inst_Append(b, OP_RET, NULL);
			AtomTable *atbl = image->table;
			Buffer buf;
			Buffer_Init(&buf, 32);
			Inst *i;
			list_for_each_entry(i, &b->insts, link) {
				Inst_Gen(atbl, &buf, i);
			}
			uint8 *data = Buffer_RawData(&buf);
			int size = Buffer_Size(&buf);
			Code_Show(data, size);
			Buffer_Fini(&buf);
			KImage_Add_Func(image, sym->name, sym->desc->proto, locvars, data, size);
			break;
		}
		default: {
			assertm(0, "unknown symbol kind:%d", sym->kind);
		}
	}
}

KImage *Generate_Image(ParserState *ps)
{
	printf("----------generate image--------------------\n");
	ParserUnit *u = ps->u;
	KImage *image = KImage_New(ps->package);
	STbl_Traverse(&u->stbl, __gen_code_fn, image);
	KImage_Finish(image);
	KImage_Show(image);
	printf("----------generate image end----------------\n");
	return image;
}
