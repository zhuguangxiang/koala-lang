
#ifndef _KOALA_CODEOBJECT_H_
#define _KOALA_CODEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

#define CODE_KLANG  0
#define CODE_CLANG  1

typedef struct codeobject {
	OBJECT_HEAD
	int flags;
	TypeDesc *proto;
	union {
		cfunc cf;
		struct {
			Object *consts;   /* for const access, not free it */
			Vector locvec;    /* local variables */
			int locvars;
			int size;
			uint8 *codes;
		} kf;
	};
} CodeObject;

/* Exported APIs */
extern Klass Code_Klass;
#define OBJ_TO_CODE(ob) OB_TYPE_OF(ob, CodeObject, Code_Klass)
Object *KFunc_New(int locvars, uint8 *codes, int size, TypeDesc *proto);
Object *CFunc_New(cfunc cf, TypeDesc *proto);
void CodeObject_Free(Object *ob);
#define CODE_ISKFUNC(code)  (((CodeObject *)(code))->flags == CODE_KLANG)
#define CODE_ISCFUNC(code)  (((CodeObject *)(code))->flags == CODE_CLANG)
// FIXME
static inline int Func_Argc(Object *ob)
{
	CodeObject *code = (CodeObject *)ob;
	TypeDesc *desc = code->proto;
	assert(desc->kind == TYPE_PROTO);
	int sz = Vector_Size(desc->proto.arg);
	if (sz <= 0) return 0;

	desc = Vector_Get(desc->proto.arg, sz - 1);
	if (Type_IsVarg(desc)) {
		return 32;
	} else {
		return sz;
	}
}
int KFunc_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int pos);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEOBJECT_H_ */
