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
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* package */
typedef struct package {
  /* save in extpkgs in global */
  HashNode hnode;
  /* deployed path */
  char *path;
  /* this package name */
  char *pkgname;
  /* symbols */
  STable *stbl;
} Package;

/*
 * one package which includes modules in one directory.
 * These modules must be the same package name.
 */
typedef struct parsergroup {
  /* package pointer */
  Package *pkg;
  /* modules: ParserState */
  Vector modules;
} ParserGroup;

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

/* import info */
typedef struct import {
  /* ID, MAY be null */
  char *id;
  Position idpos;
  /* PATH, NOT null */
  char *path;
  Position pathpos;
} Import;

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
  /* ParserGroup ptr, all modules have the same package-name */
  ParserGroup *grp;

  /* save last token for if inserted semicolon or not */
  int lastToken;
  /* input line buffer */
  LineBuffer line;

  /* all statements */
  Vector stmts;
  /* symbols in this module */
  Vector symbols;

  /* Import info, <ID>:<PATH>, <package-name>:<PATH>, .:<PATH> */
  Vector imports;
  /*
   * Save all external symbols using import (ID) "<package-path>".
   * Symbols in module are conflict with extstbl.
   * Importing it will also check it is in extdots.
   */
  STable *extstbl;
  /*
   * Save all external symbols using import . "<package-path>".
   * Symbols in module is NOT conflict with extdots.
   * Importing one by one will also check it is in extstbl.
   * Extstbl and extdots are conflict with each other.
   * If one(only) symbol is used, there is no unused error of the package.
   */
  STable *extdots;

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

Package *New_Package(char *path);
void Free_Package(Package *pkg);
Package *Find_Package(char *path);
void Parser_Set_PkgName(ParserState *ps, Ident *id);
void Parse_Imports(ParserState *ps);
void Add_ParserGroup(char *pkgpath);

void Parser_Enter_Scope(ParserState *ps, ScopeKind scope);
void Parser_Exit_Scope(ParserState *ps);
ParserUnit *Parser_Get_UpScope(ParserState *ps);

ParserState *New_Parser(ParserGroup *grp, char *filename);
void Destroy_Parser(ParserState *ps);
void Build_AST(ParserState *ps, FILE *in);
void Parse_AST(ParserState *ps);
void CheckConflictWithExternal(ParserState *ps);
void Check_Unused_Imports(ParserState *ps);
void Check_Unused_Symbols(ParserState *ps);

extern Options options;
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
void Parser_New_Const(ParserState *ps, Stmt *stmt);
void Parser_New_Var(ParserState *ps, Stmt *stmt);
void Parser_New_Func(ParserState *ps, Stmt *stmt);
void Parser_New_Proto(ParserState *ps, Stmt *stmt);
void Parser_New_Class(ParserState *ps, Stmt *stmt);
void Parser_New_Trait(ParserState *ps, Stmt *stmt);
void Parser_New_Enum(ParserState *ps, Stmt *stmt);
TypeDesc *Parser_New_KlassType(ParserState *ps, Ident *id, Ident *klazz);
#define Syntax_Error(pos, fmt, ...) \
  __Syntax_Error(ps, pos, fmt, ##__VA_ARGS__)
void __Syntax_Error(ParserState *ps, Position *pos, char *fmt, ...);

/* lex(flex) used APIs */
int Lexer_DoYYInput(ParserState *ps, char *buf, int size, FILE *in);
void Lexer_DoUserAction(ParserState *ps, char *text);

#include "koala_yacc.h"

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
