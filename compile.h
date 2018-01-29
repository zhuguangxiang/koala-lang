
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
  STable *stbl;
} Import;

typedef enum parser_state {
  PARSING_SYMBOLS = 1, PARSING_FUNCTIONS = 2, PARSING_INITFUNC = 3
} PARSER_STATE;

typedef struct parser_context {
  PARSER_STATE state;
  char *package;
  HashTable imports;
  Vector stmts;
  STable stable;
  Vector __initstmts__;
  int scope;
  struct list_head scopes;
} ParserContext;

typedef int (*stmt_handler_t)(ParserContext *, struct stmt *);
typedef int (*expr_handler_t)(ParserContext *, struct expr *);
int init_compiler(ParserContext *ctx);
int fini_compiler(ParserContext *ctx);
int compile(ParserContext *ctx);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_COMPILE_H_ */
