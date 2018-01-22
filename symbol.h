
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "common.h"
#include "itemtable.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYM_VAR       1
#define SYM_FUNC      2
#define SYM_CLASS     3
#define SYM_FIELD     4
#define SYM_METHOD    5
#define SYM_INTF      6
#define SYM_IMETHOD   7

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

/* Exported APIs */
Symbol *Symbol_New(int name_index, int kind, int access, int desc_index);
void Symbol_Free(Symbol *sym);
void Symbol_Display(Symbol *sym, ItemTable *table);
ItemTable *SItemTable_Create(void);
HashTable *SHashTable_Create(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
