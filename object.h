
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

typedef struct object Object;

#define TYPE_ANY      0
#define TYPE_BYTE     1
#define TYPE_CHAR     2
#define TYPE_INT      3
#define TYPE_FLOAT    4
#define TYPE_BOOL     5
#define TYPE_OBJECT   6
#define TYPE_STRING   7

typedef struct tvalue {
  int type;
  union {
    Object *ob;
    uint8 byte;
    uint16 ch;
    int64 ival;
    float64 fval;
    int bval;
  };
} TValue;

/* Macros to initialize tvalue */
#define init_nil_tval(v) do { \
  (v).type = TYPE_ANY; (v).ob = NULL; \
} while (0)

#define NIL_TVAL_INIT {.type = TYPE_ANY, .ob = NULL}
#define TVAL_NIL ({TValue val = NIL_TVAL_INIT; val;})

/* Macros to test type */
#define tval_isany(v)     ((v).type == TYPE_ANY)
#define tval_isint(v)     ((v).type == TYPE_INT)
#define tval_isbool(v)    ((v).type == TYPE_BOOL)
#define tval_isobject(v)  ((v).type == TYPE_OBJECT)
#define tval_isany(v)     ((v).type == TYPE_ANY)

/* Macros to access type */
#define TVAL_TYPE(v)      ((v).type)

/* Macros to access values */
#define TVAL_BYTE(v)      ((v).byte)
#define TVAL_INT(v)       ((v).ival)
#define TVAL_FLOAT(v)     ((v).fval)
#define TVAL_BOOL(v)      ((v).bval)
#define TVAL_OBJECT(v)    ((v).ob)

/*-------------------------------------------------------------------------*/

typedef struct klass Klass;

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

#define OB_KLASS(ob)  (((Object *)(ob))->ob_klass)
#define OB_CHECK_KLASS(ob, klazz) assert(OB_KLASS(ob) == &(klazz))
#define ob_klass_eq(ob1, ob2) (OB_KLASS(ob1) == OB_KLASS(ob2))

#define ob_incref(ob) (((Object *)ob)->ob_ref++)
#define ob_decref(ob) do { \
  if (--((Object *)ob)->ob_ref == 0) { \
    OB_KLASS(ob)->ob_free((Object *)ob); \
  } \
} while (0)

/*-------------------------------------------------------------------------*/

typedef void (*markfunc)(Object *ob);

typedef Object *(*allocfunc)(Klass *klazz, int num);
typedef void (*freefunc)(Object *ob);

typedef uint32 (*hashfunc)(TValue v);
typedef int (*cmpfunc)(TValue v1, TValue v2);

typedef Object *(*strfunc)(TValue v);

struct klass {
  OBJECT_HEAD
  const char *name;
  int bsize;
  int isize;

  markfunc  ob_mark;

  allocfunc ob_alloc;
  freefunc  ob_free;

  hashfunc  ob_hash;
  cmpfunc   ob_cmp;

  strfunc   ob_tostr;

  int nr_fields;
  int nr_methods;
  Object *table;
};

/*-------------------------------------------------------------------------*/

typedef Object *(*cfunc)(Object *ob, Object *args);

typedef struct method_struct {
  char *name;
  char *rdesc;
  char *pdesc;
  uint8 access;
  cfunc func;
} MethodStruct;

typedef struct member_struct {
  char *name;
  char *desc;
  uint8 access;
  uint16 offset;
} MemberStruct;

/*-------------------------------------------------------------------------*/

extern Klass Klass_Klass;
void Init_Klass_Klass(void);
Klass *Klass_New(const char *name, int bsize, int isize);
int Klass_Add_Methods(Klass *klazz, MethodStruct *meths);
TValue Klass_Get(Klass *klazz, char *name);

TValue TValue_Build(char ch, ...);
int TValue_Parse(TValue val, char ch, ...);

uint32 Integer_Hash(TValue v);
int Ineger_Compare(TValue v1, TValue v2);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
