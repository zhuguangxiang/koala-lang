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
  SYM_CONST  = 1, /* constant */
  SYM_VAR    = 2, /* variable */
  SYM_FUNC   = 3, /* function or method */
  SYM_ALIAS  = 4, /* type alias */
  SYM_CLASS  = 5, /* clas */
  SYM_TRAIT  = 6, /* trait */
  SYM_IFUNC  = 7, /* interface method */
  SYM_NFUNC  = 8, /* native function */
  SYM_IMPORT = 9, /* import */
} SymKind;

#define SYMBOL_HEAD \
  SymKind kind;     \
  HashNode hnode;   \
  char *name;       \
  int refcnt;       \
  void *parent;

/* symbol */
typedef struct symbol {
  SYMBOL_HEAD
} Symbol;

/* constant and variable symbol */
typedef struct varsymbol {
  SYMBOL_HEAD
  TypeDesc *desc; /* variable type */
  int32 index;    /* variable index */
} VarSymbol;

/* function symbol */
typedef struct funcsymbol {
  SYMBOL_HEAD
  TypeDesc *desc; /* function's proto */
  Vector locvec;  /* local varibles in the function */
  void *code;     /* codeblock */
} FuncSymbol;

/* typealias symbol */
typedef struct aliassymbol {
  SYMBOL_HEAD
  TypeDesc *desc; /* real type */
} AliasSymbol;

/* class and trait symbol */
typedef struct classsymbol {
  SYMBOL_HEAD
  Vector supers;    /* supers in liner-oder */
  STable *stbl;     /* symbol table */
} ClassSymbol;

/* interface function */
typedef struct ifuncsymbol {
  SYMBOL_HEAD
  TypeDesc *desc; /* function's proto */
} IFuncSymbol;

/* native function */
typedef struct nfuncsymbol {
  SYMBOL_HEAD
  TypeDesc *desc; /* function's proto */
} NFuncSymbol;

/* import symbol */
typedef struct importsymbol {
  SYMBOL_HEAD
  void *import; /* Import, not need free */
} ImportSymbol;

STable *STable_New(void);
typedef void (*symbol_visit_func)(Symbol *sym, void *arg);
void STable_Free(STable *stbl, symbol_visit_func visit, void *arg);
#define STable_Free_Self(stbl) STable_Free(stbl, NULL, NULL)

Symbol *Symbol_New(SymKind kind, char *name);
void Symbol_Free(Symbol *sym);
#define Symbol_IsPublic(sym) isupper((sym)->name[0])

VarSymbol *STable_Add_Const(STable *stbl, char *name, TypeDesc *desc);
VarSymbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc);
FuncSymbol *STable_Add_Func(STable *stbl, char *name, TypeDesc *proto);
AliasSymbol *STable_Add_Alias(STable *stbl, char *name, TypeDesc *desc);
ClassSymbol *STable_Add_Class(STable *stbl, char *name);
ClassSymbol *STable_Add_Trait(STable *stbl, char *name);
IFuncSymbol *STable_Add_IFunc(STable *stbl, char *name, TypeDesc *proto);
NFuncSymbol *STable_Add_NFunc(STable *stbl, char *name, TypeDesc *proto);
ImportSymbol *STable_Add_Import(STable *stbl, char *name);
Symbol *STable_Get(STable *stbl, char *name);
KImage *Generate_KImage(STable *stbl);
void STable_Show(STable *stbl);
void STable_Visit(STable *stbl, symbol_visit_func fn, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
