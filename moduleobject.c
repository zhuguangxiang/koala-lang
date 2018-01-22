
#include "koala.h"

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
  ob->itable = SItemTable_Create();
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

static HashTable *__get_table(ModuleObject *mob)
{
  if (mob->stable == NULL)
    mob->stable = SHashTable_Create();
  return mob->stable;
}

int Module_Add_Var(Object *ob, char *name, char *desc, int access)
{
  ModuleObject *mob = (ModuleObject *)ob;
  ASSERT(mob->avail_index < mob->size);
  int name_index = StringItem_Set(mob->itable, name, strlen(name));
  int desc_index = TypeItem_Set(mob->itable, desc, strlen(desc));
  Symbol *sym = Symbol_New(name_index, SYM_VAR, access, desc_index);
  sym->index = mob->avail_index++;
  return HashTable_Insert(__get_table(mob), &sym->hnode);
}

int Module_Add_Func(Object *ob, char *name, char *rdesc, char *pdesc,
                    int access, Object *method)
{
  ModuleObject *mob = (ModuleObject *)ob;
  int name_index = StringItem_Set(mob->itable, name, strlen(name));
  int desc_index = ProtoItem_Set(mob->itable, rdesc, pdesc, NULL, NULL);
  Symbol *sym = Symbol_New(name_index, SYM_FUNC, access, desc_index);
  sym->obj = method;
  return HashTable_Insert(__get_table(mob), &sym->hnode);
}

int Module_Add_CFunc(Object *ob, FuncStruct *f)
{
  MethodProto proto;
  FuncStruct_Get_Proto(&proto, f);
  Object *meth = CMethod_New(f->func, &proto);
  return Module_Add_Func(ob, f->name, f->rdesc, f->pdesc, f->access, meth);
}

static int module_add_klass(Object *ob, Klass *klazz, int access, int kind)
{
  ModuleObject *mob = (ModuleObject *)ob;
  int name_index = StringItem_Set(mob->itable, klazz->name,
                                  strlen(klazz->name));
  Symbol *sym = Symbol_New(name_index, kind, access, name_index);
  sym->obj = klazz;
  klazz->itable = mob->itable;
  return HashTable_Insert(__get_table(mob), &sym->hnode);
}

int Module_Add_Class(Object *ob, Klass *klazz, int access)
{
  return module_add_klass(ob, klazz, access, SYM_CLASS);
}

int Module_Add_Interface(Object *ob, Klass *klazz, int access)
{
  return module_add_klass(ob, klazz, access, SYM_INTF);
}

static Symbol *__module_get(ModuleObject *mob, char *name)
{
  int index = StringItem_Get(mob->itable, name, strlen(name));
  if (index < 0) return NULL;
  Symbol sym = {.name_index = index};
  HashNode *hnode = HashTable_Find(__get_table(mob), &sym);
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
      ASSERT(s->index < mob->size);
      return s->index;
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
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  Symbol *s = __module_get(mob, name);
  if (s != NULL) {
    if (s->kind == SYM_FUNC) {
      return s->obj;
    } else {
      debug_error("symbol is not a function\n");
    }
  }

  return NULL;
}

Klass *Module_Get_Class(Object *ob, char *name)
{
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  Symbol *s = __module_get(mob, name);
  if (s != NULL) {
    if (s->kind == SYM_CLASS) {
      return s->obj;
    } else {
      debug_error("symbol is not a class\n");
    }
  }

  return NULL;
}

Klass *Module_Get_Intf(Object *ob, char *name)
{
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  Symbol *s = __module_get(mob, name);
  if (s != NULL) {
    if (s->kind == SYM_INTF) {
      return s->obj;
    } else {
      debug_error("symbol is not a interface\n");
    }
  }

  return NULL;
}

int Module_Add_CFunctions(Object *ob, FuncStruct *funcs)
{
  OB_ASSERT_KLASS(ob, Module_Klass);
  int res;
  FuncStruct *f = funcs;
  while (f->name != NULL) {
    res = Module_Add_CFunc(ob, f);
    ASSERT(res == 0);
    ++f;
  }
  return 0;
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
  ModuleObject *mob = OB_TYPE_OF(ob, ModuleObject, Module_Klass);
  printf("package:%s\n", mob->name);
  HashTable_Traverse(__get_table(mob), module_visit, mob->itable);
}
