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

typedef struct moduleobject {
  OBJECT_HEAD
  /* module name */
  char *name;
  /* member table */
  MTable mtbl;
  /* tuple of value objects */
  Object *values;
  /* tuple of consts pool */
  Object *consts;
} ModuleObject;

extern TypeObject module_type;
Object *new_module(char *name);
void install_module(char *path, Object *ob);

#define module_add_const(ob, name, type, val)  \
({                                             \
  OB_TYPE_ASSERT(ob, &module_type);          \
  ModuleObject *mod = (ModuleObject *)(ob);    \
  mtbl_add_const(&mod->mtbl, name, type, val); \
})

#define module_add_var(ob, name, type)      \
({                                          \
  OB_TYPE_ASSERT(ob, &module_type);       \
  ModuleObject *mod = (ModuleObject *)(ob); \
  mtbl_add_var(&mod->mtbl, name, type);     \
})

#define module_add_func(ob, name, code)     \
({                                          \
  OB_TYPE_ASSERT(ob, &module_type);       \
  ModuleObject *mod = (ModuleObject *)(ob); \
  mtbl_add_func(&mod->mtbl, name, code);    \
})

#define module_add_cfuncs(ob, funcs)        \
({                                          \
  OB_TYPE_ASSERT(ob, &module_type);       \
  ModuleObject *mod = (ModuleObject *)(ob); \
  mtbl_add_cfuncs(&mod->mtbl, cfuncs);      \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */
