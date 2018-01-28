
#ifndef _KOALA_COMPILE_H_
#define _KOALA_COMPILE_H_

#include "koala.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct scope {
  struct list_head link;
  STable stable;
} ScopeContext;

typedef struct import {
  HashNode hnode;
  char *id;
  char *path;
} Import;

typedef struct compilecontext {
  int times;
  char *package;
  HashTable imports;
  Vector *stmts;
  STable stable;
  int scope;
  struct list_head scopes;
} CompileContext;

int init_compiler(CompileContext *ctx);
int fini_compiler(CompileContext *ctx);
int compile(CompileContext *ctx);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_COMPILE_H_ */
