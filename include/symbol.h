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
  hashmap table;
  /* constant and variable allcated index */
  int varindex;
} symtable;

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
  hashmapentry hnode;
  /* SymKind */
  SymKind kind;
  /* symbol key */
  char *name;
  /* type descriptor */
  typedesc *desc;
  /* type object */
  struct symbol *sym;
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
      /* constant value */
      literal value;
    } k;
    struct {
      /* free var */
      int freevar;
      /* variable index */
      int32_t index;
      /* constant value */
      literal value;
    } var;
    struct {
      /* type parameters, only for in module */
      vector *typeparas;
      /* local varibles in the function */
      vector locvec;
      /* codeblock */
      void *code;
    } func;
    struct {
      /* type parameters */
      vector *typeparas;
      /* bases in liner-oder */
      vector bases;
      /* symbol table */
      symtable *stbl;
    } klass;
    struct {
      /* parameter types */
      vector *typeparas;
      /* symbol table for enum value and func */
      symtable *stbl;
    } em;
    struct {
      /* which enum is it? */
      struct symbol *esym;
      /* associated types, typedesc */
      vector *descs;
      vector *syms;
    } ev;
    struct {
      /* local varibles in the closure */
      vector locvec;
      /* up local variables */
      vector uplocvec;
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
} symbol;

symtable *stable_new(void);
void stable_free(symtable *stbl);
symbol *symbol_new(char *name, SymKind kind);
symbol *stable_get(symtable *stbl, char *name);
symbol *stable_add_const(symtable *stbl, char *name, typedesc *desc);
symbol *stable_add_var(symtable *stbl, char *name, typedesc *desc);
symbol *stable_add_func(symtable *stbl, char *name, typedesc *proto);
void symbol_decref(symbol *sym);
symtable *stable_from_mobject(Object *ob);
symbol *klass_find_member(symbol *clsSym, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
