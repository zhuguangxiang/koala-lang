
#ifndef _KOALA_ITEMTABLE_H_
#define _KOALA_ITEMTABLE_H_

#include "hashtable.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  HashNode hnode;
  int type;
  int index;
  void *data;
} ItemEntry;

#define ITEM_ENTRY_INIT(t, i, d)  {.type = (t), .index = (i), .data = (d)}

typedef struct item_table {
  HashTable table;
  int size;
  Vector items[0];
} ItemTable;

typedef void (*item_fini_t)(int type, void *data, void *arg);
ItemTable *ItemTable_Create(ht_hash_func hash, ht_equal_func equal, int size);
void ItemTable_Destroy(ItemTable *table, item_fini_t fini, void *arg);
int ItemTable_Initialize(ItemTable *table,
                         ht_hash_func hash, ht_equal_func equal, int size);
void ItemTable_Finalize(ItemTable *table, item_fini_t fini, void *arg);
int ItemTable_Append(ItemTable *table, int type, void *data, int unique);
int ItemTable_Index(ItemTable *table, int type, void *data);
void *ItemTable_Get(ItemTable *table, int type, int index);
int ItemTable_Size(ItemTable *table, int type);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ITEMTABLE_H_ */
