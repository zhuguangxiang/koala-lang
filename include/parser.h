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
typedef struct packagestate {
  /* package saved in pkgfile */
  char *pkgfile;
  /* package name */
  char *pkgname;
  /* imported external packages, path as key */
  HashTable extpkgs;
  /* symbol table saves all symbols of the package */
  STable *stbl;
  /* compiling options for compiling other packages, if necessary */
  Options *opts;
  /* modules: ParserState */
  Vector modules;
} PackageState;

int Init_PackageState(PackageState *pkg, char *pkgfile, Options *opts);
void Fini_PackageState(PackageState *pkg);
void Show_PackageState(PackageState *pkg);

/* external package for import */
typedef struct extpkg {
  HashNode hnode;
  char *path;
  char *pkgname;
  STable *stbl;
} ExtPkg;

ExtPkg *ExtPkg_Find(PackageState *pkg, char *path);
ExtPkg *ExtPkg_New(PackageState *pkg, char *path, char *pkgname, STable *stbl);

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
  Position pos;

  /* last line's token */
  Position lastpos;

  /* error count of current line */
  int errors;
} LineBuffer;

/* parser unit scope */
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

  int8 merge;
  int8 loop;
  Vector jmps;
} ParserUnit;

/* max errors before stopping compiling */
#define MAX_ERRORS 8

#define STATE_NONE 0
#define STATE_BUILDING_AST 1
#define STATE_PARSING_AST 2

/*
 * ParserState per one source file
 */
typedef struct parserstate {
  /* state */
  int state;

  /* file name for this module */
  char *filename;
  /* current compiling source file's package name */
  char *pkgname;
  /* package ptr, all modules have the same pacakge */
  PackageState *pkg;

  /* save last token for if inserted semicolon or not */
  int lastToken;
  /* input line buffer */
  LineBuffer line;

  /* all statements */
  Vector stmts;

  /*
   * external symbol table,
   * imported-name, package-name or symbol-name as key
   */
  STable *extstbl;

  /* current parser unit */
  ParserUnit *u;
  /* parser unit stack depth */
  int depth;
  /* link ParserUnit */
  struct list_head ustack;

  /* number of errors */
  int errnum;
  /* error messages */
  Vector errors;
} ParserState;

void Parser_Set_PkgName(ParserState *ps, Ident *id);

void Parser_Enter_Scope(ParserState *ps, ScopeKind scope);
void Parser_Exit_Scope(ParserState *ps);

ParserState *New_Parser(PackageState *pkg, char *filename);
int Build_AST(ParserState *ps, FILE *in);
int Parse_AST(ParserState *ps);
void Destroy_Parser(ParserState *ps);
void Check_Unused_Imports(ParserState *ps);
void Check_Unused_Symbols(ParserState *ps);

extern const char *scope_strings[];
#define scope_name(u) scope_strings[(u)->scope]

void Parse_Expression(ParserState *ps, Expr *exp);
void Code_Expression(ParserState *ps, Expr *exp);
void Parse_Ident_Expr(ParserState *ps, Expr *exp);
void Code_Ident_Expr(ParserState *ps, Expr *exp);
void Parse_Unary_Expr(ParserState *ps, Expr *exp);
void Code_Unary_Expr(ParserState *ps, Expr *exp);
void Parse_Binary_Expr(ParserState *ps, Expr *exp);
void Code_Binary_Expr(ParserState *ps, Expr *exp);

typedef void (*code_func)(ParserState *, void *);
typedef struct code_generator {
  ScopeKind scope;
  code_func code;
} CodeGenerator;

/* yacc(bison) used APIs */
void Parser_New_Import(ParserState *ps, Ident *id, Ident *path);
void Parser_New_Variables(ParserState *ps, Stmt *stmt);
Stmt *__Parser_Do_Variables(ParserState *ps, Vector *ids, TypeWrapper type,
                            Vector *exps, int konst);
#define Parser_Do_Variables(ps, ids, desc, exps) \
  __Parser_Do_Variables(ps, ids, desc, exps, 0)
#define Parser_Do_Constants(ps, ids, desc, exps) \
  __Parser_Do_Variables(ps, ids, desc, exps, 1)
Stmt *Parser_Do_Typeless_Variables(ParserState *ps, Vector *ids, Vector *exps);
Stmt *Parser_Do_Assignments(ParserState *ps, Vector *left, Vector *right);
void Parser_New_Function(ParserState *ps, Stmt *stmt);
void Parser_New_TypeAlias(ParserState *ps, Stmt *stmt);
void Parser_New_ClassOrTrait(ParserState *ps, Stmt *stmt);
TypeDesc *Parser_New_KlassType(ParserState *ps, char *id, char *klazz);
void Syntax_Error(ParserState *ps, Position *pos, char *fmt, ...);

/* lex(flex) used APIs */
int Lexer_DoYYInput(ParserState *ps, char *buf, int size, FILE *in);
void Lexer_DoUserAction(ParserState *ps, char *text);

#include "koala_yacc.h"

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
