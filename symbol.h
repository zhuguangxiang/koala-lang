
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "common.h"
#include "itemtable.h"

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
  int name_index;
  uint8 kind;
  uint8 access;
  uint16 unused;
  int desc_index;
  union {
    void *obj;  /* method or klass */
    int index;  /* variable's index of array */
  };
} Symbol;

typedef struct symboltable {
  HashTable *htable;
  ItemTable *itable;
} STable;

/* Exported APIs */
Symbol *Symbol_New(int name_index, int kind, int access, int desc_index);
#define Symbol_Set_Index(symbol, index) do { \
  (symbol)->index = (index); \
} while (0)
#define Symbol_Set_Object(symbol, object) do { \
  (symbol)->obj = (object); \
} while (0)
void Symbol_Free(Symbol *sym);

int STable_Init(STable *stbl);
void STable_Fini(STable *stbl);
Symbol *STable_Add_Var(STable *stbl, char *name, char *desc, int bconst);
Symbol *STable_Add_Func(STable *stbl, char *name, char *rdesc, char *pdesc);
Symbol *STable_Add_Klass(STable *stbl, char *name, int kind);
#define STable_Add_Class(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_CLASS); sym; \
})
#define STable_Add_Field(stbl, name, desc) ({ \
  Symbol *sym = STable_Add_Var(stbl, name, desc, 0); \
  sym->kind = SYM_FIELD; sym; \
})
#define STable_Add_Method(stbl, name, rdesc, pdesc) ({ \
  Symbol *sym = STable_Add_Func(stbl, name, rdesc, pdesc); \
  sym->kind = SYM_METHOD; sym; \
})
#define STable_Add_Interface(stbl, name) ({ \
  Symbol *sym = STable_Add_Klass(stbl, name, SYM_INTF); \
  sym->kind = SYM_INTF; sym; \
})
#define STable_Add_IProto(stbl, name, rdesc, pdesc) ({ \
  Symbol *sym = STable_Add_Func(stbl, name, rdesc, pdesc); \
  sym->kind = SYM_IPROTO; sym; \
})
Symbol *STable_Get(STable *stbl, char *name);

void Symbol_Display(Symbol *sym, STable *stbl);
void Symbol_Visit(HashList *head, int size, void *arg);
void STable_Display(STable *stbl);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
