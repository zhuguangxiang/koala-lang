
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct object Object;
typedef struct klass Klass;

/*---------------------------------------------------------------------------*/

#define TYPE_NIL    0
#define TYPE_INT    1
#define TYPE_FLOAT  2
#define TYPE_BOOL   3
#define TYPE_OBJECT 4
#define TYPE_CSTR   5

typedef struct value {
	int type;
	Klass *klazz;
	union {
		int64 ival;
		float64 fval;
		int bval;
		Object *ob;
		char *cstr;
	};
} TValue;

/* Constant values */
extern TValue NilValue;
extern TValue TrueValue;
extern TValue FalseValue;

/* Macros to initialize struct value */
#define initnilvalue(v) do { \
	(v)->type = TYPE_NIL; (v)->klazz = NULL; (v)->ival = (int64)0; \
} while (0)

#define setivalue(v, _v) do { \
	(v)->type = TYPE_INT; (v)->ival = (int64)_v; \
} while (0)

#define setfltvalue(v, _v) do { \
	(v)->type = TYPE_FLOAT; (v)->fval = (float64)_v; \
} while (0)

#define setbvalue(v, _v) do { \
	(v)->type = TYPE_BOOL; (v)->bval = (int)_v; \
} while (0)

#define setobjtype(v, _klazz) do { \
	(v)->type = TYPE_OBJECT; (v)->klazz = (_klazz); \
} while (0)

#define setobjvalue(v, _v) do { \
	setobjtype(v, ((Object *)_v)->ob_klass); \
	(v)->ob = (Object *)_v; \
} while (0)

#define setcstrvalue(v, _v) do { \
	(v)->type = TYPE_CSTR; (v)->cstr = (void *)_v; \
} while (0)

#define setkstrvalue(v, _v) do { \
	(v)->type = TYPE_OBJECT; (v)->ob = String_New(_v); \
} while (0)

#define NIL_VALUE_INIT()      {.type = TYPE_NIL,    .ival = 0}
#define INT_VALUE_INIT(v)     {.type = TYPE_INT,    .ival = (v)}
#define FLOAT_VALUE_INIT(v)   {.type = TYPE_FLOAT,  .fval = (v)}
#define BOOL_VALUE_INIT(v)    {.type = TYPE_BOOL,   .bval = (v)}
#define OBJECT_VALUE_INIT(v)  {.type = TYPE_OBJECT, .ob   = (v)}
#define CSTR_VALUE_INIT(v)    {.type = TYPE_CSTR,   .cstr = (v)}

/* Macros to test type */
#define VALUE_ISNIL(v)      (VALUE_TYPE(v) == TYPE_NIL)
#define VALUE_ISINT(v)      (VALUE_TYPE(v) == TYPE_INT)
#define VALUE_ISFLOAT(v)    (VALUE_TYPE(v) == TYPE_FLOAT)
#define VALUE_ISBOOL(v)     (VALUE_TYPE(v) == TYPE_BOOL)
#define VALUE_ISOBJECT(v)   (VALUE_TYPE(v) == TYPE_OBJECT)
#define VALUE_ISSTRING(v)   (OB_CHECK_KLASS(VALUE_OBJECT(v), String_Klass))
#define VALUE_ISCSTR(v)     (VALUE_TYPE(v) == TYPE_CSTR)

/* Assert for TValue */
#define VALUE_ASSERT(v)         (assert(!VALUE_ISNIL(v)))
#define VALUE_ASSERT_INT(v)     (assert(VALUE_ISINT(v)))
#define VALUE_ASSERT_FLOAT(v)   (assert(VALUE_ISFLOAT(v)))
#define VALUE_ASSERT_BOOL(v)    (assert(VALUE_ISBOOL(v)))
#define VALUE_ASSERT_OBJECT(v)  (assert(VALUE_ISOBJECT(v)))
#define VALUE_ASSERT_STRING(v)  (assert(VALUE_ISSTRING(v)))
#define VALUE_ASSERT_CSTR(v)    (assert(VALUE_ISCSTR(v)))

/* Macros to access type & values */
#define VALUE_TYPE(v)   ((v)->type)
#define VALUE_INT(v)    (VALUE_ASSERT_INT(v), (v)->ival)
#define VALUE_FLOAT(v)  (VALUE_ASSERT_FLOAT(v), (v)->fval)
#define VALUE_BOOL(v)   (VALUE_ASSERT_BOOL(v), (v)->bval)
#define VALUE_OBJECT(v) (VALUE_ASSERT_OBJECT(v), (v)->ob)
#define VALUE_STRING(v) (VALUE_ASSERT_STRING(v), (v)->ob)
#define VALUE_CSTR(v)   (VALUE_ASSERT_CSTR(v), (v)->cstr)

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
	.ob_next = NULL, .ob_ref = 1, .ob_klass = (klazz), \
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
	o->ob_next = NULL; \
	o->ob_ref = 1; \
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

#define FLAGS_FINAL (1 << 0)
#define FLAGS_TRAIT (1 << 1)

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
	Vector traits;
	STable stbl;
};

extern Klass Klass_Klass;
extern Klass Any_Klass;
Klass *Klass_New(char *name, Klass *base, int flags, Vector *traits);
void Fini_Klass(Klass *klazz);
int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc);
Symbol *Klass_Get_Symbol(Klass *klazz, char *name);
int Klass_Add_Method(Klass *klazz, char *name, Proto *proto, Object *code);
Object *Klass_Get_Method(Klass *klazz, char *name);
#define Klass_STable(ob) (&((Klass *)(ob))->stbl)
void Check_Klass(Klass *klazz);
Klass *Trait_New(char *name, Vector *traits);
int Klass_Add_IMethod(Klass *klazz, char *name, Proto *proto);

/*-------------------------------------------------------------------------*/
/*
	All functions's proto, including c function and koala function
	'args' and 'return' are both Tuple.
 */
typedef Object *(*cfunc)(Object *ob, Object *args);

typedef struct funcdef {
	char *name;
	int rsz;
	char *rdesc;
	int psz;
	char *pdesc;
	cfunc fn;
} FuncDef;

int Klass_Add_CFunctions(Klass *klazz, FuncDef *funcs);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
