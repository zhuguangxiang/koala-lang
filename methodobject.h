
#ifndef _KOALA_METHODOBJECT_H_
#define _KOALA_METHODOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct methodproto {
  uint16 nr_rets;
  uint16 nr_args;
  uint16 nr_locals;
} MethodProto;

#define METHOD_PROTO_INIT(rets, args, locals) {(rets), (args), (locals)}
#define Init_Method_Proro(proto, rets, args, locals) do { \
  (proto)->nr_rets = (rets); \
  (proto)->nr_args = (args); \
  (proto)->nr_locals = (locals); \
} while (0)

/*
  All functions's proto, including c function and koala function
  'args' and 'return' are both Tuple.
 */
typedef Object *(*cfunc)(Object *ob, Object *args);

typedef struct funcstruct {
  char *name;
  char *rdesc;
  char *pdesc;
  cfunc func;
} FuncStruct;

int Klass_Add_CFunctions(Klass *klazz, FuncStruct *funcs);
void FuncStruct_Get_Proto(MethodProto *proto, FuncStruct *f);

/*-------------------------------------------------------------------------*/

#define METH_CFUNC  1
#define METH_KFUNC  2

typedef struct methodobject {
  OBJECT_HEAD
  uint16 type;
  uint16 nr_rets;
  uint16 nr_args;
  uint16 nr_locals;
  union {
    cfunc cf;             // c language function
    struct {
      //Object *closure;    // tuple for closure
      ItemTable *itable;  // item table with const string
      ConstItem *k;       // constant pool
      uint8 *codes;       // koala's instructions
    } kf;
  };
} MethodObject;

/* Exported APIs */
extern Klass Method_Klass;
void Init_Method_Klass(Object *ob);
Object *Method_New(MethodProto *proto, uint8 *codes,
                   ConstItem *k, ItemTable *itable);
Object *CMethod_New(cfunc cf, MethodProto *proto);
void Module_Free(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_METHODOBJECT_H_ */
