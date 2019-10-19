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

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "object.h"
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
  SYM_LABEL  = 7,   /* enum label         */
  SYM_IFUNC  = 8,   /* interface method   */
  SYM_ANONY  = 9,   /* anonymous function */
  SYM_MOD    = 10,  /* (external) module  */
  SYM_REF    = 11,  /* reference symbol   */
  SYM_MAX
} SymKind;

typedef struct symbol Symbol;

/* symbol structure */
struct symbol {
  /* hash node */
  HashMapEntry hnode;
  /* SymKind */
  SymKind kind;
  /* symbol key */
  char *name;
  /* type descriptor */
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
      /* type object */
      Symbol *typesym;
      /* constant value */
      Literal value;
    } k;
    struct {
      /* type object */
      Symbol *typesym;
      /* variable index */
      int32_t index;
      /* dot access index(enum&tuple) */
      int dotindex;
    } var;
    struct {
      /* type parameters, only for in module */
      Vector *typeparas;
      /* local varibles in the function */
      Vector locvec;
      /* free variables' name */
      Vector freevec;
      /* CodeBlock */
      void *codeblock;
    } func;
    struct {
      /* type parameters */
      Vector *typeparas;
      /* bases in liner-oder */
      Vector bases;
      /* symbol table */
      STable *stbl;
    } type;
    struct {
      /* which enum is it? */
      Symbol *esym;
      /* associated types, TypeDesc */
      Vector *types;
    } label;
    struct {
      /* local varibles in the closure */
      Vector locvec;
      /* free values */
      Vector freevec;
      /* up values */
      Vector upvec;
      /* codeblock */
      void *codeblock;
    } anony;
    struct {
      /* module(no free) */
      void *ptr;
      /* atom(no free) */
      char *path;
    } mod;
    struct {
      /* dot import path */
      char *path;
      /* reference symbol in module */
      Symbol *sym;
      /* reference module */
      void *module;
    } ref;
  };
};

STable *stable_new(void);
void stable_free(STable *stbl);
Symbol *symbol_new(char *name, SymKind kind);
Symbol *stable_get(STable *stbl, char *name);
Symbol *stable_remove(STable *stbl, char *name);
int stable_add_symbol(STable *stbl, Symbol *sym);
Symbol *stable_add_const(STable *stbl, char *name, TypeDesc *desc);
Symbol *stable_add_var(STable *stbl, char *name, TypeDesc *desc);
Symbol *stable_add_func(STable *stbl, char *name, TypeDesc *proto);
Symbol *stable_add_ifunc(STable *stbl, char *name, TypeDesc *proto);
Symbol *stable_add_class(STable *stbl, char *name);
Symbol *stable_add_trait(STable *stbl, char *name);
Symbol *stable_add_enum(STable *stbl, char *name);
Symbol *stable_add_label(STable *stbl, char *name);
void symbol_decref(Symbol *sym);
void symbol_free(Symbol *sym);
STable *stable_from_mobject(Object *ob);
Symbol *type_find_mbr(Symbol *clsSym, char *name);
Symbol *enum_find_mbr(Symbol *eSym, char *name);
void fill_locvars(Symbol *sym, Vector *vec);
void free_locvars(Vector *locvec);
void type_write_image(Symbol *clssym, Image *image);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
