
#include "symbol.h"
#include "hash.h"
#include "log.h"

Symbol *symbol_new(void)
{
  Symbol *sym = calloc(1, sizeof(Symbol));
  Init_HashNode(&sym->hnode, sym);
  return sym;
}

void symbol_free(Symbol *sym)
{
  free(sym);
}

static uint32 symbol_hash(void *k)
{
  Symbol *s = k;
  return hash_uint32(s->name, 0);
}

static int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return s1->name == s2->name;
}

#define SYMBOL_ACCESS(name) \
  (isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE)

/*-------------------------------------------------------------------------*/

#define SYM_ATOM_MAX (ITEM_CONST + 1)

static HashTable *__get_hashtable(SymTable *stbl)
{
  if (stbl->htbl == NULL) {
    Decl_HashInfo(hashinfo, symbol_hash, symbol_equal);
    stbl->htbl = HashTable_New(&hashinfo);
  }
  return stbl->htbl;
}

int STbl_Init(SymTable *stbl, AtomTable *atbl)
{
  stbl->htbl = NULL;
  if (atbl == NULL) {
    Decl_HashInfo(hashinfo, item_hash, item_equal);
    stbl->atbl = AtomTable_New(&hashinfo, SYM_ATOM_MAX);
  } else {
    stbl->atbl = atbl;
  }
  stbl->next = 0;
  return 0;
}

void STbl_Fini(SymTable *stbl)
{
  UNUSED_PARAMETER(stbl);
}

Symbol *STbl_Add_Var(SymTable *stbl, char *name, TypeDesc *desc, bool konst)
{
  Symbol *sym = STbl_Add_Symbol(stbl, name, SYM_VAR, konst);
  if (sym == NULL) return NULL;

  idx_t idx = -1;
  if (desc != NULL) {
    idx = TypeItem_Set(stbl->atbl, desc);
    ASSERT(idx >= 0);
  }
  sym->desc  = idx;
  sym->type  = desc;
  sym->index = stbl->next++;
  return sym;
}

Symbol *STbl_Add_Proto(SymTable *stbl, char *name, ProtoInfo *proto)
{
  Symbol *sym = STbl_Add_Symbol(stbl, name, SYM_PROTO, 0);
  if (sym == NULL) return NULL;

  idx_t idx = ProtoItem_Set(stbl->atbl, proto);
  ASSERT(idx >= 0);
  sym->desc = idx;
  TypeDesc *type = TypeDesc_New(TYPE_PROTO);
  type->proto = ProtoInfo_Dup(proto);
  sym->type = type;
  return sym;
}

Symbol *STbl_Add_IProto(SymTable *stbl, char *name, ProtoInfo *proto)
{
  Symbol *sym = STbl_Add_Proto(stbl, name, proto);
  if (sym == NULL) return NULL;
  sym->kind = SYM_IPROTO;
  return sym;
}

Symbol *STbl_Add_Symbol(SymTable *stbl, char *name, int kind, bool konst)
{
  Symbol *sym = symbol_new();
  idx_t idx = StringItem_Set(stbl->atbl, name);
  ASSERT(idx >= 0);
  sym->name = idx;
  if (HashTable_Insert(__get_hashtable(stbl), &sym->hnode) < 0) {
    symbol_free(sym);
    return NULL;
  }
  sym->kind = kind;
  sym->konst = konst;
  sym->access = SYMBOL_ACCESS(name);
  sym->str = name;
  return sym;
}

Symbol *STbl_Get(SymTable *stbl, char *name)
{
  idx_t index = StringItem_Get(stbl->atbl, name);
  if (index < 0) return NULL;
  Symbol sym = {.name = index};
  HashNode *hnode = HashTable_Find(__get_hashtable(stbl), &sym);
  if (hnode != NULL) {
    return container_of(hnode, Symbol, hnode);
  } else {
    return NULL;
  }
}

/*-------------------------------------------------------------------------*/

static void desc_show(TypeDesc *type)
{
  char *str = TypeDesc_ToString(type);
  printf("%s", str);
  if (type->kind == TYPE_USERDEF)
    free(str);
}

// static void proto_show(Symbol *sym, SymTable *stbl)
// {
//   int index = sym->desc;
//   if (index < 0) return;
//   ProtoItem *proto = AtomTable_Get(stbl->atbl, ITEM_PROTO, index);
//   ASSERT_PTR(proto);

//   TypeListItem *typelist;
//   if (proto->pindex >= 0) {
//     typelist = AtomTable_Get(stbl->atbl, ITEM_TYPELIST, proto->pindex);
//     ASSERT_PTR(typelist);
//     printf("(");
//     typelist_show(typelist, stbl);
//     printf(")");
//   } else {
//     printf("()");
//   }

//   if (proto->rindex >= 0) {
//     printf(" ");
//     typelist = AtomTable_Get(stbl->atbl, ITEM_TYPELIST, proto->rindex);
//     ASSERT_PTR(typelist);
//     if (typelist->size > 1) printf("(");
//     typelist_show(typelist, stbl);
//     if (typelist->size > 1) printf(")");
//   }

//   puts(";");  /* with newline */
// }

static void symbol_show(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  Symbol *sym = container_of(hnode, Symbol, hnode);
  switch (sym->kind) {
    case SYM_VAR: {
      /* show's format: "type name desc;" */
      printf("%s %s ", sym->konst ? "const":"var", sym->str);
      desc_show(sym->type);
      puts(";"); /* with newline */
      break;
    }
    case SYM_PROTO: {
      /* show's format: "func name args rets;" */
      printf("func %s", sym->str);
      desc_show(sym->type);
      break;
    }
    case SYM_CLASS: {
      printf("class %s;\n", sym->str);
      break;
    }
    case SYM_INTF: {
      printf("interface %s;\n", sym->str);
      break;
    }
    default: {
      ASSERT(0);
      break;
    }
  }
}

void STbl_Show(SymTable *stbl, int detail)
{
  HashTable_Traverse(stbl->htbl, symbol_show, stbl);
  if (detail) AtomTable_Show(stbl->atbl);
}
