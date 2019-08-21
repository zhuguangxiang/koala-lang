/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include <inttypes.h>
#include "ast.h"
#include "atom.h"
#include "list.h"
#include "common.h"
#include "log.h"
#include "bytebuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct codeblock {
  int bytes;
  struct list_head insts;
  /* control flow */
  struct codeblock *next;
  /* false, no OP_RET, needs add one */
  int ret;
} CodeBlock;

typedef struct module {
  /* saved in global modules */
  HashMapEntry entry;
  /* deployed path */
  char *path;
  /* module name is file name, dir name or __name__ */
  char *name;
  /* symbol table per module(share between files) */
  STable *stbl;
  /* ParserState per source file */
  Vector pss;
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
  /* for module, function, class, trait and method */
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
  short row;
  short col;

  /* current parserunit */
  ParserUnit *u;
  /* depth of parserunit */
  int depth;
  /* parserunit stack */
  Vector ustack;

  /* error numbers */
  int errnum;
} ParserState;

/* more than MAX_ERRORS, discard left errors shown */
#define MAX_ERRORS 8

/* Record and print syntax error. */
#define syntax_error(ps, row, col, fmt, ...)             \
({                                                       \
  if (++ps->errnum > MAX_ERRORS) {                       \
    fprintf(stderr, "%s: " __ERR_COLOR__                 \
            "Too many errors.\n", ps->filename);         \
  } else {                                               \
    fprintf(stderr, "%s:%d:%d: " __ERR_COLOR__ fmt "\n", \
            ps->filename, row, col, ##__VA_ARGS__);      \
  }                                                      \
})

void codeblock_free(CodeBlock *block);
void init_parser(void);
void fini_parser(void);
void parser_enter_scope(ParserState *ps, ScopeKind scope);
void parser_exit_scope(ParserState *ps);
void parse_stmt(ParserState *ps, Stmt *stmt);
void code_gen(CodeBlock *block, Image *image, ByteBuffer *buf);
Symbol *find_from_builtins(char *name);
void mod_from_mobject(Module *mod, Object *ob);
Symbol *mod_find_symbol(Module *mod, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
