
#ifndef _KOALA_COMPILE_H_
#define _KOALA_COMPILE_H_

#include "symtable.h"
#include "ast.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct compiler {
  struct symtable *symtable;
  struct object *obj;
  struct object *arg;
};

int compiler_module(struct mod *mod);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_COMPILE_H_ */
