/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/moduleobject.h"
#include "node.h"

static VECTOR_PTR(modules);

static int _module_set_(Object *ob, char *name, Object *val)
{

}

static Object *_module_get_(Object *ob, char *name)
{

}

TypeObject module_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Module",
  .setfunc = _module_set_,
  .getfunc = _module_get_,
};

static Object *_module_members_(Object *ob, Object *args)
{

}

static Object *_module_name_(Object *ob, Object *args)
{

}

static struct cfuncdef mod_cfuncs[] = {
  {"__init__", "s", NULL, _module_new_},
  {"__members__", NULL, "Llang.Tuple;", _module_members_},
  {"__name__", NULL, "s", _module_name_},
  {"__variables__", NULL, "Llang.Tuple;", _module_get_vars_},
  {"__functions__", NULL, "Llang.Tuple;", _module_get_funcs_},
  {"add_const", "sA", "z", _module_add_const_},
  {"add_var", "ss", "z", _module_add_var_},
  {"add_func", "sLlang.Code;", "z", _module_add_func_},
  {"remove", "s", "z", _module_remove_},
  {NULL}
};

void init_moduleobject(void)
{
  mtbl_init(&module_type.mtbl);
  klass_add_cfuncs(&module_type, mod_cfuncs);
}

void fini_moduleobject(void)
{
  mtbl_fini(&module_type.mtbl);
}

Object *new_module(char *name)
{
  ModuleObject *mob = kmalloc(sizeof(*mob));
  init_object_head(mob, &module_type);
  mob->name = atom(name);
  mtbl_init(&mob->mtbl);
  return (Object *)mob;
}

void install_module(char *path, Object *ob)
{
  OB_TYPE_ASSERT(ob, &module_type);
  char **name = path_toarr(path, strlen(path));
  int res = add_leaf(name, ob);
  if (!res) {
    vector_push_back(&modules, ob);
  }
  kfree(name);
}
