
#include "codeobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"
#include "log.h"

static CodeObject *codeobject_new(int flags)
{
	CodeObject *code = calloc(1, sizeof(CodeObject));
	init_object_head(code, &Code_Klass);
	code->flags = flags;
	return code;
}

Object *CFunc_New(cfunc cf)
{
	CodeObject *code = codeobject_new(CODE_CLANG);
	code->cf = cf;
	return (Object *)code;
}

void CodeObject_Free(Object *ob)
{
	CodeObject *code = OB_TYPE_OF(ob, CodeObject, Code_Klass);
	if (CODE_ISKFUNC(code)) {
		//FIXME
		//Proto_Free(code->kf.proto);
	}
	free(ob);
}

Object *KFunc_New(int locvars, uint8 *codes, int size)
{
	CodeObject *code = codeobject_new(CODE_KLANG);
	code->kf.locvars = locvars;
	code->kf.codes = codes;
	code->kf.size = size;
	return (Object *)code;
}

int KFunc_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int pos)
{
	CodeObject *code = OB_TYPE_OF(ob, CodeObject, Code_Klass);
	if (CODE_ISKFUNC(code)) {
		Symbol *sym = Symbol_New(SYM_VAR);
		int32 idx = StringItem_Set(code->kf.atbl, name);
		assert(idx >= 0);
		sym->nameidx = idx;
		sym->name = strdup(name);

		idx = -1;
		if (desc) {
			idx = TypeItem_Set(code->kf.atbl, desc);
			assert(idx >= 0);
		}
		sym->descidx = idx;
		sym->desc = desc;
		sym->index = pos;

		Vector_Append(&code->kf.locvec, sym);

		return 0;
	} else {
		error("add locvar to cfunc?");
		return -1;
	}
}

/*-------------------------------------------------------------------------*/

Klass Code_Klass = {
	OBJECT_HEAD_INIT(&Klass_Klass),
	.name = "Code",
	.size = sizeof(CodeObject),
};
