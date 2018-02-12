
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "codeformat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYM_VAR     1
#define SYM_PROTO   2
#define SYM_CLASS   3
#define SYM_INTF    4
#define SYM_IPROTO  5
#define SYM_STABLE  6

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  1

typedef struct symbol {
  HashNode hnode;
  idx_t name;
  int8 kind;
  int8 access;
  bool konst;
  int8 refcnt;    /* for compiler */
  idx_t desc;     /* variable's type or func's proto */
  char *str;      /* -> name */
  TypeDesc *type; /* -> desc */
  union {
    void *obj;    /* method or klass */
    idx_t index;  /* variable's index */
  };
} Symbol;

typedef struct symboltable {
  HashTable *htbl;
  AtomTable *atbl;
  idx_t next;
} SymTable;

/* Exported APIs */
int STbl_Init(SymTable *stbl, AtomTable *atbl);
void STbl_Fini(SymTable *stbl);
SymTable *STbl_New(AtomTable *atbl);
void STbl_Free(SymTable *stbl);
Symbol *STbl_Add_Var(SymTable *stbl, char *name, TypeDesc *desc, bool konst);
Symbol *STbl_Add_Proto(SymTable *stbl, char *name, Proto *proto);
Symbol *STbl_Add_IProto(SymTable *stbl, char *name, Proto *proto);
#define STbl_Add_Class(stbl, name) STbl_Add_Symbol(stbl, name, SYM_CLASS, 0)
#define STbl_Add_Intf(stbl, name) STbl_Add_Symbol(stbl, name, SYM_INTF, 0)
Symbol *STbl_Add_Symbol(SymTable *stbl, char *name, int kind, bool konst);
Symbol *STbl_Get(SymTable *stbl, char *name);
typedef void (*symbolfunc)(Symbol *sym, void *arg);
void STbl_Traverse(SymTable *stbl, symbolfunc fn, void *arg);
void STbl_Show(SymTable *stbl, int detail);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
