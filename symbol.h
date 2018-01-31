
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "common.h"
#include "codeformat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYM_VAR     1
#define SYM_FUNC    2
#define SYM_CLASS   3
#define SYM_FIELD   4
#define SYM_METHOD  5
#define SYM_INTF    6
#define SYM_IPROTO  7

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  1
#define ACCESS_CONST    2

typedef struct symbol {
  HashNode hnode;
  struct list_head link;
  int name_index;
  int8 kind;
  int8 access;
  int16 refcnt; /* for compiler */
  int desc_index;
  union {
    void *obj;  /* method or klass */
    int index;  /* variable's index */
  };
} Symbol;

typedef struct symboltable {
  HashTable *htable;
  ItemTable *itable;
  int next_index;
  struct list_head head;
} STable;

/* Exported APIs */
Symbol *Symbol_New(int name_index, int kind, int access, int desc_index);
void Symbol_Free(Symbol *sym);
int STable_Init(STable *stbl, ItemTable *itable);
void STable_Fini(STable *stbl);
Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst);
Symbol *STable_Add_Func(STable *stbl, char *name, ProtoInfo *proto);
Symbol *STable_Add_Klass(STable *stbl, char *name, int kind);
#define STable_Add_Class(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_CLASS); sym; \
})
#define STable_Add_Field(stbl, name, desc) ({ \
  Symbol *sym = STable_Add_Var(stbl, name, desc, 0); \
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
