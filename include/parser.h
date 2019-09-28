/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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
#include "image.h"

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
  /* init func symbol */
  Symbol *initsym;
} Module;

/* ParserUnit scope */
typedef enum scopekind {
  SCOPE_MODULE = 1,
  SCOPE_CLASS,
  SCOPE_FUNC,
  SCOPE_BLOCK,
  SCOPE_ANONY,
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
  /* which block scope */
  int blocktype;
#define ONLY_BLOCK    1
#define IF_BLOCK      2
#define WHILE_BLOCK   3
#define FOR_BLOCK     4
#define MATCH_BLOCK   5
  /* save break & continue */
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
  /* interactive quit */
  int quit;

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
#define syntax_error(ps, row, col, fmt, ...)           \
({                                                     \
  if (++ps->errnum > MAX_ERRORS) {                     \
    fprintf(stderr, "%s: " _ERR_COLOR_                 \
            "Too many errors.\n", ps->filename);       \
  } else {                                             \
    fprintf(stderr, "%s:%d:%d: " _ERR_COLOR_ fmt "\n", \
            ps->filename, row, col, ##__VA_ARGS__);    \
  }                                                    \
})

#define has_error(ps) ((ps)->errnum > 0)

void codeblock_free(CodeBlock *block);
void init_parser(void);
void fini_parser(void);
ParserState *new_parser(char *filename);
void free_parser(ParserState *ps);
void parser_enter_scope(ParserState *ps, ScopeKind scope, int block);
void parser_exit_scope(ParserState *ps);
void parse_stmt(ParserState *ps, Stmt *stmt);
void parser_visit_expr(ParserState *ps, Expr *exp);
void code_gen(CodeBlock *block, Image *image, ByteBuffer *buf);
Symbol *find_from_builtins(char *name);
void mod_from_mobject(Module *mod, Object *ob);
Symbol *mod_find_symbol(Module *mod, char *name);
Symbol *get_desc_symbol(ParserState *ps, TypeDesc *desc);
void fill_locvars(Symbol *sym, Vector *vec);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
