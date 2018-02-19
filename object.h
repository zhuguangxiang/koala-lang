
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct object Object;
typedef struct klass Klass;

/*-------------------------------------------------------------------------*/

#define TYPE_NIL    0
#define TYPE_INT    1
#define TYPE_FLOAT  2
#define TYPE_BOOL   3
#define TYPE_OBJECT 4
#define TYPE_CSTR   5

typedef struct value {
	int type;
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
	(v)->type = TYPE_NIL; (v)->ival = (int64)0; \
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

#define setobjvalue(v, _v) do { \
	(v)->type = TYPE_OBJECT; (v)->ob = (Object *)_v; \
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

/* Macros to access type & values */
#define VALUE_TYPE(v)   ((v)->type)
#define VALUE_INT(v)    (assert(VALUE_ISINT(v)), (v)->ival)
#define VALUE_FLOAT(v)  (assert(VALUE_ISFLOAT(v)), (v)->fval)
#define VALUE_BOOL(v)   (assert(VALUE_ISBOOL(v)), (v)->bval)
#define VALUE_OBJECT(v) (assert(VALUE_ISOBJECT(v)), (v)->ob)
#define VALUE_STRING(v) (assert(VALUE_ISSTRING(v)), (v)->ob)
#define VALUE_CSTR(v)   (assert(VALUE_ISCSTR(v)), (v)->cstr)

/* Assert for TValue */
#define VALUE_ASSERT(v)         (assert(!VALUE_ISNIL(v)))
#define VALUE_ASSERT_INT(v)     (assert(VALUE_ISINT(v)))
#define VALUE_ASSERT_FLOAT(v)   (assert(VALUE_ISFLOAT(v)))
#define VALUE_ASSERT_BOOL(v)    (assert(VALUE_ISBOOL(v)))
#define VALUE_ASSERT_OBJECT(v)  (assert(VALUE_ISOBJECT(v)))
#define VALUE_ASSERT_STRING(v)  (assert(VALUE_ISSTRING(v)))
#define VALUE_ASSERT_CSTR(v)    (assert(VALUE_ISCSTR(v)))

/* TValue utils's functions */
TValue Va_Build_Value(char ch, va_list *ap);
TValue TValue_Build(char ch, ...);
int Va_Parse_Value(TValue *val, char ch, va_list *ap);
int TValue_Parse(TValue *val, char ch, ...);
int TValue_Print(char *buf, int sz, TValue *val);

/*-------------------------------------------------------------------------*/

#define OBJECT_HEAD \
	Object *ob_next; int ob_ref; Klass *ob_klass;

struct object {
	OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(klazz) \
	.ob_next = NULL, .ob_ref = 1, .ob_klass = klazz

#define init_object_head(ob, klazz) do { \
	Object *o = (Object *)ob; \
	o->ob_next = NULL; o->ob_ref = 1; o->ob_klass = klazz; \
} while (0)

#define OB_KLASS(ob) (((Object *)(ob))->ob_klass)
#define OB_ASSERT_KLASS(ob, klazz)  assert(OB_KLASS(ob) == &(klazz))
#define OB_KLASS_EQUAL(ob1, ob2)    (OB_KLASS(ob1) == OB_KLASS(ob2))
#define OB_CHECK_KLASS(ob, klazz)   (OB_KLASS(ob) == &(klazz))
#define KLASS_ASSERT(klazz, expected) assert(((Klass *)klazz) == &(expected))
#define OB_TYPE_OF(ob, ctype, klazz) \
	(OB_ASSERT_KLASS(ob, klazz), ((ctype *)ob))

/*-------------------------------------------------------------------------*/

typedef void (*markfunc)(Object *ob);

typedef Object *(*allocfunc)(Klass *klazz, int nr);
typedef void (*freefunc)(Object *ob);

typedef uint32 (*hashfunc)(TValue *v);
typedef int (*cmpfunc)(TValue *v1, TValue *v2);

typedef Object *(*strfunc)(TValue *v);

struct klass {
	OBJECT_HEAD
	char *name;
	int bsize;
	int isize;

	markfunc ob_mark;

	allocfunc ob_alloc;
	freefunc ob_free;

	hashfunc ob_hash;
	cmpfunc ob_cmp;

	strfunc ob_tostr;

	STable stbl;
};

extern Klass Klass_Klass;
Klass *Klass_New(char *name, int bsize, int isize, Klass *parent);
void Fini_Klass(Klass *klazz);
int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc);
int Klass_Add_Method(Klass *klazz, char *name, Proto *proto, Object *code);
Object *Klass_Get_Method(Klass *klazz, char *name);
#define Klass_STable(ob) (&((Klass *)(ob))->stbl)

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
