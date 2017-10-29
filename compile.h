
#ifndef _KOALA_COMPILE_H_
#define _KOALA_COMPILE_H_

#include "symtable.h"
#include "ast.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Compiler {
  struct symtable *symtable;
  struct Object *obj;
  struct Object *arg;
};

int compiler_module(struct sequence *stmts);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_COMPILE_H_ */
