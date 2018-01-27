
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
    HashInfo hashinfo = HashInfo_Init(symbol_hash, symbol_equal);
    stbl->htable = HashTable_Create(&hashinfo);
  }
  return stbl->htable;
}

int STable_Init(STable *stbl)
{
  HashInfo hashinfo = HashInfo_Init(item_hash, item_equal);
  stbl->itable = ItemTable_Create(&hashinfo, ITEM_MAX);
  return 0;
}

void STable_Fini(STable *stbl)
{
  free(stbl);
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
    debug_error("add a variable failed.\n");
    Symbol_Free(sym);
    return NULL;
  }
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

/*-------------------------------------------------------------------------*/

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

static char *access_tostr(int access)
{
  static char *str[] = {
    "public", "private"
  };

  return to_str(str, nr_elts(str), access);
}

static char *get_name(int index, STable *stbl)
{
  StringItem *item = ItemTable_Get(stbl->itable, ITEM_STRING, index);
  return item->data;
}

static char *desc_tostr(int index, STable *stbl)
{
  if (index < 0) return "";

  static char ch[] = {
    'i', 'f', 'z', 's', 'A'
  };

  static char *str[] = {
    "int", "float", "bool", "string", "Any"
  };

  TypeItem *item = ItemTable_Get(stbl->itable, ITEM_TYPE, index);
  StringItem *sitem = ItemTable_Get(stbl->itable, ITEM_STRING, item->index);

  char *s = sitem->data;
  if (s == NULL) return NULL;
  if (s[1] == '\0') {
    for (int i = 0; i < nr_elts(ch); i++) {
      if (ch[i] == *s) {
        return str[i];
      }
    }
  }
  return NULL;
}

void Symbol_Display(Symbol *sym, STable *stbl)
{
  char *str = "var";
  if (sym->kind == 1 && sym->access & ACCESS_CONST)
    str = "const";
  else
    str = type_tostr(sym->kind);
  printf("%s %s %s %s\n",
         access_tostr(sym->access & 1),
         str,
         get_name(sym->name_index, stbl),
         desc_tostr(sym->desc_index, stbl));
}

void Symbol_Visit(HashList *head, int size, void *arg)
{
  Symbol *sym;
  STable *stbl = arg;
  HashNode *hnode;
  for (int i = 0; i < size; i++) {
    if (!HashList_Empty(head)) {
      HashList_ForEach(hnode, head) {
        sym = container_of(hnode, Symbol, hnode);
        Symbol_Display(sym, stbl);
      }
    }
    head++;
  }
}

void STable_Display(STable *stbl)
{
  HashTable_Traverse(__get_hashtable(stbl), Symbol_Visit, stbl);
}
