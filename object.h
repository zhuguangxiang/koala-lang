
#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

#define TYPE_NIL      0
#define TYPE_CHAR     1
#define TYPE_INT      2
#define TYPE_FLOAT    3
#define TYPE_BOOL     4
#define TYPE_OBJECT   5
#define TYPE_STRING   6
#define TYPE_ANY      7
#define TYPE_FUNC     8
#define TYPE_NAME     9

typedef struct object Object;

typedef struct tvalue {
  int type;
  union {
    Object *ob;
    uint16 ch;
    int64 ival;
    float64 fval;
    int bval;
    cfunc func;
    Name *name;
  };
} TValue;

/* Macros to initialize tvalue */
#define init_nil_tval(tv) do { \
  (tv)->type = TYPE_NIL; (tv)->ob = NULL; \
} while (0)

/* Macros to test type */
#define tval_isnil(tv)  ((tv)->type == TYPE_NIL)
#define tval_isint(tv)  ((tv)->type == TYPE_INT)
#define tval_isbool(tv) ((tv)->type == TYPE_BOOL)
#define tval_isobject(tv) \
  ((tv)->type == TYPE_OBJECT || (tv)->type == TYPE_STRING)
#define tval_isany(tv)  ((tv)->type == TYPE_ANY)
#define tval_isname(tv) ((tv)->type == TYPE_NAME)

/* Macros to access values */
#define TVAL_IVAL(tv) ((tv)->ival)
#define TVAL_OVAL(tv) ((tv)->ob)

/*-------------------------------------------------------------------------*/

typedef struct klass Klass;

#define OBJECT_HEAD \
  Object *ob_next; int ob_ref; Klass *ob_klass;

struct object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(klass) \
  .ob_next = NULL, .ob_ref = 1, .ob_klass = klass

#define init_object_head(ob, klass)  do {  \
  Object *o = (Object *)ob;  \
  o->ob_next = NULL; o->ob_ref = 1; o->ob_klass = klass; \
} while (0)

#define OB_KLASS(ob)  (((Object *)(ob))->ob_klass)

#define ob_klass_equal(ob1, ob2) (OB_KLASS(ob1) == OB_KLASS(ob2))

#define ob_incref(ob) (((Object *)ob)->ob_ref++)
#define ob_decref(ob) do { \
  if (--((Object *)ob)->ob_ref == 0) { \
    OB_KLASS(ob)->ob_free((Object *)ob); \
  } \
} while (0)

/*-------------------------------------------------------------------------*/

typedef void (*markfunc)(Object *ob);

typedef Object *(*allocfunc)(Klass *klass, int num);
typedef void (*freefunc)(Object *ob);

typedef uint32_t (*hashfunc)(TValue *tv);
typedef int (*cmpfunc)(TValue *tv1, TValue *tv2);

typedef Object *(*strfunc)(TValue *tv);

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

  int nr_vars;
  int nr_meths;
  Object *table;
};

/*-------------------------------------------------------------------------*/

#define NT_VAR      1
#define NT_FUNC     2
#define NT_STRUCT   3
#define NT_INTF     4
#define NT_MODULE   5

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  1
#define ACCESS_RDONLY   2

typedef Object *(*cfunc)(Object *ob, Object *args);

typedef struct method_struct {
  char *name;
  char *signature;
  uint8 access;
  cfunc func;
} MethodStruct;

typedef struct member_struct {
  char *name;
  char *signature;
  uint8 access;
  uint16 offset;
} MemberStruct;

typedef struct name_struct {
  char *name;
  char *signature;
  uint8 type;
  uint8 access;
} Name;

#define name_isprivate(n) ((n)->access & ACCESS_PRIVATE)
#define name_ispublic(n)  (!name_isprivate(n))
#define name_isconst(n)   ((n)->access & ACCESS_RDONLY)

/*-------------------------------------------------------------------------*/

extern Klass Klass_Klass;
Klass *Klass_New(const char *name, int bsize, int isize);
int Klass_Add_Methods(Klass *klass, MethodStruct *meths);
int Ineger_Compare(TValue *tv1, TValue *tv2);
Name *Name_New(char *name, uint8 type, char *signature, uint8 access);
int Name_Compare(TValue *tv1, TValue *tv2);
uint32 Name_Hash(TValue *tv);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
