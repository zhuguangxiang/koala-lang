/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include <inttypes.h>
#include "ast.h"
#include "codegen.h"
#include "atom.h"
#include "list.h"
#include "common.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct module {
  /* module name is file name, dir name or __name__ */
  char *name;
  /* symbol table per module(share between files) */
  STable *stbl;
} Module;

/* ParserUnit scope */
typedef enum scopekind {
  SCOPE_MODULE = 1,
  SCOPE_CLASS,
  SCOPE_FUNCTION,
  SCOPE_BLOCK,
  SCOPE_CLOSURE,
} ScopeKind;

/* parser unit, one of Scope */
typedef struct parserunit {
  /* one of ScopeKind */
  ScopeKind scope;

  /* link to parserstate->ustack */
  struct list_head link;

  /* for function, class, trait and method */
  Symbol *sym;
  /* symbol table for current scope */
  STable *stbl;

  /* instructions within current scope */
  CodeBlock *block;

  int merge;
  int loop;
  Vector jmps;
} ParserUnit;

/* per source file */
typedef struct parserstate {
  /* file name */
  char *filename;
  /* its module */
  Module *module;

  /* is interactive ? */
  int interactive;
  /* is complete ? */
  int more;
  /* token */
  int token;
  /* token length */
  int len;
  /* token position */
  Position pos;

  /* current ParserUnit */
  ParserUnit *u;
  /* ParserUnit stack depth */
  int depth;
  /* link ParserUnit */
  struct list_head ustack;

  /* error numbers */
  int errnum;
} ParserState;

/* more than MAX_ERRORS, discard left errors shown */
#define MAX_ERRORS 8

/* Record and print syntax error. */
#define syntax_error(ps, pos, fmt, ...)                        \
({                                                             \
  if (++ps->errnum > MAX_ERRORS) {                             \
    fprintf(stderr, "%s: " __ERR_COLOR__ "Too many errors.\n", \
            ps->filename);                                     \
  } else {                                                     \
    fprintf(stderr, "%s:%d:%d: " __ERR_COLOR__ fmt "\n",       \
            ps->filename, pos->row, pos->col, __VA_ARGS__);    \
  }                                                            \
})

void parse_expr(ParserState *ps, Expr *exp);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
