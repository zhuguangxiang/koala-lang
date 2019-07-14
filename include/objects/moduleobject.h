/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct module_object {
  OBJECT_HEAD
  /* module name */
  char *name;
  /* member table */
  struct mtable mtbl;
  /* tuple of variables */
  struct object *variables;
  /* tuple of constant */
  struct object *consts;
};

extern struct klass module_type;
void module_initialize(void);
void module_destroy(void);
struct object *new_module(char *name);
void install_module(char *path, struct object *ob);

#define module_add_const(ob, name, type, val)    \
({                                               \
  OB_TYPE_ASSERT(ob, &module_type);              \
  struct module_object *mob =                    \
    (struct module_object *)(ob);                \
  mtable_add_const(&mob->mtbl, name, type, val); \
})

#define module_add_var(ob, name, type)    \
({                                        \
  OB_TYPE_ASSERT(ob, &module_type);       \
  struct module_object *mob =             \
    (struct module_object *)(ob);         \
  mtable_add_var(&mob->mtbl, name, type); \
})

#define module_add_func(ob, name, code)    \
({                                         \
  OB_TYPE_ASSERT(ob, &module_type);        \
  struct module_object *mob =              \
    (struct module_object *)(ob);          \
  mtable_add_func(&mob->mtbl, name, code); \
})

#define module_add_cfuncs(ob, funcs)     \
({                                       \
  OB_TYPE_ASSERT(ob, &module_type);      \
  struct module_object *mob =            \
    (struct module_object *)(ob);        \
  mtable_add_cfuncs(&mob->mtbl, cfuncs); \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */
