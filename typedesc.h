
#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_PRIMITIVE 1
#define TYPE_USRDEF    2
#define TYPE_PROTO     3
#define TYPE_ARRAY     4
#define TYPE_MAP       5

#define PRIMITIVE_BYTE   'b'
#define PRIMITIVE_CHAR   'c'
#define PRIMITIVE_INT    'i'
#define PRIMITIVE_FLOAT  'f'
#define PRIMITIVE_BOOL   'z'
#define PRIMITIVE_STRING 's'
#define PRIMITIVE_ANY    'A'
#define PRIMITIVE_VARG   'v'

/* Type's descriptor */
typedef struct typedesc TypeDesc;

struct typedesc {
	int kind;
	union {
		int primitive;
		struct {
			char *path;
			char *type;
		} usrdef;
		struct {
			Vector *arg;
			Vector *ret;
		} proto;
		struct {
			TypeDesc *key;
			TypeDesc *val;
		} map;
		struct {
			int dims;
			TypeDesc *base;
		} array;
	};
};

extern TypeDesc ByteType;
extern TypeDesc CharType;
extern TypeDesc IntType;
extern TypeDesc FloatType;
extern TypeDesc BoolType;
extern TypeDesc StringType;
extern TypeDesc AnyType;
extern TypeDesc VargType;

#define Init_Type_UsrDef(desc, p, t) do { \
	(desc)->kind = TYPE_USRDEF; \
	(desc)->usrdef.path = (p); \
	(desc)->usrdef.type = (t); \
} while (0)

void Type_Free(TypeDesc *desc);
TypeDesc *Type_Dup(TypeDesc *desc);
TypeDesc *Type_New_UsrDef(char *path, char *type);
TypeDesc *Type_New_Proto(Vector *arg, Vector *ret);
TypeDesc *Type_New_Map(TypeDesc *key, TypeDesc *val);
TypeDesc *Type_New_Array(int dims, TypeDesc *base);
int TypeList_Equal(Vector *v1, Vector *v2);
int Type_Equal(TypeDesc *t1, TypeDesc *t2);
void Type_ToString(TypeDesc *desc, char *buf);
Vector *CString_To_TypeList(char *str);

#define Type_Byte    &ByteType
#define Type_Integer &IntType
#define Type_Float   &FloatType
#define Type_Bool    &BoolType
#define Type_String  &StringType
#define Type_Any     &AnyType
#define Type_Varg    &VargType

#define Type_IsInt(type)    ((type) == &IntType)
#define Type_IsFloat(type)  ((type) == &FloatType)
#define Type_IsBool(type)   ((type) == &BoolType)
#define Type_IsString(type) ((type) == &StringType)
#define Type_IsVarg(type)   ((type) == &VargType)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TYPEDESC_H_ */
