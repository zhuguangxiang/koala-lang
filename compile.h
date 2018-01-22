
#ifndef _KOALA_COMPILE_H_
#define _KOALA_COMPILE_H_

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

struct scope_context {
  struct list_head link;
  HashTable stable;
  ItemTable *itable;
};

struct compiler {
  char *package;
  HashTable *stable;
  ItemTable *itable;
  ModuleObject *module;
  int scope;
  struct list_head scopes;
};

int init_compiler(struct compiler *cp);
int fini_compiler(struct compiler *cp);
int compile(struct compiler *cp, Vector *stmts);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_COMPILE_H_ */
