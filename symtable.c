
#include "symtable.h"
#include "hash.h"

struct symbol *new_symbol(char *name, enum symbol_kind kind)
{
  struct symbol *sym = malloc(sizeof(*sym));
  sym->name = name;
  sym->kind = kind;
  init_hash_node(&sym->hnode, sym->name);
  return sym;
}

static uint32_t sym_hash(void *o)
{
  return hash_string((char *)o);
}

static int sym_equal(void *o1, void *o2)
{
  return (strcmp((char *)o1, (char *)o2) == 0) ? 1 : 0;
}

struct symtable *new_symtable(void)
{
  struct symtable *tbl = malloc(sizeof(*tbl));
  tbl->level = -1;
  tbl->vec = vector_create();
  return tbl;
}

void free_symtable(struct symtable *tbl)
{
  struct hash_table *htbl;
  for (int i = 0; i < tbl->level; i++) {
    htbl = vector_get(tbl->vec, i);
    hash_table_destroy(htbl, NULL, NULL);
  }
  vector_destroy(tbl->vec, NULL, NULL);
  free(tbl);
}

int symtable_begin_scope(struct symtable *tbl)
{
  ++tbl->level;
  vector_set(tbl->vec, tbl->level, hash_table_create(sym_hash, sym_equal));
  return 0;
}

int symtable_end_scope(struct symtable *tbl)
{
  int index = tbl->level;
  struct hash_table *htbl = vector_get(tbl->vec, index);
  hash_table_destroy(htbl, NULL, NULL);
  if (tbl->level > 0)
    --tbl->level;
  return 0;
}

int symtable_add(struct symtable *tbl, struct symbol *sym)
{
  struct hash_table *htbl = vector_get(tbl->vec, tbl->level);
  assert(htbl != NULL);
  return hash_table_insert(htbl, &sym->hnode);
}

struct symbol *symtable_find(struct symtable *tbl, void *key)
{
  struct hash_node *node;
  struct hash_table *htbl = vector_get(tbl->vec, tbl->level);
  assert(htbl != NULL);
  node = hash_table_find(htbl, key);
  return (node != NULL) ? container_of(node, struct symbol, hnode) : NULL;
}
