/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MODULE_OBJECT_H_
#define _KOALA_MODULE_OBJECT_H_

#include "fieldobject.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ModInitFunc)(Object *m);
typedef void (*ModFiniFunc)(Object *m);

typedef struct _ModuleDef {
    char *name;
    size_t size;
    MemberDef *members;
    GetSetDef *getsets;
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
    /* all symbols map for external module link */
    HashMap map;
    /* constants */
    Vector consts;
    /* relocations for external symbols */
    Vector rels;
} ModuleObject;

typedef struct _RelocInfo {
    /* namespace */
    /* module: ./a/b; class: ./a/b.Foo */
    const char *ns;
    /* symbols */
    Vector syms;
} RelocInfo;

typedef struct _SymbolInfo {
    /* symbol name */
    const char *name;
    /* Object: FieldObject/CodeObject/CFuncObject */
    Object *obj;
} SymbolInfo;

extern TypeObject module_type;
#define IS_MODULE(ob) IS_TYPE((ob), &module_type)

Object *kl_new_module(const char *name);
int module_add_member(Object *_m, MemberDef *member);
int module_add_getset(Object *_m, GetSetDef *getset);
int module_add_cfunc(Object *m, MethodDef *def);
int module_add_object(Object *_m, const char *name, Object *obj);

static inline RelocInfo *module_get_rel(Object *_m, int index)
{
    ModuleObject *m = (ModuleObject *)_m;
    RelocInfo *item = vector_get(&m->rels, index);
    return item;
}

static inline Object *module_get_symbol(Object *_m, int index)
{
    ModuleObject *m = (ModuleObject *)_m;
    Object **item = vector_get(&m->symbols, index);
    if (item) return *item;
    return NULL;
}

static inline const char *module_get_name(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;
    return m->def->name;
}

int kl_module_def_init(ModuleDef *def);
int kl_module_link(Object *_m);
int module_add_int_const(Object *_m, int64_t val);
int module_add_str_const(Object *_m, const char *s);
int module_add_obj_const(Object *_m, Object *obj);
int module_add_rel(Object *_m, const char *path, SymbolInfo *sym);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MODULE_OBJECT_H_ */
