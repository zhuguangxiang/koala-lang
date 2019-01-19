/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "codegen.h"
#include "options.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * per one package which includes all source files in the same directory.
 * These files must be the same package name.
 */
typedef struct packageinfo {
  /* package saved in pkgfile */
  String pkgfile;
  /* package name */
  String pkgname;
  /* imported external packages' symbol table, path as key */
  HashTable *extstbl;
  /* symbol table saves all symbols of the package */
  STable *stbl;
  /* compiling options for compiling other packages, if necessary */
  Options *opts;
} PkgInfo;

int Init_PkgInfo(PkgInfo *pkg, char *pkgname, char *pkgfile, Options *opts);
void Fini_PkgInfo(PkgInfo *pkg);
#if 1
void Show_PkgInfo(PkgInfo *pkg);
#else
#define Show_PkgInfo(pkg) ((void *)0)
#endif

/* line max length */
#define LINE_MAX_LEN 256
/* a token max length */
#define TOKEN_MAX_LEN 80

/* line buffer for flex input */
typedef struct line_buf {
  /* line data cache */
  char buf[LINE_MAX_LEN];
  /* line data length */
  int linelen;
  /* left data length */
  int lineleft;

  /* current token length */
  int len;
  /* current token buffer */
  char token[TOKEN_MAX_LEN];

  /* current token's position */
  int row;
  int col;

  /* error count of current line */
  int errors;
} LineBuffer;

/* parser unit scope */
typedef enum scopekind {
  SCOPE_MODULE = 1,
  SCOPE_FUNCTION,
  SCOPE_BLOCK,
  SCOPE_CLOSURE,
  SCOPE_CLASS,
  SCOPE_METHOD
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
  int8 merge;
  int8 loop;
  Vector jmps;
} ParserUnit;

/* max errors before stopping compiling */
#define MAX_ERRORS 8

/*
 * ParserState per one source file
 */
typedef struct parserstate {
  /* file name for this module */
  char *filename;
  /* current compiling source file's package name */
  char *pkgname;
  /* package ptr, all modules have the same pacakge */
  PkgInfo *pkg;

  /* save last token for if inserted semicolon or not */
  int lastToken;
  /* input line buffer */
  LineBuffer line;

  /* all statements */
  Vector stmts;

  /* imported information */
  HashTable imports;
  /* external symbol table, imported-name or package-name as key */
  STable *extstbl;

  /* top parser unit */
  ParserUnit top;
  /* current parser unit */
  ParserUnit *u;
  /* parser unit stack depth */
  int depth;
  /* link ParserUnit */
  struct list_head ustack;

  /* optimization level */
  short olevel;
  /* warning level */
  short wlevel;
  /* number of errors */
  int errnum;
  /* error messages */
  Vector errors;
} ParserState;

#include "koala_yacc.h"

#define loc_row(loc) ((loc)->first_line)
#define loc_col(loc) ((loc)->first_column)

void Parser_Enter_Scope(ParserState *ps, ScopeKind scope);
void Parser_Exit_Scope(ParserState *ps);

ParserState *New_Parser(PkgInfo *pkg, char *filename);
int Build_AST(ParserState *ps, FILE *in);
void Parse(ParserState *ps);
void Destroy_Parser(ParserState *ps);
void Check_Unused_Imports(ParserState *ps);
void Check_Unused_Symbols(ParserState *ps);

/* yacc(bison) used APIs */
void Init_Imports(ParserState *ps);
void Fini_Imports(ParserState *ps);
Symbol *Parser_New_Import(ParserState *ps, char *id, char *path,
  YYLTYPE *idloc,YYLTYPE *pathloc);

void Parser_New_Variables(ParserState *ps, Stmt *stmt);
Stmt *__Parser_Do_Variables(ParserState *ps, Vector *ids, TypeDesc *desc,
  Vector *exps, int k);
#define Parser_Do_Variables(ps, ids, desc, exps) \
  __Parser_Do_Variables(ps, ids, desc, exps, 0)
#define Parser_Do_Constants(ps, ids, desc, exps) \
  __Parser_Do_Variables(ps, ids, desc, exps, 1)
Stmt *Parser_Do_Assignments(ParserState *ps, Vector *left, Vector *right);
void Parser_New_Function(ParserState *ps, Stmt *stmt);
void Parser_New_TypeAlias(ParserState *ps, Stmt *stmt);
void Parser_New_ClassOrTrait(ParserState *ps, Stmt *stmt);
TypeDesc *Parser_New_KlassType(ParserState *ps, char *id, char *klazz);
void Parser_SetLineInfo(ParserState *ps, LineInfo *line);

void Parser_Synatx_Error(ParserState *ps, YYLTYPE *loc, char *fmt, ...);

/* lex(flex) used APIs */
int Lexer_DoYYInput(ParserState *ps, char *buf, int size, FILE *in);
void Lexer_DoUserAction(ParserState *ps, char *text);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
