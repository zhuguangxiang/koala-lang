/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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
  HashMap table;
  /* constant and variable allcated index */
  int varindex;
} STable;

/* symbol kind */
typedef enum symbolkind {
  SYM_CONST  = 1,   /* constant           */
  SYM_VAR    = 2,   /* variable           */
  SYM_FUNC   = 3,   /* function or method */
  SYM_CLASS  = 4,   /* class              */
  SYM_TRAIT  = 5,   /* trait              */
  SYM_ENUM   = 6,   /* enum               */
  SYM_EVAL   = 7,   /* enum value         */
  SYM_IFUNC  = 8,   /* interface method   */
  SYM_NFUNC  = 9,   /* native function    */
  SYM_AFUNC  = 10,  /* anonymous function */
  SYM_PKG    = 11,  /* (external) package */
  SYM_REF    = 12,  /* reference symbol   */
  SYM_MAX
} SymKind;

/* source code token position for error handler */
typedef struct position {
  int row;
  int col;
} Position;

#define SYMBOL_HEAD   \
  /* hash node */     \
  HashMapEntry hnode; \
  /* SymKind */       \
  SymKind kind;       \
  /* symbol key */    \
  char *name;         \
  /* symbol type */   \
  TypeDesc *desc;     \
  /* filename */      \
  char *filename;     \
  /* position */      \
  Position pos;       \
  /* refcnt */        \
  int refcnt;

/* symbol structure */
typedef struct symbol {
  SYMBOL_HEAD
} Symbol;

/* const and variable symbol */
typedef struct varsymbol {
  SYMBOL_HEAD
  /* variable index */
  int32_t index;
  /* if is constant, save its value */
  ConstValue value;
  /* symbol's owner, module or class */
  Symbol *owner;
  /* variable's stbl(stables of typeparas), not need free */
  union {
    STable *stbl;
    Vector *vec;
  };
} VarSymbol;

/* function symbol */
typedef struct funcsymbol {
  SYMBOL_HEAD
  /* symbol's owner, module or class */
  Symbol *owner;
  /* type parameters, only for in module */
  Vector *typeparas;
  /* local varibles in the function */
  Vector locvec;
  /* CodeBlock */
  void *code;
} FuncSymbol;

/* class and trait symbol */
typedef struct classsymbol {
  SYMBOL_HEAD
  /* type parameters */
  Vector *typeparas;
  /* supers in liner-oder */
  Vector supers;
  /* symbol table */
  STable *stbl;
} ClassSymbol;

/* enum symbol */
typedef struct enumsymbol {
  SYMBOL_HEAD
  /* parameter types */
  Vector *typeparas;
  /* symbol table for EnumSymbol and FuncSymbol */
  STable *stbl;
} EnumSymbol;

/* enum value symbol */
typedef struct enumvalsymbol {
  SYMBOL_HEAD
  /* which enum is it? */
  EnumSymbol *esym;
  /* associated types, TypeDesc */
  Vector *types;
} EnumValSymbol;

/* anonoymous function */
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
  /* reference symbol in module */
  Symbol *sym;
  /* reference module */
  void *module;
} RefSymbol;

STable *STable_New(void);
void STable_Free(STable *stbl);

Symbol *Symbol_New(SymKind kind, char *name);
void Symbol_DecRef(Symbol *sym);

VarSymbol *STable_Add_Const(STable *stbl, char *name, TypeDesc *desc);
VarSymbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc);
FuncSymbol *STable_Add_Func(STable *stbl, char *name, TypeDesc *proto);

#if 0
ClassSymbol *STable_Add_Class(STable *stbl, char *name);
ClassSymbol *STable_Add_Trait(STable *stbl, char *name);
EnumSymbol *STable_Add_Enum(STable *stbl, char *name);
EnumValSymbol *STable_Add_EnumValue(STable *stbl, char *name);
Symbol *STable_Add_Proto(STable *stbl, char *name, int kind, TypeDesc *desc);
#define STable_Add_IFunc(stbl, name, proto) \
  STable_Add_Proto(stbl, name, SYM_IFUNC, proto)
#define STable_Add_NFunc(stbl, name, proto) \
  STable_Add_Proto(stbl, name, SYM_NFUNC, proto)
AFuncSymbol *STable_Add_Anonymous(STable *stbl, TypeDesc *desc);
PkgSymbol *STable_Add_Package(STable *stbl, char *name);
RefSymbol *STable_Add_Reference(STable *stbl, char *name);
Symbol *STable_Get(STable *stbl, char *name);
KImage *Gen_Image(STable *stbl, char *pkgname);
int STable_From_Image(char *path, char **pkgname, STable **stbl);
void STable_Show(STable *stbl);
void STable_Visit(STable *stbl, symbol_visit_func fn, void *arg);
void __Copy_Symbol_Func(Symbol *sym, STable *to);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
