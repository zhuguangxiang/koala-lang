
#include "symbol.h"
#include "debug.h"
#include "koala.h"

#define ATOM_ITEM_MAX   (ITEM_CONST + 1)

static uint32 htable_hash(void *key)
{
  ItemEntry *e = key;
  ASSERT(e->type > 0 && e->type < ATOM_ITEM_MAX);
  item_hash_t hash_fn = item_func[e->type].ihash;
  ASSERT(hash_fn != NULL);
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
  ASSERT(equal_fn != NULL);
  return equal_fn(e1->data, e2->data);
}

/*-------------------------------------------------------------------------*/

Object *Module_New(char *name, char *path, int nr_locals)
{
  int size = sizeof(ModuleObject) + sizeof(TValue) * nr_locals;
  ModuleObject *ob = malloc(size);
  init_object_head(ob, &Module_Klass);
  ob->stable = NULL;
  ob->itable = NULL;
  ob->name = name;
  ob->avail_index = 0;
  ob->size = nr_locals;
  ob->itable = ItemTable_Create(htable_hash, htable_equal, ATOM_ITEM_MAX);
  for (int i = 0; i < nr_locals; i++)
    initnilvalue(ob->locals + i);
  if (Koala_Add_Module(path, (Object *)ob) < 0) {
    Module_Free((Object *)ob);
    return NULL;
  }
  return (Object *)ob;
}

void Module_Free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  free(ob);
}

static HashTable *__get_table(ModuleObject *mo)
{
  if (mo->stable == NULL)
    mo->stable = HashTable_Create(Symbol_Hash, Symbol_Equal);
  return mo->stable;
}

int Module_Add_Var(Object *ob, char *name, char *desc, uint8 access)
{
  ModuleObject *mo = (ModuleObject *)ob;
  ASSERT(mo->avail_index < mo->size);
  int name_index = StringItem_Set(mo->itable, name, strlen(name));
  int desc_index = TypeItem_Set(mo->itable, desc, strlen(desc));
  Symbol *sym = Symbol_New(name_index, SYM_VAR, access, desc_index);
  sym->value.index = mo->avail_index++;
  return HashTable_Insert(__get_table(mo), &sym->hnode);
}

int Module_Add_Func(Object *ob, char *name, char *rdesc, char *pdesc,
                    uint8 access, Object *method)
{
  ModuleObject *mo = (ModuleObject *)ob;
  int name_index = StringItem_Set(mo->itable, name, strlen(name));
  int desc_index = ProtoItem_Set(mo->itable, rdesc, pdesc, NULL, NULL);
  Symbol *sym = Symbol_New(name_index, SYM_FUNC, access, desc_index);
  sym->value.obj = method;
  return HashTable_Insert(__get_table(mo), &sym->hnode);
}

static int module_add_klass(Object *ob, Klass *klazz, uint8 access, int kind)
{
  ModuleObject *mo = (ModuleObject *)ob;
  int name_index = StringItem_Set(mo->itable, klazz->name,
                                  strlen(klazz->name));
  Symbol *sym = Symbol_New(name_index, kind, access, name_index);
  sym->value.obj = klazz;
  return HashTable_Insert(__get_table(mo), &sym->hnode);
}

int Module_Add_Class(Object *ob, Klass *klazz, uint8 access)
{
  return module_add_klass(ob, klazz, access, SYM_CLASS);
}

int Module_Add_Interface(Object *ob, Klass *klazz, uint8 access)
{
  return module_add_klass(ob, klazz, access, SYM_INTF);
}

static Symbol *__module_get(ModuleObject *mo, char *name)
{
  int index = StringItem_Get(mo->itable, name, strlen(name));
  if (index < 0) return NULL;
  Symbol sym = {.name_index = index};
  HashNode *hnode = HashTable_Find(__get_table(mo), &sym);
  if (hnode != NULL) {
    return container_of(hnode, Symbol, hnode);
  } else {
    return NULL;
  }
}

static int __get_value_index(ModuleObject *mob, char *name)
{
  struct symbol *s = __module_get(mob, name);
  if (s != NULL) {
    if (s->kind == SYM_VAR) {
      ASSERT(s->value.index < mob->size);
      return s->value.index;
    } else {
      debug_error("symbol is not a variable\n");
    }
  }
  return -1;
}

TValue Module_Get_Value(Object *ob, char *name)
{
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  int index = __get_value_index(mob, name);
  if (index < 0) return NilValue;
  return mob->locals[index];
}

void Module_Set_Value(Object *ob, char *name, TValue *val)
{
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  int index = __get_value_index(mob, name);
  if (index < 0) return;
  mob->locals[index] = *val;
}

Object *Module_Get_Function(Object *ob, char *name)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  Symbol *s = __module_get(mo, name);
  if (s != NULL) {
    if (s->kind == SYM_FUNC) {
      return s->value.obj;
    } else {
      debug_error("symbol is not a function\n");
    }
  }

  return NULL;
}

Object *Module_Get_Class(Object *ob, char *name)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  Symbol *s = __module_get(mo, name);
  if (s != NULL) {
    if (s->kind == SYM_CLASS) {
      return s->value.obj;
    } else {
      debug_error("symbol is not a class\n");
    }
  }

  return NULL;
}

Object *Module_Get_Interface(Object *ob, char *name)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  Symbol *s = __module_get(mo, name);
  if (s != NULL) {
    if (s->kind == SYM_INTF) {
      return s->value.obj;
    } else {
      debug_error("symbol is not a interface\n");
    }
  }

  return NULL;
}

int Module_Add_CFunctions(Object *ob, FunctionStruct *funcs)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  int res;
  FunctionStruct *f = funcs;
  Object *meth;
  while (f->name != NULL) {
    meth = CMethod_New(f->func);
    res = Module_Add_Func(ob, f->name, f->rdesc, f->pdesc,
                          (uint8)f->access, meth);
    ASSERT(res == 0);
    ++f;
  }
  return 0;
}

/*-------------------------------------------------------------------------*/

void Init_Module_Klass(Object *ob)
{
  Module_Add_Class(ob, &Module_Klass, ACCESS_PUBLIC);
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
    }
    head++;
  }
}

void Module_Display(Object *ob)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  ModuleObject *mo = (ModuleObject *)ob;
  printf("package:%s\n", mo->name);
  HashTable_Traverse(__get_table(mo), module_visit, mo->itable);
}
