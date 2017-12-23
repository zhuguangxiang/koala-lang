
#include "moduleobject.h"
#include "nameobject.h"
#include "tableobject.h"
#include "tupleobject.h"

Object *Module_New(char *name, int nr_vars)
{
  ModuleObject *ob = malloc(sizeof(*ob));
  init_object_head(ob, &Module_Klass);
  ob->name  = name;
  ob->avail = 0;
  ob->table = NULL;
  if (nr_vars > 0)
    ob->tuple = Tuple_New(nr_vars);
  else
    ob->tuple = NULL;

  Object_Add_GCList((Object *)ob);
  return (Object *)ob;
}

void Module_Free(Object *ob)
{
  OB_CHECK_KLASS(ob, Module_Klass);
  free(ob);
}

static Object *__get_table(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  if (mo->table == NULL) mo->table = Table_New();
  return mo->table;
}

int Module_Add_Variable(Object *ob, char *name, char *desc,
                        uint8 access, int k)
{
  ModuleObject *mo = (ModuleObject *)ob;

  if (mo->tuple == NULL) {
    fprintf(stderr, "[ERROR]please set number of variables in this module.\n");
    return -1;
  }

  if (mo->avail >= Tuple_Size(mo->tuple)) {
    fprintf(stderr, "[ERROR]there is no space for vars.");
    return -1;
  }
  int type = (k == 0) ? NT_VAR : NT_CONST;
  Object *no = Name_New(name, type, access, desc, NULL);
  TValue key = TValue_Build('O', no);
  TValue val = TValue_Build('i', mo->avail++);
  return Table_Put(__get_table(ob), key, val);
}

int Module_Add_Function(Object *ob, char *name, char *desc, char *pdesc,
                        uint8 access, Object *method)
{
  int type = NT_FUNC;
  Object *no = Name_New(name, type, access, desc, pdesc);
  TValue key = TValue_Build('O', no);
  TValue val = TValue_Build('O', method);
  return Table_Put(__get_table(ob), key, val);
}

int Module_Add_Klass(Object *ob, Klass *klazz, uint8 access, int intf)
{
  int type = (intf == 0) ? NT_KLASS : NT_INTF;
  Object *no = Name_New(klazz->name, type, access, NULL, NULL);
  TValue key = TValue_Build('O', no);
  TValue val = TValue_Build('O', klazz);
  return Table_Put(__get_table(ob), key, val);
}

int Module_Get(Object *ob, char *name, TValue *k, TValue *v)
{
  OB_CHECK_KLASS(ob, Module_Klass);
  Object *no = Name_New(name, 0, 0, NULL, NULL);
  TValue key = TValue_Build('O', no);
  return Table_Get(__get_table(ob), key, k, v);
}

Object *Load_Module(char *path_name)
{
  UNUSED_PARAMETER(path_name);
  return NULL;
}

/*-------------------------------------------------------------------------*/

static MethodStruct module_methods[] = {
  {NULL, NULL, NULL, 0, NULL}
};

void Init_Module_Klass(void)
{
  Klass_Add_Methods(&Module_Klass, module_methods);
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

static void module_visit(TValue key, TValue val, void *arg)
{
  UNUSED_PARAMETER(val);
  UNUSED_PARAMETER(arg);
  Object *ob = NULL;
  TValue_Parse(key, 'O', &ob);
  Name_Display(ob);
}

void Module_Display(Object *ob)
{
  assert(OB_KLASS(ob) == &Module_Klass);
  Table_Traverse(__get_table(ob), module_visit, NULL);
}
