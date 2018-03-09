
#ifndef _KOALA_TYPE_H_
#define _KOALA_TYPE_H_

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_PRIMITIVE  1
#define TYPE_USERDEF    2
#define TYPE_PROTO      3

#define PRIMITIVE_INT     'i'
#define PRIMITIVE_FLOAT   'f'
#define PRIMITIVE_BOOL    'b'
#define PRIMITIVE_STRING  's'
#define PRIMITIVE_ANY     'A'

/* Type's descriptor */
typedef struct typedesc {
	int8 kind;
	int8 varg;
	int16 dims;
	union {
		char primitive;
		struct {
			char *path;
			char *type;
		};
		void *proto;
	};
} TypeDesc;

typedef struct proto {
	int16 rsz;
	int16 psz;
	TypeDesc *rdesc;
	TypeDesc *pdesc;
} Proto;

#define Init_Primitive_TypeDesc(desc, d, p) do { \
	(desc)->varg = 0; \
	(desc)->dims = (d); \
	(desc)->kind = TYPE_PRIMITIVE; \
	(desc)->primitive = (p); \
} while (0)
#define Init_UserDef_TypeDesc(desc, d, p, t) do { \
	(desc)->varg = 0; \
	(desc)->dims = (d); \
	(desc)->kind = TYPE_USERDEF; \
	(desc)->path = (p); \
	(desc)->type = (t); \
} while (0)
#define Init_Proto_TypeDesc(desc, d, p) do { \
	(desc)->varg = 0; \
	(desc)->dims = (d); \
	(desc)->kind = TYPE_PROTO; \
	(dec)->proto = (p); \
} while (0)
TypeDesc *TypeDesc_New(int kind);
void TypeDesc_Free(TypeDesc *desc);
TypeDesc *TypeDesc_From_Primitive(int primitive);
TypeDesc *TypeDesc_From_UserDef(char *path, char *type);
TypeDesc *TypeDesc_From_Proto(Proto *proto);
TypeDesc *TypeDesc_From_FuncType(Vector *rvec, Vector *pvec);
void FullPath_To_TypeDesc(char *fullpath, int len, TypeDesc *desc);
int TypeVec_ToTypeArray(Vector *vec, TypeDesc **desc);
int TypeDesc_Check(TypeDesc *t1, TypeDesc *t2);
char *TypeDesc_ToString(TypeDesc *desc);

Proto *Proto_New(int rsz, char *rdesc, int psz, char *pdesc);
void Proto_Free(Proto *proto);
int Init_Proto(Proto *proto, int rsz, char *rdesc, int psz, char *pdesc);
void Fini_Proto(Proto *proto);
int Proto_Has_Vargs(Proto *proto);
Proto *Proto_Dup(Proto *proto);
int TypeDesc_IsBool(TypeDesc *desc);
char *Primitive_ToString(int type);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TYPE_H_ */
