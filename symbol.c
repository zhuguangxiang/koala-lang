
#include "hash.h"
#include "symbol.h"
#include "debug.h"

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

uint32 Symbol_Hash(void *k)
{
  Symbol *s = k;
  return hash_uint32(s->name_index, 0);
}

int Symbol_Equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return s1->name_index == s2->name_index;
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
