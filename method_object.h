
#ifndef _KOALA_METHOD_OBJECT_H_
#define _KOALA_METHOD_OBJECT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

#define METH_CFUNC    1
#define METH_KFUNC    2
#define METH_CLOSURE  3

struct method_object {
  OBJECT_HEAD
  int flags;
};

struct code_struct {
  uint8_t *codes;
  struct linkage_struct *linkages;
};

struct closure_struct {
  int size;
  struct object **items;
};

/* Exported symbols */
extern struct klass_object method_klass;
void init_method_klass(void);
struct object *new_cfunc(cfunc_t func);
struct object *new_kfunc(void *codes, void *consts);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_METHOD_OBJECT_H_ */
