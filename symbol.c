
#include "symbol.h"
#include "hash.h"
#include "log.h"

#define SYM_ATOM_MAX (ITEM_CONST + 1)

static uint32 symbol_hash(void *k)
{
  Symbol *s = k;
  return hash_uint32(s->nameindex, 0);
}

static int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return s1->nameindex == s2->nameindex;
}

static HashTable *__get_hashtable(STable *stbl)
{
  if (stbl->htable == NULL) {
    Decl_HashInfo(hashinfo, symbol_hash, symbol_equal);
    stbl->htable = HashTable_New(&hashinfo);
  }
  return stbl->htable;
}

int STable_Init(STable *stbl, AtomTable *atable)
{
  stbl->htable = NULL;
  Decl_HashInfo(hashinfo, item_hash, item_equal);
  if (atable == NULL)
    stbl->atable = AtomTable_New(&hashinfo, SYM_ATOM_MAX);
  else
    stbl->atable = atable;
  stbl->nextindex = 0;
  return 0;
}

void STable_Fini(STable *stbl)
{
  UNUSED_PARAMETER(stbl);
}

Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst)
{
  int access = isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int descindex = -1;
  if (desc != NULL) {
    descindex = TypeItem_Set(stbl->atable, desc);
    ASSERT(descindex >= 0);
  }
  Symbol *sym = STable_Add_Symbol(stbl, SYM_VAR, access, name, descindex);
  if (sym == NULL) return NULL;
  sym->index = stbl->nextindex++;
  sym->bconst = bconst;
  return sym;
}

Symbol *STable_Add_Func(STable *stbl, char *name, ProtoInfo *proto)
{
  int access = isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int descindex = ProtoItem_Set(stbl->atable, proto);
  ASSERT(descindex >= 0);
  return STable_Add_Symbol(stbl, SYM_FUNC, access, name, descindex);
}

int FuncSym_Get_Proto(STable *stbl, Symbol *sym, ProtoInfo *proto)
{
  ASSERT(sym->kind == SYM_FUNC);
  ProtoItem *item = AtomTable_Get(stbl->atable, ITEM_PROTO, sym->descindex);
  ASSERT_PTR(item);
  return ProtoItem_ToProto(stbl->atable, item, proto);
}

Symbol *STable_Add_Klass(STable *stbl, char *name, int kind)
{
  int access = isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  return STable_Add_Symbol(stbl, kind, access, name, -1);
}

Symbol *STable_Add_Symbol(STable *stbl, int kind, int access,
                          char *name, int descindex)
{
  int nameindex = StringItem_Set(stbl->atable, name);
  ASSERT(nameindex >= 0);
  Symbol *sym = Symbol_New(nameindex, kind, access, descindex);
  if (HashTable_Insert(__get_hashtable(stbl), &sym->hnode) < 0) {
    Symbol_Free(sym);
    return NULL;
  }
  return sym;
}

Symbol *STable_Get(STable *stbl, char *name)
{
  int index = StringItem_Get(stbl->atable, name);
  if (index < 0) return NULL;
  Symbol sym = {.nameindex = index};
  HashNode *hnode = HashTable_Find(__get_hashtable(stbl), &sym);
  if (hnode != NULL) {
    return container_of(hnode, Symbol, hnode);
  } else {
    return NULL;
  }
}

Symbol *Symbol_New(int nameindex, int kind, int access, int descindex)
{
  Symbol *sym = calloc(1, sizeof(Symbol));
  Init_HashNode(&sym->hnode, sym);
  sym->nameindex = nameindex;
  sym->kind = (uint8)kind;
  sym->access = (uint8)access;
  sym->descindex = descindex;
  return sym;
}

void Symbol_Free(Symbol *sym)
{
  free(sym);
}

/*-------------------------------------------------------------------------*/

static char *name_tostr(int index, STable *stbl)
{
  if (index < 0) return "";
  StringItem *item = AtomTable_Get(stbl->atable, ITEM_STRING, index);
  ASSERT_PTR(item);
  return item->data;
}

static void desc_show(int index, STable *stbl)
{
  if (index < 0) return;

  static char ch[] = {
    'i', 'f', 'z', 's', 'A'
  };

  static char *str[] = {
    "int", "float", "bool", "string", "Any"
  };

  TypeItem *type = AtomTable_Get(stbl->atable, ITEM_TYPE, index);
  ASSERT_PTR(type);
  int dims = type->dims;
  while (dims-- > 0) printf("[]");
  if (type->kind == TYPE_PRIMITIVE) {
    for (int i = 0; nr_elts(ch); i++) {
      if (ch[i] == type->primitive) {
        printf("%s", str[i]); return;
      }
    }
  } else {
    StringItem *item = AtomTable_Get(stbl->atable, ITEM_STRING, type->index);
    ASSERT_PTR(item);
    char *s = item->data;
    ASSERT_PTR(s);
    printf("%s", s);
  }
}

static void symbol_var_show(Symbol *sym, STable *stbl)
{
  /* show's format: "type name desc;" */
  char *type = sym->bconst ? "const":"var";
  printf("%s %s ", type, name_tostr(sym->nameindex, stbl));
  desc_show(sym->descindex, stbl);
  puts(";"); /* with newline */
}

static void typelist_show(TypeListItem *typelist, STable *stbl)
{
  int sz = typelist->size;
  for (int i = 0; i < sz; i++) {
    desc_show(typelist->index[i], stbl);
    if (i < sz - 1) printf(", ");
  }
}

static void proto_show(Symbol *sym, STable *stbl)
{
  int index = sym->descindex;
  if (index < 0) return;
  ProtoItem *proto = AtomTable_Get(stbl->atable, ITEM_PROTO, index);
  ASSERT_PTR(proto);

  TypeListItem *typelist;
  if (proto->pindex >= 0) {
    typelist = AtomTable_Get(stbl->atable, ITEM_TYPELIST, proto->pindex);
    ASSERT_PTR(typelist);
    printf("(");
    typelist_show(typelist, stbl);
    printf(")");
  } else {
    printf("()");
  }

  if (proto->rindex >= 0) {
    printf(" ");
    typelist = AtomTable_Get(stbl->atable, ITEM_TYPELIST, proto->rindex);
    ASSERT_PTR(typelist);
    if (typelist->size > 1) printf("(");
    typelist_show(typelist, stbl);
    if (typelist->size > 1) printf(")");
  }

  puts(";");  /* with newline */
}

static void symbol_func_show(Symbol *sym, STable *stbl)
{
  /* show's format: "func name args rets;" */
  printf("func %s", name_tostr(sym->nameindex, stbl));
  proto_show(sym, stbl);
}

static void symbol_class_show(Symbol *sym, STable *stbl)
{
  printf("class %s;\n", name_tostr(sym->nameindex, stbl));
}

static void symbol_intf_show(Symbol *sym, STable *stbl)
{
  printf("interface %s;\n", name_tostr(sym->nameindex, stbl));
}

typedef void (*symbol_show_func)(Symbol *sym, STable *stbl);

static symbol_show_func show_funcs[] = {
  NULL,
  symbol_var_show,
  symbol_func_show,
  symbol_class_show,
  symbol_intf_show
};

static void symbol_show(HashNode *hnode, void *arg)
{
  Symbol *sym = container_of(hnode, Symbol, hnode);
  STable *stbl = arg;
  symbol_show_func show = show_funcs[sym->kind];
  ASSERT_PTR(show);
  show(sym, stbl);
}

void STable_Show(STable *stbl, int showAtom)
{
  HashTable_Traverse(stbl->htable, symbol_show, stbl);
  if (showAtom) AtomTable_Show(stbl->atable, SYM_ATOM_MAX);
}
