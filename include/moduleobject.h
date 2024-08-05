/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ModInitFunc)(Object *m);
typedef void (*ModFiniFunc)(Object *m);

typedef struct _ModuleDef {
    char *name;
    size_t size;
    MethodDef *methods;
    ModInitFunc init;
    ModFiniFunc fini;
} ModuleDef;

typedef struct _ModuleObject {
    OBJECT_HEAD
    /* module definition */
    ModuleDef *def;
    /* module state */
    void *state;
    /* all symbols */
    Vector symbols;
    /* global values */
    Vector values;
    /* relocations */
    Vector relocs;
} ModuleObject;

typedef struct _RelocInfo {
    /* module name */
    const char *name;
    /* module object */
    ModuleObject *module;
} RelocInfo;

extern TypeObject module_type;
#define IS_MODULE(ob) IS_TYPE((ob), &module_type)

Object *kl_new_module(const char *name);
int module_add_code(Object *m, Object *code);
int module_add_cfunc(Object *m, MethodDef *def);
int module_add_type(Object *m, TypeObject *ty);

static inline ModuleObject *module_get_reloc(Object *_m, int index)
{
    ModuleObject *m = (ModuleObject *)_m;
    RelocInfo *item = vector_get(&m->relocs, index);
    if (item) return item->module;
    return NULL;
}

static inline Object *module_get_symbol(Object *_m, int m_idx, int index)
{
    ModuleObject *m = module_get_reloc(_m, m_idx);
    if (!m) return NULL;
    Object **item = vector_get(&m->symbols, index);
    if (item) return *item;
    return NULL;
}

static inline const char *module_get_name(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    return m->def->name;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */
