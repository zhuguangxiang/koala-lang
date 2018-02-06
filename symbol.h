
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "codeformat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYM_VAR     1
#define SYM_FUNC    2
#define SYM_CLASS   3
#define SYM_INTF    4
#define SYM_IFUNC   5

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  1

typedef struct symbol {
  HashNode hnode;
  int nameindex;
  int8 kind;
  int8 access;
  int8 bconst;
  int8 refcnt; /* for compiler */
  int descindex;
  union {
    void *obj;  /* method or klass */
    int index;  /* variable's index */
  };
} Symbol;

typedef struct symboltable {
  HashTable *htable;
  AtomTable *atable;
  int nextindex;
} STable;

/* Exported APIs */
Symbol *Symbol_New(int nameindex, int kind, int access, int descindex);
void Symbol_Free(Symbol *sym);
int STable_Init(STable *stbl, AtomTable *atable);
void STable_Fini(STable *stbl);
Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst);
Symbol *STable_Add_Func(STable *stbl, char *name, ProtoInfo *proto);
Symbol *STable_Add_Klass(STable *stbl, char *name, int kind);
#define STable_Add_Class(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_CLASS); sym; \
})
#define STable_Add_Interface(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_INTF); \
  sym->kind = SYM_INTF; sym; \
})
#define STable_Add_IFunc(stbl, name, proto) ({ \
  Symbol *sym = STable_Add_Func(stbl, name, proto); \
  sym->kind = SYM_IFUNC; sym; \
})
Symbol *STable_Get(STable *stbl, char *name);
void STable_Show(STable *stbl, int showAtom);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
