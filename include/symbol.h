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

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* symbol table */
typedef struct symboltable {
  /* hash table for saving symbols */
  HashTable table;
  /* constant and variable allcated index */
  int varindex;
} STable;

/* symbol kind */
typedef enum symbolkind {
  SYM_CONST  = 1,   /* constant           */
  SYM_VAR    = 2,   /* variable           */
  SYM_FUNC   = 3,   /* function or method */
  SYM_ALIAS  = 4,   /* type alias         */
  SYM_CLASS  = 5,   /* clas               */
  SYM_TRAIT  = 6,   /* trait              */
  SYM_IFUNC  = 7,   /* interface method   */
  SYM_NFUNC  = 8,   /* native function    */
  SYM_AFUNC  = 9,   /* anonymous function */
  SYM_PKG    = 10,  /* (external) package */
  SYM_REF    = 11,  /* reference symbol   */
  SYM_MAX
} SymKind;

/* source code token position for error handler */
typedef struct position {
  int row;
  int col;
} Position;

#define SYMBOL_HEAD \
  SymKind kind;     \
  /* hash node */   \
  HashNode hnode;   \
  /* symbol key */  \
  char *name;       \
  /* symbol type */ \
  TypeDesc *desc;   \
  /* filename */    \
  char *filename;   \
  /* position */    \
  Position pos;     \
  /* free refcnt */ \
  int refcnt;       \
  /* used count */  \
  int used;

/* symbol structure */
typedef struct symbol {
  SYMBOL_HEAD
} Symbol;

/* constant value */
typedef struct constvalue {
  /* see BASE_XXX in typedesc.h */
  int kind;
  union {
    uchar ch;
    int64 ival;
    float64 fval;
    int bval;
    char *str;
  };
} ConstValue;

/* constant and variable symbol */
typedef struct varsymbol {
  SYMBOL_HEAD
  /* variable index */
  int32 index;
  /* if is constant, save its value */
  ConstValue value;
} VarSymbol;

/* function symbol */
typedef struct funcsymbol {
  SYMBOL_HEAD
  /* local varibles in the function */
  Vector locvec;
  /* codeblock */
  void *code;
} FuncSymbol;

/* class and trait symbol */
typedef struct classsymbol {
  SYMBOL_HEAD
  /* supers in liner-oder */
  Vector supers;
  /* symbol table */
  STable *stbl;
} ClassSymbol;

/* closure/anonoymous function */
typedef struct afuncsymbol {
  SYMBOL_HEAD
  /* local varibles in the closure */
  Vector locvec;
  /* up local variables */
  Vector uplocvec;
  /* codeblock */
  void *code;
} AFuncSymbol;

/* package symbol */
typedef struct pkgsymbol {
  SYMBOL_HEAD
  /* package, not need free */
  void *pkg;
} PkgSymbol;

/* reference symbol */
typedef struct refsymbol {
  SYMBOL_HEAD
  /* dot import path */
  char *path;
  /* reference symbol in package */
  Symbol *sym;
  /* reference package */
  void *pkg;
} RefSymbol;

STable *STable_New(void);
typedef void (*symbol_visit_func)(Symbol *sym, void *arg);
void __STable_Free(STable *stbl, symbol_visit_func visit, void *arg);
#define STable_Free(stbl) __STable_Free(stbl, NULL, NULL)

Symbol *Symbol_New(SymKind kind, char *name);
void Symbol_Free(Symbol *sym);
#define Symbol_IsUsed(sym) ((sym)->used > 0)
void Show_Symbol(Symbol *sym);

Symbol *STable_Add_Const(STable *stbl, char *name, TypeDesc *desc);
Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc);
Symbol *STable_Add_Func(STable *stbl, char *name, TypeDesc *proto);
Symbol *STable_Add_Alias(STable *stbl, char *name, TypeDesc *desc);
Symbol *STable_Add_Class(STable *stbl, char *name);
Symbol *STable_Add_Trait(STable *stbl, char *name);
Symbol *STable_Add_Proto(STable *stbl, char *name, int k, TypeDesc *desc);
#define STable_Add_IFunc(stbl, name, proto) \
  STable_Add_Proto(stbl, name, SYM_IFUNC, proto)
#define STable_Add_NFunc(stbl, name, proto) \
  STable_Add_Proto(stbl, name, SYM_NFUNC, proto)
Symbol *STable_Add_Anonymous(STable *stbl, TypeDesc *desc);
PkgSymbol *STable_Add_Package(STable *stbl, char *name);
RefSymbol *STable_Add_Reference(STable *stbl, char *name);
Symbol *STable_Get(STable *stbl, char *name);
KImage *Gen_Image(STable *stbl, char *pkgname);
int STable_From_Image(char *path, char **pkgname, STable **stbl);
void STable_Show(STable *stbl);
void STable_Visit(STable *stbl, symbol_visit_func fn, void *arg);
void __Copy_Symbol_Func(Symbol *sym, STable *to);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
