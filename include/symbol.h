/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "image.h"
#include "object.h"

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
  SYM_MOD    = 11,  /* (external) module  */
  SYM_REF    = 12,  /* reference symbol   */
  SYM_MAX
} SymKind;

/* symbol structure */
typedef struct symbol {
  /* hash node */
  HashMapEntry hnode;
  /* SymKind */
  SymKind kind;
  /* symbol key */
  char *name;
  /* symbol type */
  TypeDesc *desc;
  /* filename */
  char *filename;
  /* position */
  short row;
  short col;
  /* refcnt */
  int refcnt;
  /* used */
  int used;
  union {
    struct {
      /* variable index */
      int32_t index;
      /* if is constant, save its value */
      ConstValue value;
      /* symbol's owner, module or class */
      struct symbol *owner;
      /* variable's stbl(typeparas), not need free */
      union {
        STable *stbl;
        Vector *paras;
      };
    } var;
    struct {
      /* symbol's owner, module or class */
      struct symbol *owner;
      /* type parameters, only for in module */
      Vector *typeparas;
      /* local varibles in the function */
      Vector locvec;
      /* CodeBlock */
      void *code;
    } func;
    struct {
      /* type parameters */
      Vector *typeparas;
      /* supers in liner-oder */
      Vector supers;
      /* symbol table */
      STable *stbl;
    } klass;
    struct {
      /* parameter types */
      Vector *typeparas;
      /* symbol table for EnumSymbol and FuncSymbol */
      STable *stbl;
    } em;
    struct {
      /* which enum is it? */
      struct symbol *esym;
      /* associated types, TypeDesc */
      Vector *types;
    } ev;
    struct {
      /* local varibles in the closure */
      Vector locvec;
      /* up local variables */
      Vector uplocvec;
      /* codeblock */
      void *code;
    } anony;
    struct {
      /* module, not need free */
      void *ptr;
    } mod;
    struct {
      /* dot import path */
      char *path;
      /* reference symbol in module */
      struct symbol *sym;
      /* reference module */
      void *module;
    } ref;
  };
} Symbol;

STable *stable_new(void);
void stable_free(STable *stbl);
Symbol *symbol_new(char *name, SymKind kind);
Symbol *stable_get(STable *stbl, char *name);
Symbol *stable_add_const(STable *stbl, char *name, TypeDesc *desc);
Symbol *stable_add_var(STable *stbl, char *name, TypeDesc *desc);
Symbol *stable_add_func(STable *stbl, char *name, TypeDesc *proto);
void symbol_decref(Symbol *sym);
STable *stable_from_mobject(Object *ob);
Symbol *klass_find_member(Symbol *clsSym, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
