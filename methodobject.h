
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

#define METH_CFUNC  1
#define METH_KFUNC  2

typedef struct methodobject {
  OBJECT_HEAD
  uint16 type;
  uint16 rets;
  uint16 args;
  uint16 locals;
  union {
    cfunc cf;             // c language function
    struct {
      //Object *closure;  // tuple for closure
      ItemTable *itable;  // item table with const string
      ConstItem *k;       // constant pool
      uint8 *codes;       // koala's instructions
    } kf;
  };
} MethodObject;

/* Exported APIs */
extern Klass Method_Klass;
void Init_Method_Klass(Object *ob);
Object *Method_New(FuncInfo *info, ConstItem *k, ItemTable *itable);
Object *CMethod_New(cfunc cf, ProtoInfo *proto);
void Module_Free(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_METHODOBJECT_H_ */
