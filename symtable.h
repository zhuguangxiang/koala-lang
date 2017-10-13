
#ifndef _KOALA_SYMTABLE_H_
#define _KOALA_SYMTABLE_H_

#include "vector.h"
#include "hash_table.h"

#ifdef __cplusplus
extern "C" {
#endif

struct symtable {
  int level;
  struct vector *vec;
};

enum symbol_kind {
  MODREF_KIND = 1, VAR_KIND = 2, FUNC_KIND = 3, CLASS_KIND = 4,
};

struct symbol {
  struct hash_node hnode;
  char *name;
  int scope;
  enum symbol_kind kind;
  union {
    char *mod_path;
  } v;
};

struct symbol *new_symbol(char *name, enum symbol_kind kind);
struct symtable *new_symtable(void);
void free_symtable(struct symtable *tbl);
int symtable_begin_scope(struct symtable *tbl);
int symtable_end_scope(struct symtable *tbl);
int symtable_add(struct symtable *tbl, struct symbol *sym);
struct symbol *symtable_find(struct symtable *tbl, void *key);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMTABLE_H_ */
