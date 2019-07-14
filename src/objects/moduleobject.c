/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/moduleobject.h"
#include "atom.h"
#include "node.h"
#include "path.h"

static VECTOR_PTR(modules);

struct klass module_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Module",
};

struct object *__module_members__(struct object *ob, struct object *args)
{

}

struct object *__module_name__(struct object *ob, struct object *args)
{

}

static struct cfuncdef mod_funcs[] = {
  {"__members__", NULL, "Llang.Tuple;", __module_members__},
  {"__name__", NULL, "s", __module_name__},
  {NULL}
};

void module_initialize(void)
{
  atom_initialize();
  node_initialize();
  mtable_init(&module_type.mtbl);
  klass_add_cfuncs(&module_type, mod_funcs);
}

void module_destroy(void)
{
  mtable_free(&module_type.mtbl);
  node_destroy();
  atom_destroy();
  typedesc_destroy();
}

struct object *new_module(char *name)
{
  struct module_object *mob = kmalloc(sizeof(*mob));
  init_object_head(mob, &module_type);
  mob->name = atom(name);
  mtable_init(&mob->mtbl);
  return (struct object *)mob;
}

void install_module(char *path, struct object *ob)
{
  OB_TYPE_ASSERT(ob, &module_type);
  char **name = path_toarr(path, strlen(path));
  int res = add_leaf(name, ob);
  if (!res) {
    vector_push_back(&modules, ob);
  }
  kfree(name);
}
