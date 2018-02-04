
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "codeformat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYM_VAR     1
#define SYM_CONST   2
#define SYM_FUNC    3
#define SYM_CLASS   4
#define SYM_FIELD   5
#define SYM_METHOD  6
#define SYM_INTF    7
#define SYM_IPROTO  8

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  1

typedef struct symbol {
  HashNode hnode;
  int nameindex;
  int8 kind;
  int8 access;
  int16 refcnt; /* for compiler */
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
Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc);
#define STable_Add_Const(stbl, name, desc) ({ \
  Symbol *sym = STable_Add_Var(stbl, name, desc); \
  sym->kind = SYM_CONST; sym; \
})
Symbol *STable_Add_Func(STable *stbl, char *name, ProtoInfo *proto);
Symbol *STable_Add_Klass(STable *stbl, char *name, int kind);
#define STable_Add_Class(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_CLASS); sym; \
})
#define STable_Add_Field(stbl, name, desc) ({ \
  Symbol *sym = STable_Add_Var(stbl, name, desc); \
  sym->kind = SYM_FIELD; sym; \
})
#define STable_Add_Method(stbl, name, proto) ({ \
  Symbol *sym = STable_Add_Func(stbl, name, proto); \
  sym->kind = SYM_METHOD; sym; \
})
#define STable_Add_Interface(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_INTF); \
  sym->kind = SYM_INTF; sym; \
})
#define STable_Add_IProto(stbl, name, proto) ({ \
  Symbol *sym = STable_Add_Func(stbl, name, proto); \
  sym->kind = SYM_IPROTO; sym; \
})
Symbol *STable_Get(STable *stbl, char *name);
void STable_Show(STable *stbl);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
