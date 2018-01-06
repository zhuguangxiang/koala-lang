
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "types.h"
#include "codeformat.h"
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

typedef struct value {
  int type;
  union {
    int64 ival;
    float64 fval;
    int bval;
    struct object *ob;
  };
} TValue;

/* Constant values */
extern TValue NilValue;
extern TValue TrueValue;
extern TValue FalseValue;

/* Macros to initialize struct value */
#define init_nil_value(v) do { \
  (v)->type = TYPE_NIL; (v)->ival = 0; \
} while (0)

#define set_int_value(v, _v) do { \
  (v)->type = TYPE_INT; (v)->ival = _v; \
} while (0)

#define set_float_value(v, _v) do { \
  (v)->type = TYPE_FLOAT; (v)->fval = _v; \
} while (0)

#define set_bool_value(v, _v) do { \
  (v)->type = TYPE_BOOL; (v)->bval = _v; \
} while (0)

#define set_object_value(v, _v) do { \
  (v)->type = TYPE_OBJECT; (v)->ob = _v; \
} while (0)

#define NIL_VALUE_INIT()      {.type = TYPE_NIL,    .ival = 0}
#define INT_VALUE_INIT(v)     {.type = TYPE_INT,    .ival = (v)}
#define FLOAT_VALUE_INIT(v)   {.type = TYPE_FLOAT,  .fval = (v)}
#define BOOL_VALUE_INIT(v)    {.type = TYPE_BOOL,   .bval = (v)}
#define OBJECT_VALUE_INIT(v)  {.type = TYPE_OBJECT, .ob   = (v)}

/* Macros to access type & values */
#define VALUE_TYPE(v)   ((v)->type)
#define VALUE_INT(v)    ((v)->ival)
#define VALUE_FLOAT(v)  ((v)->fval)
#define VALUE_BOOL(v)   ((v)->bval)
#define VALUE_OBJECT(v) ((v)->ob)

/* Macros to test type */
#define VALUE_ISNIL(v)     (VALUE_TYPE(v) == TYPE_NIL)
#define VALUE_ISINT(v)     (VALUE_TYPE(v) == TYPE_INT)
#define VALUE_ISFLOAT(v)   (VALUE_TYPE(v) == TYPE_FLOAT)
#define VALUE_ISBOOL(v)    (VALUE_TYPE(v) == TYPE_BOOL)
#define VALUE_ISOBJECT(v)  (VALUE_TYPE(v) == TYPE_OBJECT)

/* TValue utils's functions */
#define VALUE_ASSERT_TYPE(v, t)  assert(VALUE_TYPE(v) == (t))
int Va_Build_Value(TValue *ret, char ch, va_list *ap);
int TValue_Build(TValue *ret, char ch, ...);
int Va_Parse_Value(TValue *val, char ch, va_list *ap);
int TValue_Parse(TValue *val, char ch, ...);

/*-------------------------------------------------------------------------*/

#define OBJECT_HEAD \
  Object *ob_next; int ob_ref; Klass *ob_klass;

struct object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(klazz) \
  .ob_next = NULL, .ob_ref = 1, .ob_klass = klazz

#define init_object_head(ob, klazz)  do {  \
  Object *o = (Object *)ob;  \
  o->ob_next = NULL; o->ob_ref = 1; o->ob_klass = klazz; \
} while (0)

#define OB_KLASS(ob) (((Object *)(ob))->ob_klass)
#define OB_ASSERT_KLASS(ob, klazz)  assert(OB_KLASS(ob) == &(klazz))
#define OB_KLASS_EQUAL(ob1, ob2)    (OB_KLASS(ob1) == OB_KLASS(ob2))
#define KLASS_ASSERT(klazz, expected) assert(((Klass *)klazz) == &(expected))
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

  HashTable *stable;
  ItemTable *itable;
  uint32 avail_index;
};

extern Klass Klass_Klass;
void Init_Klass_Klass(Object *ob);
Klass *Klass_New(char *name, int bsize, int isize, Klass *parent);
int Klass_Add_Field(Klass *klazz, char *name, char *desc, uint8 access);
int Klass_Add_Method(Klass *klazz, char *name, char *rdesc, char *pdesc,
                     uint8 access, Object *method);
int Klass_Add_IMethod(Klass *klazz, char *name, char *rdesc, char *pdesc,
                     uint8 access);
Symbol *Klass_Get(Klass *klazz, char *name);
Object *Klass_Get_Method(Klass *klazz, char *name);

/*-------------------------------------------------------------------------*/

typedef Object *(*cfunc)(Object *ob, Object *args);

typedef struct function_struct {
  char *name;
  char *rdesc;
  char *pdesc;
  int access;
  cfunc func;
} FunctionStruct;

int Klass_Add_CFunctions(Klass *klazz, FunctionStruct *funcs);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
