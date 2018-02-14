
#ifndef _KOALA_CODEOBJECT_H_
#define _KOALA_CODEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

#define CODE_KLANG  0
#define CODE_CLANG  1

typedef struct codeobject {
  OBJECT_HEAD
  int flags;
  union {
    cfunc cf;
    struct {
      STable *stbl;
      Proto *proto;
      int locvars;
      int size;
      uint8 *codes;
    } kf;
  };
} CodeObject;

/* Exported APIs */
extern Klass Code_Klass;
Object *KFunc_New(int locvars, uint8 *codes, int size);
Object *CFunc_New(cfunc cf);
#define CODE_ISKFUNC(code)  (((CodeObject *)(code))->flags == CODE_KLANG)
#define CODE_ISCFUNC(code)  (((CodeObject *)(code))->flags == CODE_CLANG)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CODEOBJECT_H_ */
