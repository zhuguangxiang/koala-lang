
#ifndef _KOALA_METHODOBJECT_H_
#define _KOALA_METHODOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/*-------------------------------------------------------------------------*/

#define METH_CFUNC    1
#define METH_KFUNC    2
#define METH_VARGS    4

typedef struct methodobject {
  OBJECT_HEAD
  short flags;
  short rets;
  short args;
  short locals;
  union {
    cfunc cf;             // c language function
    struct {
      //Object *closure;  // tuple for closure
      AtomTable *atable;  // constant pool
      CodeInfo codeinfo;  // codeinfo
    } kf;
  };
} MethodObject;

/* Exported APIs */
extern Klass Method_Klass;
void Init_Method_Klass(Object *ob);
Object *Method_New(FuncInfo *info, AtomTable *atable);
Object *CFunc_New(cfunc cf, ProtoInfo *proto);
#define METH_ISCFUNC(meth)  ((meth)->flags & METH_CFUNC)
#define METH_ISKFUNC(meth)  ((meth)->flags & METH_KFUNC)
#define METH_ISVARGS(meth)  ((meth)->flags & METH_VARGS)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_METHODOBJECT_H_ */
