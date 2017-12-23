
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
#define TYPE_PTR      8

typedef struct tvalue {
  int type;
  union {
    void *ptr;
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

#define TVAL_INIT_INT(v)    {.type = TYPE_INT, .ival = (v)}
#define TVAL_INIT_PTR(v)    {.type = TYPE_PTR, .ptr = (v)}
#define TVAL_INIT_OBJ(t, v) {.type = t, .ob = (v)}
#define TVAL_INIT_NIL       TVAL_INIT_OBJ(TYPE_ANY, NULL)

/* Macros to test type */
#define TVAL_ISANY(v)     ((v).type == TYPE_ANY)
#define TVAL_ISINT(v)     ((v).type == TYPE_INT)
#define TVAL_ISBOOL(v)    ((v).type == TYPE_BOOL)
#define TVAL_ISOBJ(v)     ((v).type == TYPE_OBJECT)

/* Macros to access type */
#define TVAL_TYPE(v)      ((v).type)

/* Macros to access values */
extern TValue NilTVal;
#define NIL_TVAL          NilTVal
#define BYTE_TVAL(v)      ((v).byte)
#define INT_TVAL(v)       ((v).ival)
#define FLOAT_TVAL(v)     ((v).fval)
#define BOOL_TVAL(v)      ((v).bval)
#define OBJECT_TVAL(v)    ((v).ob)

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
#define OB_KLASS_EQUAL(ob1, ob2)  (OB_KLASS(ob1) == OB_KLASS(ob2))

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
  char *name;
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

typedef struct field_struct {
  char *name;
  char *desc;
  uint8 access;
} FieldStruct;

/*-------------------------------------------------------------------------*/

extern Klass Klass_Klass;
void Init_Klass_Klass(void);
Klass *Klass_New(char *name, int bsize, int isize);
int Klass_Add_Methods(Klass *klazz, MethodStruct *meths);
int Klass_Get(Klass *klazz, char *name, TValue *k, TValue *v);

TValue TValue_Build(char ch, ...);
int TValue_Parse(TValue val, char ch, ...);

uint32 Integer_Hash(TValue v);
int Ineger_Compare(TValue v1, TValue v2);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
