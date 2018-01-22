
#include "symbol.h"
#include "hash.h"
#include "debug.h"
#include "codeformat.h"

#define ATOM_ITEM_MAX   (ITEM_CONST + 1)

static uint32 htable_hash(void *key)
{
  ItemEntry *e = key;
  ASSERT(e->type > 0 && e->type < ATOM_ITEM_MAX);
  item_hash_t hash_fn = item_func[e->type].ihash;
  ASSERT_PTR(hash_fn);
  return hash_fn(e->data);
}

static int htable_equal(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  ASSERT(e1->type > 0 && e1->type < ATOM_ITEM_MAX);
  ASSERT(e2->type > 0 && e2->type < ATOM_ITEM_MAX);
  if (e1->type != e2->type) return 0;
  item_equal_t equal_fn = item_func[e1->type].iequal;
  ASSERT_PTR(equal_fn);
  return equal_fn(e1->data, e2->data);
}

ItemTable *SItemTable_Create(void)
{
  return ItemTable_Create(htable_hash, htable_equal, ATOM_ITEM_MAX);
}

/*-------------------------------------------------------------------------*/

Symbol *Symbol_New(int name_index, int kind, int access, int desc_index)
{
  Symbol *sym = calloc(1, sizeof(Symbol));
  init_hash_node(&sym->hnode, sym);
  sym->name_index = name_index;
  sym->kind = (uint8)kind;
  sym->access = (uint8)access;
  sym->desc_index = desc_index;
  return sym;
}

void Symbol_Free(Symbol *sym)
{
  free(sym);
}

uint32 symbol_hash(void *k)
{
  Symbol *s = k;
  return hash_uint32(s->name_index, 0);
}

int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return s1->name_index == s2->name_index;
}

HashTable *SHashTable_Create(void)
{
  return HashTable_Create(symbol_hash, symbol_equal);
}

/*-------------------------------------------------------------------------*/

static inline char *to_str(char *str[], int size, int idx)
{
  return (idx >= 0 && idx < size) ? str[idx] : "";
}

static char *type_tostr(int kind)
{
  static char *kind_str[] = {
    "", "var", "func", "class", "field", "method", "interface", "imethod"
  };

  return to_str(kind_str, nr_elts(kind_str), kind);
}

static char *access_tostr(int access)
{
  static char *str[] = {
    "public", "private"
  };

  return to_str(str, nr_elts(str), access);
}

static char *get_name(int index, ItemTable *itable)
{
  StringItem *item = ItemTable_Get(itable, ITEM_STRING, index);
  return item->data;
}

void Symbol_Display(Symbol *sym, ItemTable *itable)
{
  printf("%s %s %s\n",
         access_tostr(sym->access), type_tostr(sym->kind),
         get_name(sym->name_index, itable));
}
