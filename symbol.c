
#include "symbol.h"
#include "hash.h"
#include "debug.h"

static uint32 symbol_hash(void *k)
{
  Symbol *s = k;
  return hash_uint32(s->name_index, 0);
}

static int symbol_equal(void *k1, void *k2)
{
  Symbol *s1 = k1;
  Symbol *s2 = k2;
  return s1->name_index == s2->name_index;
}

static HashTable *__get_hashtable(STable *stbl)
{
  if (stbl->htable == NULL) {
    Decl_HashInfo(hashinfo, symbol_hash, symbol_equal);
    stbl->htable = HashTable_Create(&hashinfo);
  }
  return stbl->htable;
}

int STable_Init(STable *stbl)
{
  stbl->htable = NULL;
  Decl_HashInfo(hashinfo, item_hash, item_equal);
  stbl->itable = ItemTable_Create(&hashinfo, ITEM_MAX);
  Vector_Init(&stbl->vector);
  return 0;
}

void STable_Fini(STable *stbl)
{
  Vector_Fini(&stbl->vector, NULL, NULL);
}

Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst)
{
  int access = bconst ? ACCESS_CONST : 0;
  access |= isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int name_index = StringItem_Set(stbl->itable, name);
  int desc_index = -1;
  if (desc != NULL) {
    desc_index = TypeItem_Set(stbl->itable, desc);
  }
  Symbol *sym = Symbol_New(name_index, SYM_VAR, access, desc_index);
  if (HashTable_Insert(__get_hashtable(stbl), &sym->hnode) < 0) {
    debug_error("add '%s' variable failed.\n", name);
    Symbol_Free(sym);
    return NULL;
  }
  int index = Vector_Appand(&stbl->vector, sym);
  ASSERT(index >= 0);
  Symbol_Set_Index(sym, index);
  return sym;
}

Symbol *STable_Add_Func(STable *stbl, char *name, ProtoInfo *proto)
{
  int access = isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int name_index = StringItem_Set(stbl->itable, name);
  int desc_index = ProtoItem_Set(stbl->itable, proto);
  Symbol *sym = Symbol_New(name_index, SYM_FUNC, access, desc_index);
  if (HashTable_Insert(__get_hashtable(stbl), &sym->hnode) < 0) {
    debug_error("add a function failed.\n");
    Symbol_Free(sym);
    return NULL;
  }
  Vector_Appand(&stbl->vector, sym);
  return sym;
}

Symbol *STable_Add_Klass(STable *stbl, char *name, int kind)
{
  int access = isupper(name[0]) ? ACCESS_PUBLIC : ACCESS_PRIVATE;
  int name_index = StringItem_Set(stbl->itable, name);
  Symbol *sym = Symbol_New(name_index, kind, access, -1);
  if (HashTable_Insert(__get_hashtable(stbl), &sym->hnode) < 0) {
    debug_error("add a function failed.\n");
    Symbol_Free(sym);
    return NULL;
  }
  Vector_Appand(&stbl->vector, sym);
  return sym;
}

Symbol *STable_Get(STable *stbl, char *name)
{
  int index = StringItem_Get(stbl->itable, name);
  if (index < 0) return NULL;
  Symbol sym = {.name_index = index};
  HashNode *hnode = HashTable_Find(__get_hashtable(stbl), &sym);
  if (hnode != NULL) {
    return container_of(hnode, Symbol, hnode);
  } else {
    return NULL;
  }
}

Symbol *Symbol_New(int name_index, int kind, int access, int desc_index)
{
  Symbol *sym = calloc(1, sizeof(Symbol));
  Init_HashNode(&sym->hnode, sym);
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

/*-------------------------------------------------------------------------*/

static inline char *to_str(char *str[], int size, int idx)
{
  return (idx >= 0 && idx < size) ? str[idx] : "";
}

static char *type_tostr(int kind)
{
  static char *kind_str[] = {
    "", "var", "func", "class", "field", "method", "interface", "iproto"
  };

  return to_str(kind_str, nr_elts(kind_str), kind);
}

static char *name_tostr(int index, STable *stbl)
{
  if (index < 0) return "";
  StringItem *item = ItemTable_Get(stbl->itable, ITEM_STRING, index);
  ASSERT_PTR(item);
  return item->data;
}

/*-------------------------------------------------------------------------*/

static void desc_show(int index, STable *stbl)
{
  if (index < 0) return;

  static char ch[] = {
    'i', 'f', 'z', 's', 'A'
  };

  static char *str[] = {
    "int", "float", "bool", "string", "Any"
  };

  TypeItem *type = ItemTable_Get(stbl->itable, ITEM_TYPE, index);
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
    StringItem *item = ItemTable_Get(stbl->itable, ITEM_STRING, type->index);
    ASSERT_PTR(item);
    char *s = item->data;
    ASSERT_PTR(s);
    printf("%s", s);
  }
}

static void symbol_var_show(Symbol *sym, STable *stbl)
{
  ASSERT(sym->kind == SYM_VAR);
  char *str;
  if (sym->kind == 1 && sym->access & ACCESS_CONST)
    str = "const";
  else
    str = type_tostr(sym->kind);
  /* show's format: "type name desc;" */
  printf("%s %s ", str, name_tostr(sym->name_index, stbl));
  desc_show(sym->desc_index, stbl);
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
  int index = sym->desc_index;
  if (index < 0) return;
  ProtoItem *proto = ItemTable_Get(stbl->itable, ITEM_PROTO, index);
  ASSERT_PTR(proto);

  TypeListItem *typelist;
  if (proto->pindex >= 0) {
    typelist = ItemTable_Get(stbl->itable, ITEM_TYPELIST, proto->pindex);
    ASSERT_PTR(typelist);
    printf("(");
    typelist_show(typelist, stbl);
    printf(")");
  } else {
    printf("()");
  }

  if (proto->rindex >= 0) {
    printf(" ");
    typelist = ItemTable_Get(stbl->itable, ITEM_TYPELIST, proto->rindex);
    ASSERT_PTR(typelist);
    if (typelist->size > 1) printf("(");
    typelist_show(typelist, stbl);
    if (typelist->size > 1) printf(")");
  }

  puts(";");  /* with newline */
}

static void symbol_func_show(Symbol *sym, STable *stbl)
{
  ASSERT(sym->kind == SYM_FUNC);
  /* show's format: "func name args rets;" */
  printf("func %s", name_tostr(sym->name_index, stbl));
  proto_show(sym, stbl);
}

typedef void (*symbol_show_func)(Symbol *sym, STable *stbl);

static symbol_show_func show_funcs[] = {
  NULL,
  symbol_var_show,
  symbol_func_show,
};

static void symbol_show(Symbol *sym, STable *stbl)
{
  symbol_show_func show = show_funcs[sym->kind];
  ASSERT_PTR(show);
  show(sym, stbl);
}

void STable_Show(STable *stbl)
{
  int sz = Vector_Size(&stbl->vector);
  Symbol *sym;
  for (int i = 0; i < sz; i++) {
    sym = Vector_Get(&stbl->vector, i);
     symbol_show(sym, stbl);
  }
}
