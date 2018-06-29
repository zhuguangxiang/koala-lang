
#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_PRIME   1
#define TYPE_USRDEF  2
#define TYPE_PROTO   3
#define TYPE_MAP     4
#define TYPE_ARRAY   5

#define PRIME_BYTE   'b'
#define PRIME_CHAR   'c'
#define PRIME_INT    'i'
#define PRIME_FLOAT  'f'
#define PRIME_BOOL   'z'
#define PRIME_STRING 's'
#define PRIME_ANY    'A'
#define PRIME_VARG   'v'

/* Type's descriptor */
typedef struct typedesc TypeDesc;

struct typedesc {
	int kind;
	int refcnt;
	union {
		struct {
			int val;
		} prime;
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

void Type_Free(TypeDesc *desc);
TypeDesc *Type_New_Prime(int prime);
TypeDesc *Type_New_UsrDef(char *path, char *type);
TypeDesc *Type_New_Proto(Vector *arg, Vector *ret);
TypeDesc *Type_New_Map(TypeDesc *key, TypeDesc *val);
TypeDesc *Type_New_Array(int dims, TypeDesc *base);
int TypeList_Equal(Vector *v1, Vector *v2);
int Type_Equal(TypeDesc *t1, TypeDesc *t2);
void Type_ToString(TypeDesc *desc, char *buf);
Vector *String_To_TypeList(char *str);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TYPEDESC_H_ */
