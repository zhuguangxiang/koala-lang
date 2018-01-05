
#include "moduleobject.h"
#include "symbol.h"
#include "debug.h"

#define ATOM_ITEM_MAX   (ITEM_CONST + 1)

static uint32 htable_hash(void *key)
{
  ItemEntry *e = key;
  assert(e->type > 0 && e->type < ATOM_ITEM_MAX);
  item_hash_t hash_fn = item_func[e->type].ihash;
  assert(hash_fn != NULL);
  return hash_fn(e->data);
}

static int htable_equal(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  assert(e1->type > 0 && e1->type < ATOM_ITEM_MAX);
  assert(e2->type > 0 && e2->type < ATOM_ITEM_MAX);
  if (e1->type != e2->type) return 0;
  item_equal_t equal_fn = item_func[e1->type].iequal;
  assert(equal_fn != NULL);
  return equal_fn(e1->data, e2->data);
}

Object *Module_New(char *name, int nr_locals)
{
  int size = sizeof(ModuleObject) + sizeof(TValue) * nr_locals;
  ModuleObject *ob = malloc(size);
  init_object_head(ob, &Module_Klass);
  ob->stable = HashTable_Create(Symbol_Hash, Symbol_Equal);
  ob->itable = ItemTable_Create(htable_hash, htable_equal, ATOM_ITEM_MAX);
  ob->name = name;
  ob->avail_index = 0;
  ob->size = nr_locals;
  for (int i = 0; i < nr_locals; i++)
    init_nil_value(ob->locals + i);
  return (Object *)ob;
}

void Module_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  free(ob);
}

int Module_Add_Var(Object *ob, char *name, char *desc, uint8 access)
{
  ModuleObject *mo = (ModuleObject *)ob;
  assert(mo->avail_index < mo->size);
  int name_index = StringItem_Set(mo->itable, name);
  int desc_index = TypeItem_Set(mo->itable, desc);
  Symbol *sym = Symbol_New(name_index, SYM_VAR, access, desc_index);
  sym->value.index = mo->avail_index++;
  return HashTable_Insert(mo->stable, &sym->hnode);
}

int Module_Add_Func(Object *ob, char *name, char *rdesc[], int rsz,
                    char *pdesc[], int psz, uint8 access, Object *method)
{
  ModuleObject *mo = (ModuleObject *)ob;
  int name_index = StringItem_Set(mo->itable, name);
  int desc_index = ProtoItem_Set(mo->itable, rdesc, rsz, pdesc, psz);
  Symbol *sym = Symbol_New(name_index, SYM_FUNC, access, desc_index);
  sym->value.method = method;
  return HashTable_Insert(mo->stable, &sym->hnode);
}

static int module_add_klass(Object *ob, Klass *klazz, uint8 access, int kind)
{
  ModuleObject *mo = (ModuleObject *)ob;
  int name_index = StringItem_Set(mo->itable, klazz->name);
  Symbol *sym = Symbol_New(name_index, kind, access, name_index);
  return HashTable_Insert(mo->stable, &sym->hnode);
}

int Module_Add_Class(Object *ob, Klass *klazz, uint8 access)
{
  return module_add_klass(ob, klazz, access, SYM_CLASS);
}

int Module_Add_Intf(Object *ob, Klass *klazz, uint8 access)
{
  return module_add_klass(ob, klazz, access, SYM_INTF);
}

Symbol *__module_get(ModuleObject *mo, char *name)
{
  int index = StringItem_Get(mo->itable, name);
  if (index < 0) return NULL;
  Symbol sym = {.name_index = index};
  HashNode *hnode = HashTable_Find(mo->stable, &sym);
  if (hnode != NULL) {
    return container_of(hnode, Symbol, hnode);
  } else {
    return NULL;
  }
}

int Module_Get_VarValue(Object *ob, char *name, TValue *v)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  struct symbol *s = __module_get(mo, name);
  if (s != NULL) {
    if (s->kind == SYM_VAR) {
      assert(s->value.index < mo->size);
      *v = mo->locals[s->value.index];
      return 0;
    } else {
      debug_error("symbol is not a variable\n");
    }
  }

  return -1;
}

int Module_Get_FuncValue(Object *ob, char *name, Object **func)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  Symbol *s = __module_get(mo, name);
  if (s != NULL) {
    if (s->kind == SYM_FUNC) {
      *func = s->value.method;
      return 0;
    } else {
      debug_error("symbol is not a function\n");
    }
  }

  return -1;
}

Object *Load_Module(char *path)
{
  UNUSED_PARAMETER(path);
  return NULL;
}

/*-------------------------------------------------------------------------*/

static void module_free(Object *ob)
{
  Module_Free(ob);
}

Klass Module_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Module",
  .bsize = sizeof(ModuleObject),

  .ob_free = module_free
};

/*-------------------------------------------------------------------------*/

static void module_visit(struct hlist_head *head, int size, void *arg)
{
  Symbol *sym;
  ItemTable *itable = arg;
  HashNode *hnode;
  for (int i = 0; i < size; i++) {
    if (!hlist_empty(head)) {
      hlist_for_each_entry(hnode, head, link) {
        sym = container_of(hnode, Symbol, hnode);
        Symbol_Display(sym, itable);
      }
      printf("\n");
    }
    head++;
  }
}

void Module_Display(Object *ob)
{
  assert(OB_KLASS(ob) == &Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  HashTable_Traverse(mo->stable, module_visit, mo->itable);
}
