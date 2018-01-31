
#include "itemtable.h"

ItemTable *ItemTable_Create(HashInfo *hashinfo, int size)
{
  ItemTable *table = malloc(sizeof(ItemTable) + size * sizeof(Vector));
  ItemTable_Init(table, hashinfo, size);
  return table;
}

static ItemEntry *itementry_new(int type, int index, void *data)
{
  ItemEntry *e = malloc(sizeof(ItemEntry));
  Init_HashNode(&e->hnode, e);
  e->type  = type;
  e->index = index;
  e->data  = data;
  return e;
}

int ItemTable_Init(ItemTable *table, HashInfo *hashinfo, int size)
{
  HashTable_Init(&table->table, hashinfo);
  table->size = size;
  for (int i = 0; i < size; i++)
    Vector_Init(table->items + i);
  return 0;
}

typedef struct item_data {
  item_fini_t fini;
  void *arg;
  int type;
} ItemData;

static void item_fini(void *data, void *arg)
{
  ItemData *itemdata = arg;
  itemdata->fini(itemdata->type, data, itemdata->arg);
}

void ItemTable_Fini(ItemTable *table, item_fini_t fini, void *arg)
{
  ItemData itemdata = {fini, arg, 0};
  for (int i = 0; i < table->size; i++) {
    itemdata.type = i;
    Vector_Fini(table->items + i, item_fini, &itemdata);
  }
}

int ItemTable_Append(ItemTable *table, int type, void *data, int unique)
{
  Vector *vec = table->items + type;
  int index = Vector_Append(vec, data);
  if (unique) {
    ItemEntry *e = itementry_new(type, index, data);
    int res = HashTable_Insert(&table->table, &e->hnode);
    ASSERT(!res);
  }
  return index;
}

int ItemTable_Index(ItemTable *table, int type, void *data)
{
  ItemEntry e = ITEM_ENTRY_INIT(type, 0, data);
  HashNode *hnode = HashTable_Find(&table->table, &e);
  if (hnode == NULL) return -1;
  return container_of(hnode, ItemEntry, hnode)->index;
}

void *ItemTable_Get(ItemTable *table, int type, int index)
{
  ASSERT(type >= 0 && type < table->size);
  return Vector_Get(table->items + type, index);
}

int ItemTable_Size(ItemTable *table, int type)
{
  ASSERT(type >= 0 && type < table->size);
  return Vector_Size(table->items + type);
}
