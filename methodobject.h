
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
  uint16 type;
  uint16 nr_paras;
  uint16 nr_locals;
  uint16 nr_rets;
  union {
    cfunc cf;           // c language function
    struct {
      Object *closure;  // tuple for closure
      Object *k;        // constant tuple
      uint8 *codes;     // koala's instructions
    } kf;
  };
} MethodObject;

/* Exported symbols */
extern Klass Method_Klass;
void Init_Method_Klass(void);
Object *KMethod_New(uint8 *codes, Object *k, Object *closure);
Object *CMethod_New(cfunc cf);
Object *Method_Invoke(Object *mob, Object *ob, Object *args);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_METHODOBJECT_H_ */
