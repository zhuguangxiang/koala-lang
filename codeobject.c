
#include "codeobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"

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
	OB_ASSERT_KLASS(ob, Code_Klass);
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

/*-------------------------------------------------------------------------*/

Klass Code_Klass = {
	OBJECT_HEAD_INIT(&Klass_Klass),
	.name  = "Code",
	.bsize = sizeof(CodeObject),
};
