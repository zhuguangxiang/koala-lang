
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "hashtable.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct object Object;
typedef struct klass Klass;

/*---------------------------------------------------------------------------*/

extern Klass Int_Klass;
extern Klass Float_Klass;
extern Klass Bool_Klass;

typedef struct value {
	Klass *klazz;
	union {
		int64 ival;
		float64 fval;
		int bval;
		Object *ob;
	};
} TValue;

/* Constant values */
extern TValue NilValue;
extern TValue TrueValue;
extern TValue FalseValue;

/* Macros to initialize struct value */
#define initnilvalue(v) do { \
	(v)->klazz = NULL; (v)->ival = (int64)0; \
} while (0)

#define setivalue(v, _v) do { \
	(v)->klazz = &Int_Klass; (v)->ival = (int64)_v; \
} while (0)

#define setfltvalue(v, _v) do { \
	(v)->klazz = &Float_Klass; (v)->fval = (float64)_v; \
} while (0)

#define setbvalue(v, _v) do { \
	(v)->klazz = &Bool_Klass; (v)->bval = (int)_v; \
} while (0)

#define setobjtype(v, _klazz) do { \
	(v)->klazz = (_klazz); \
} while (0)

#define setobjvalue(v, _v) do { \
	Object *obj = (Object *)_v; \
	(v)->klazz = obj->ob_klass; \
	(v)->ob = obj; \
} while (0)

#define NIL_VALUE_INIT()      {.klazz = NULL,         .ival = 0}
#define INT_VALUE_INIT(v)     {.klazz = &Int_Klass,   .ival = (v)}
#define FLOAT_VALUE_INIT(v)   {.klazz = &Float_Klass, .fval = (v)}
#define BOOL_VALUE_INIT(v)    {.klazz = &Bool_Klass,  .bval = (v)}

/* Macros to test type */
#define VALUE_ISNIL(v)     ((v)->klazz == NULL)
#define VALUE_ISINT(v)     ((v)->klazz == &Int_Klass)
#define VALUE_ISFLOAT(v)   ((v)->klazz == &Float_Klass)
#define VALUE_ISBOOL(v)    ((v)->klazz == &Bool_Klass)

/* Assert for TValue */
#define VALUE_ASSERT(v)         (assert(!VALUE_ISNIL(v)))
#define VALUE_ASSERT_INT(v)     (assert(VALUE_ISINT(v)))
#define VALUE_ASSERT_FLOAT(v)   (assert(VALUE_ISFLOAT(v)))
#define VALUE_ASSERT_BOOL(v)    (assert(VALUE_ISBOOL(v)))

/* Macros to access type & values */
#define VALUE_INT(v)    (VALUE_ASSERT_INT(v), (v)->ival)
#define VALUE_FLOAT(v)  (VALUE_ASSERT_FLOAT(v), (v)->fval)
#define VALUE_BOOL(v)   (VALUE_ASSERT_BOOL(v), (v)->bval)

/* TValue utils's functions */
TValue Va_Build_Value(char ch, va_list *ap);
TValue TValue_Build(int ch, ...);
int Va_Parse_Value(TValue *val, char ch, va_list *ap);
int TValue_Parse(TValue *val, int ch, ...);
int TValue_Print(char *buf, int sz, TValue *val, int escape);
void TValue_Set_TypeDesc(TValue *val, TypeDesc *desc, Object *module);
void TValue_Set_Value(TValue *val, TValue *v);
int TValue_Check(TValue *v1, TValue *v2);
int TValue_Check_TypeDesc(TValue *val, TypeDesc *desc);

/*---------------------------------------------------------------------------*/

#define OBJECT_HEAD \
	Object *ob_next; int ob_ref; Klass *ob_klass; \
	Object *ob_base; Object *ob_head; int ob_size;

struct object {
	OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(ob, klazz) \
	.ob_klass = (klazz), \
	.ob_base = (Object *)(ob), .ob_head = (Object *)(ob), .ob_size = 0,

#define OB_KLASS(ob) (((Object *)(ob))->ob_klass)
#define OB_CHECK_KLASS(ob, klazz) (OB_KLASS(ob) == &(klazz))
#define OB_ASSERT_KLASS(ob, klazz) assert(OB_CHECK_KLASS(ob, klazz))
#define OB_TYPE_OF(ob, ctype, klazz) \
	(OB_ASSERT_KLASS(ob, klazz), (ctype *)(ob))
#define OB_Head(ob) (((Object *)(ob))->ob_head)
#define OB_Base(ob) (((Object *)(ob))->ob_base)
#define OB_HasBase(ob) (OB_Base(ob) != (Object *)(ob))

#define OBJECT_SIZE(klazz) \
	((klazz)->basesize + sizeof(TValue) * (klazz)->itemsize)

#define NEXT_OBJECT(ob, klazz) \
	(Object *)((char *)ob + OBJECT_SIZE(klazz))

#define Init_Object_Head(ob, klazz) do { \
	Object *o = (Object *)(ob); \
	o->ob_klass = klazz; \
	o->ob_base = OB_HasBase(klazz) ? NEXT_OBJECT(ob, klazz) : o; \
	o->ob_head = o; \
	o->ob_size = (klazz)->itemsize; \
} while (0)

TValue Object_Get_Value(Object *ob, char *name);
int Object_Set_Value(Object *ob, char *name, TValue *val);
Object *Object_Get_Method(Object *ob, char *name, Object **rob);

/*---------------------------------------------------------------------------*/

typedef void (*markfunc)(Object *ob);
typedef Object *(*allocfunc)(Klass *klazz);
typedef void (*initfunc)(Object *ob);
typedef void (*freefunc)(Object *ob);
typedef uint32 (*hashfunc)(TValue *v);
typedef int (*equalfunc)(TValue *v1, TValue *v2);
typedef Object *(*strfunc)(TValue *v);

typedef struct numberfunctions {
	TValue (*add)(TValue *, TValue *);

} NumberFunctions;

struct klass {
	OBJECT_HEAD
	char *name;
	int basesize;
	int itemsize; //number of TValues
	int flags;
	Object *module;
	markfunc ob_mark;
	allocfunc ob_alloc;
	initfunc ob_init;
	freefunc ob_free;
	hashfunc ob_hash;
	equalfunc ob_equal;
	strfunc ob_tostr;
	NumberFunctions *numops;
	Vector traits;
	// STable stbl;
	Object *consts;
	HashTable *table;
	int varcnt;
	Vector lines;
};

extern Klass Klass_Klass;
extern Klass Any_Klass;
extern Klass Trait_Klass;
Klass *Klass_New(char *name, Klass *base, Vector *traits, Klass *type);
#define Class_New(name, base, traits) \
	Klass_New(name, base, traits, &Klass_Klass)
#define Trait_New(name, traits) \
	Klass_New(name, NULL, traits, &Trait_Klass)
void Fini_Klass(Klass *klazz);
int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc);
int Klass_Add_Method(Klass *klazz, char *name, Object *code);
Object *Klass_Get_Method(Klass *klazz, char *name, Klass **trait);
void Check_Klass(Klass *klazz);
int Klass_Add_Proto(Klass *klazz, char *name, TypeDesc *proto);

/*-------------------------------------------------------------------------*/
/*
	All functions's proto, including c function and koala function
	'args' and 'return' are both Tuple.
 */
typedef Object *(*cfunc)(Object *ob, Object *args);

typedef struct funcdef {
	char *name;
	char *rdesc;
	char *pdesc;
	cfunc fn;
} FuncDef;

int Klass_Add_CFunctions(Klass *klazz, FuncDef *funcs);

#define MEMBER_VAR    1
#define MEMBER_CODE   2
#define MEMBER_PROTO  3
#define MEMBER_CLASS  4
#define MEMBER_TRAIT  5

typedef struct memberdef {
	HashNode hnode;
	int kind;
	char *name;
	TypeDesc *desc;
	int bconst;
	union {
		int offset;
		Object *code;
		Klass *klazz;
	};
} MemberDef;

MemberDef *Member_New(int kind, char *name, TypeDesc *desc, int bconst);
void Member_Free(MemberDef *m);
uint32 Member_Hash(MemberDef *m);
int Member_Equal(MemberDef *m1, MemberDef *m2);

#define Member_Var_New(name, desc, bconst) \
	Member_New(MEMBER_VAR, name, desc, bconst)
#define Member_Code_New(name, desc) \
	Member_New(MEMBER_CODE, name, desc, 0)
#define Member_Proto_New(name, desc) \
	Member_New(MEMBER_PROTO, name, desc, 0)
#define Member_Class_New(name) \
	Member_New(MEMBER_CLASS, name, NULL, 0)
#define Member_Trait_New(name) \
	Member_New(MEMBER_TRAIT, name, NULL, 0)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
