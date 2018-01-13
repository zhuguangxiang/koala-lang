
#include "itemtable.h"

ItemTable *ItemTable_Create(ht_hash_func hash, ht_equal_func equal, int size)
{
  ItemTable *table = malloc(sizeof(ItemTable) + size * sizeof(struct vector));
  ItemTable_Init(table, hash, equal, size);
  return table;
}

static ItemEntry *itementry_new(int type, int index, void *data)
{
  ItemEntry *e = malloc(sizeof(*e));
  init_hash_node(&e->hnode, e);
  e->type  = type;
  e->index = index;
  e->data  = data;
  return e;
}

int ItemTable_Init(ItemTable *table,
                   ht_hash_func hash, ht_equal_func equal, int size)
{
  HashTable_Init(&table->table, hash, equal);
  table->size = size;
  for (int i = 0; i < size; i++)
    Vector_Init(table->items + i, 0);
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
  int index = Vector_Set(vec, Vector_Size(vec), data);
  if (unique) {
    ItemEntry *e = itementry_new(type, index, *(void **)data);
    int res = HashTable_Insert(&table->table, &e->hnode);
    assert(!res);
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
  assert(type >= 0 && type < table->size);
  void **data = Vector_Get(table->items + type, index);
  return *data;
}

int ItemTable_Size(ItemTable *table, int type)
{
  assert(type >= 0 && type < table->size);
  return Vector_Size(table->items + type);
}
