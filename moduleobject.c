
#include "koala.h"

/*-------------------------------------------------------------------------*/

Object *Module_New(char *name, char *path, int nr_locals)
{
  int size = sizeof(ModuleObject) + sizeof(TValue) * nr_locals;
  ModuleObject *ob = malloc(size);
  init_object_head(ob, &Module_Klass);
  ob->name = name;
  ob->avail = 0;
  ob->size = nr_locals;
  STable_Init(&ob->stable);
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

#define OBJECT_TO_MODULE(ob) OB_TYPE_OF(ob, ModuleObject, Module_Klass)

int Module_Add_Var(Object *ob, char *name, TypeDesc *desc, int bconst)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  ASSERT(mob->avail < mob->size);
  Symbol *sym = STable_Add_Var(&mob->stable, name, desc, bconst);
  if (sym != NULL) {
    sym->index = mob->avail++;
    return 0;
  }
  return -1;
}

int Module_Add_Func(Object *ob, char *name, ProtoInfo *proto, Object *meth)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  Symbol *sym = STable_Add_Func(&mob->stable, name, proto);
  if (sym != NULL) {
    sym->obj = meth;
    return 0;
  }
  return -1;
}

int Module_Add_CFunc(Object *ob, FuncDef *f)
{
  ProtoInfo proto;
  Init_ProtoInfo(f->rsz, f->rdesc, f->psz, f->pdesc, &proto);
  Object *meth = CMethod_New(f->fn, &proto);
  return Module_Add_Func(ob, f->name, &proto, meth);
}

int Module_Add_Class(Object *ob, Klass *klazz)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  Symbol *sym = STable_Add_Class(&mob->stable, klazz->name);
  if (sym != NULL) {
    sym->obj = klazz;
    klazz->stable.itable = mob->stable.itable;
    return 0;
  }
  return -1;
}

int Module_Add_Interface(Object *ob, Klass *klazz)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  Symbol *sym = STable_Add_Interface(&mob->stable, klazz->name);
  if (sym != NULL) {
    sym->obj = klazz;
    klazz->stable.itable = mob->stable.itable;
    return 0;
  }
  return -1;
}

static int __get_value_index(ModuleObject *mob, char *name)
{
  struct symbol *s = STable_Get(&mob->stable, name);
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
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  int index = __get_value_index(mob, name);
  if (index < 0) return NilValue;
  return mob->locals[index];
}

void Module_Set_Value(Object *ob, char *name, TValue *val)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  int index = __get_value_index(mob, name);
  if (index < 0) return;
  mob->locals[index] = *val;
}

Object *Module_Get_Function(Object *ob, char *name)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  Symbol *s = STable_Get(&mob->stable, name);
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
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  Symbol *s = STable_Get(&mob->stable, name);
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
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  Symbol *s = STable_Get(&mob->stable, name);
  if (s != NULL) {
    if (s->kind == SYM_INTF) {
      return s->obj;
    } else {
      debug_error("symbol is not a interface\n");
    }
  }

  return NULL;
}

int Module_Add_CFunctions(Object *ob, FuncDef *funcs)
{
  int res;
  FuncDef *f = funcs;
  while (f->name != NULL) {
    res = Module_Add_CFunc(ob, f);
    ASSERT(res == 0);
    ++f;
  }
  return 0;
}

void Init_Module_Klass(Object *ob)
{
  Module_Add_Class(ob, &Module_Klass);
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

void Module_Display(Object *ob)
{
  ModuleObject *mob = OBJECT_TO_MODULE(ob);
  printf("package:%s\n", mob->name);
  STable_Display(&mob->stable);
}
