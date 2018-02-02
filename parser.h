
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "koala.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct funcdata {
  Object *owner;
  int scope;
  struct list_head scopes;
  Buffer buf;
  ItemTable *itable;
  int stacksize;
} FuncData;

typedef struct scope {
  struct list_head link;
  STable stable;
} Scope;

typedef struct import {
  HashNode hnode;
  char *id;
  char *path;
  Object *module;
  int refcnt;
} Import;

typedef struct error {
  char *msg;
  int line;
} Error;

typedef struct parser {
  char *package;
  HashTable imports;
  Vector stmts;
  Object *module;
  Vector __initstmts__;
  Object *klazz;
  FuncData func;
  Vector errors;
} Parser;

typedef void (*stmt_parser_t)(Parser *, struct stmt *);
typedef int (*expr_handler_t)(Parser *, struct expr *);
void parse(Parser *parser);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
