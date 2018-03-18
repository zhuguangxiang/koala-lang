
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
	union {
		cfunc cf;
		struct {
			AtomTable *atbl;  /* for const access, not free it */
			Proto *proto;     /* for runtime to check args */
			Vector locvec;    /* local variables */
			int locvars;
			int size;
			uint8 *codes;
		} kf;
	};
} CodeObject;

/* Exported APIs */
extern Klass Code_Klass;
Object *KFunc_New(int locvars, uint8 *codes, int size);
Object *CFunc_New(cfunc cf);
void CodeObject_Free(Object *ob);
#define CODE_ISKFUNC(code)  (((CodeObject *)(code))->flags == CODE_KLANG)
#define CODE_ISCFUNC(code)  (((CodeObject *)(code))->flags == CODE_CLANG)
static inline int KFunc_Argc(Object *ob)
{
	CodeObject *code = (CodeObject *)ob;
	Proto *p = code->kf.proto;
	if (p->psz <= 0) return 0;

	TypeDesc *desc = p->pdesc + p->psz - 1;
	if (desc->varg) {
		return 32;
	} else {
		return p->psz;
	}
}
int KFunc_Add_LocVar(Object *ob, char *name, TypeDesc *desc, int pos);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEOBJECT_H_ */
