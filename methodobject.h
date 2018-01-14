
#ifndef _KOALA_METHODOBJECT_H_
#define _KOALA_METHODOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define METH_CFUNC  1
#define METH_KFUNC  2

typedef struct methodobject {
  OBJECT_HEAD
  Object *owner;
  uint16 type;
  uint16 nr_args;
  uint16 nr_locals;
  uint16 nr_rets;
  union {
    cfunc cf;             // c language function
    struct {
      Object *closure;    // tuple for closure
      ConstItem *k;       // constant pool
      uint8 *codes;       // koala's instructions
    } kf;
  };
} MethodObject;

/* Exported APIs */
extern Klass Method_Klass;
void Init_Method_Klass(Object *ob);
Object *KMethod_New(uint8 *codes, ConstItem *k, Object *closure);
Object *CMethod_New(cfunc cf);
Object *Method_Invoke(Object *method, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_METHODOBJECT_H_ */
